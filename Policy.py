import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from overrides import overrides
from common.utils import create_nn_and_optimizer

class PolicyBase(metaclass=ABCMeta):
    def __init__(self, name: str):
        self.name = name
        self.state: torch.Tensor = None
        self.actions: torch.Tensor = None
        self.gamma = 0.999
        self.threads_count = -1
        self.step_coeff = 1.0
        self.step = 0

    def clean(self, threads_count: int):
        self.threads_count = threads_count
        self.step_coeff = 1.0
        self.step = 0

    def sample_actions(self, state: torch.Tensor) -> List[int]:
        self.state = state
        self.actions = self._sample_actions_impl(state)
        return self.actions

    def set_step_reward(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor, well_done: torch.Tensor):
        reward_advantage = self._update_values(new_state, reward, done, well_done)
        self._update_policy_nn(new_state, reward_advantage)

        #self.step_coeff *= self.gamma
        self.step += 1

    @abstractmethod
    def _update_values(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor, well_done: torch.Tensor) -> torch.Tensor:
        pass

    @abstractmethod
    def _update_policy_nn(self, new_state: torch.Tensor, reward_advantage: torch.Tensor):
        pass

    @abstractmethod
    def _sample_actions_impl(self, state) -> torch.Tensor:
        pass

    @abstractmethod
    def get_expected_return(selfself, state):
        pass

    @abstractmethod
    def clone(self):
        pass


class FastforwardPolicy(PolicyBase):
    def __init__(self, name: str, input_size: int, layers_sizes: List[int]):
        self.input_size = input_size
        self.layers_sizes = layers_sizes.copy()
        self.policy, self.policy_optim = create_nn_and_optimizer(input_size, layers_sizes, True, 0.001)
        layers_sizes[-1] = 1
        self.values, self.values_optim = create_nn_and_optimizer(input_size, layers_sizes, False, 0.001)
        self.values_lost = torch.nn.MSELoss()
        PolicyBase.__init__(self, name)

    @overrides
    def _update_values(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor, well_done: torch.Tensor) -> torch.Tensor:
        self.values.zero_grad()
        value = self.values(self.state)
        target = ((self.values(new_state).detach() * self.gamma + reward) * ~done)

        advantage = (target - value) * ~well_done.detach()
        critic_loss = (advantage * advantage) .mean()

        critic_loss.backward()
        self.values_optim.step()

        if self.step % 20 == 0:
            show_values_2d(lambda x, y: self.values(torch.tensor((0.0, 0.0, x, y))), (-1.0, 1.0), (-2.0, 2.0))

        return advantage.detach()


    @overrides
    def _update_policy_nn(self, new_state: torch.Tensor, reward_advantage):
        #std = torch.max(torch.abs(reward_advantage))
        #reward_advantage = reward_advantage / (std + 1e-8)

        #check1 = self.get_checkpoint()

        self.policy.zero_grad()
        actions_probabilities = self.policy(self.state)
        log_actions = torch.log(actions_probabilities + 1e-6)
        entropy = -torch.sum(log_actions * actions_probabilities, dim=-1, keepdim=True)
        loss: torch.Tensor = ((-(reward_advantage * log_actions.gather(1, self.actions) * self.step_coeff) - 0.01 * entropy)).mean()

        loss.backward()
        self.policy_optim.step()

        if self.step % 20 == 0:
            show_policy_2d(lambda x, y: self.policy(torch.tensor((0.0, 0.0, x, y))), 0, (-1.0, 1.0), (-1.0, 1.0))

        #if self.get_checkpoint() < check1 - 0.04:
        #    print("Error!!!!!!!!")


    @overrides
    def _sample_actions_impl(self, state) -> torch.Tensor:
        actions = self.policy(state)
        return actions.multinomial(1).detach()

    @overrides
    def get_expected_return(self, state):
        return self.values(state)

    @overrides
    def clone(self):
        new_policy = FastforwardPolicy(self.name, self.input_size, self.layers_sizes)
        buffer = io.BytesIO()
        torch.save(self.policy, buffer)
        buffer.seek(0)
        new_policy.policy = torch.load(buffer)
        buffer = io.BytesIO()
        torch.save(self.values, buffer)
        buffer.seek(0)
        new_policy.values = torch.load(buffer)
        new_policy.step = self.step
        new_policy.step_coeff = self.step_coeff
        return new_policy

    def get_checkpoint(self):
        state = np.zeros([4])
        state[2] = -4.0
        state[3] = -4.0
        state = torch.from_numpy(state).float()
        actions = self.policy(state)
        return actions[0].item()
