import numpy as np
import torch
from abc import abstractmethod, ABCMeta
from typing import *

from overrides import overrides


class PolicyBase(metaclass=ABCMeta):
    def __init__(self, name: str):
        self.name = name
        self.state: torch.Tensor = None
        self.actions: List[int] = None
        self.gamma = 0.999
        self.done: torch.Tensor = None
        self.threads_count = -1

    def clean(self, threads_count: int):
        if self.done is not None:
            del self.done

        self.done = torch.zeros([threads_count, 1], dtype=torch.bool)
        self.threads_count = threads_count

    def is_done(self):
        return self.done.all()

    def sample_actions(self, state: torch.Tensor) -> List[int]:
        self.state = state
        self.actions = self._sample_actions_impl(state)
        return self.actions

    def set_step_reward(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor):
        reward_advantage = self._update_values(new_state, reward, done)
        self._update_policy_nn(new_state, reward_advantage)
        self.done |= done

    @abstractmethod
    def _update_values(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor) -> torch.Tensor:
        pass

    @abstractmethod
    def _update_policy_nn(self, new_state: torch.Tensor, reward_advantage: torch.Tensor):
        pass

    @abstractmethod
    def _sample_actions_impl(self, state) -> Tuple[List[int], torch.Tensor]:
        pass

    @abstractmethod
    def get_expected_return(selfself, state):
        pass


class FastforwardPolicy(PolicyBase):
    def __init__(self, name: str, input_size: int, layers_sizes: List[int]):
        self.policy, self.policy_optim = self._create_nn_and_optimizer(input_size, layers_sizes, True, 0.001)
        layers_sizes[-1] = 1
        self.values, self.values_optim = self._create_nn_and_optimizer(input_size, layers_sizes, False, 0.0004)
        PolicyBase.__init__(self, name)

        self.error = 0.0

    def get_and_reset_error(self):
        error = self.error
        self.error = 0.0
        return error


    def _create_nn_and_optimizer(self, input_size: int, layers_sizes: List, add_softmax: bool, lr: float) -> \
            Tuple[torch.nn.Sequential, torch.optim.Optimizer]:
        layers = OrderedDict()

        for id, size in enumerate(layers_sizes):
            layers["Layer_" + str(id)] = torch.nn.Linear(input_size, size)
            input_size = size

        if add_softmax:
            layers["Softmax"] = torch.nn.Softmax()

        nn = torch.nn.Sequential(layers)
        optimizer = torch.optim.Adam(nn.parameters(), lr=lr)
        return nn, optimizer

    @overrides
    def _update_values(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor) -> torch.Tensor:
        self.values_optim.zero_grad()
        self.values.zero_grad()
        value = self.values(self.state)
        detached_done = done.detach()
        target = (self.values(new_state).detach() * self.gamma + reward) * ~detached_done - detached_done * 100.0
        advantage = (target - value) * ~self.done.detach()
        critic_loss = (advantage * advantage).mean()
        critic_loss.backward()
        self.values_optim.step()
        self.error += critic_loss.item()


        # if critic_loss.item() > 100:
        #     for i in range(100):
        #         value = self.values(self.state)
        #         target = self.values(new_state).detach() * ~self.done * self.gamma + reward - done * 100
        #         advantage = (target - value) * ~self.done
        #         critic_loss = (advantage * advantage).mean()
        #         self.values.zero_grad()
        #         critic_loss.backward()
        #         self.values_optim.step()
        #         print("Adv: " + str(critic_loss))


        return advantage.detach()


    @overrides
    def _update_policy_nn(self, new_state: torch.Tensor, reward_advantage):
        #mean = torch.mean(reward_advantage)
        #std = torch.std(reward_advantage)
        #reward_advantage = (reward_advantage - mean) / (std + 1e-8)

        self.policy.zero_grad()
        self.policy_optim.zero_grad()
        actions_probabilities = self.policy(self.state)
        log_actions = torch.log(actions_probabilities + 1e-6)
        entropy = torch.sum(log_actions * actions_probabilities, dim=-1, keepdim=True) * ~self.done

        loss: torch.Tensor = (-(reward_advantage * log_actions.gather(1, self.actions)) + 0.1 * entropy).mean()
        loss.backward()
        self.policy_optim.step()
        #print("after: " + str(self.policy(self.state)))

    @overrides
    def _sample_actions_impl(self, state) -> Tuple[List[int], torch.Tensor]:
        actions = self.policy(state)
        return actions.multinomial(1).detach()

    @overrides
    def get_expected_return(self, state):
        return self.values(state)