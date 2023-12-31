import numpy as np
import torch
import io
from typing import *
from diagrams import *
from overrides import overrides
from common.utils import create_nn_and_optimizer
from Policies.PolicyBase import PolicyBase

class A2CPolicy(PolicyBase):
    def __init__(self, name: str, input_size: int, layers_sizes: List[int]):
        self.input_size = input_size
        self.layers_sizes = layers_sizes.copy()
        self.policy, self.policy_optim = create_nn_and_optimizer(input_size, layers_sizes, True, 0.001)
        layers_sizes[-1] = 1
        self.values, self.values_optim = create_nn_and_optimizer(input_size, layers_sizes, False, 0.001)
        self.values_lost = torch.nn.MSELoss()
        PolicyBase.__init__(self, name)

    @overrides
    def _update_values(self,
                       prev_state: torch.Tensor,
                       actions: torch.Tensor,
                       new_state: torch.Tensor,
                       reward: torch.Tensor,
                       done: torch.Tensor,
                       well_done: torch.Tensor) -> torch.Tensor:
        self.values.zero_grad()
        value = self.values(prev_state.detach())
        target = ((self.values(new_state).detach() * self.gamma + reward.detach()) * ~done)

        advantage = (target - value) * ~well_done.detach()
        critic_loss = self.values_lost(value, target)
        critic_loss.backward()
        self.values_optim.step()

        if self.step % 20 == 0:
            show_values_2d(lambda x, y: self.values(torch.tensor((0.0, 0.0, x, y))), (-1.0, 1.0), (-2.0, 2.0))

        return advantage.detach()


    @overrides
    def _update_policy_nn(self,
                          prev_state: torch.Tensor,
                          new_state: torch.Tensor,
                          actions: torch.Tensor,
                          reward_advantage: torch.Tensor):
        #std = torch.max(torch.abs(reward_advantage))
        #reward_advantage = reward_advantage / (std + 1e-8)

        #check1 = self.get_checkpoint()

        self.policy.zero_grad()
        actions_probabilities = self.policy(prev_state.detach())
        log_actions = torch.log(actions_probabilities + 1e-6)
        entropy = -torch.sum(log_actions * actions_probabilities, dim=-1, keepdim=True)
        loss: torch.Tensor = ((-(reward_advantage.detach() * log_actions.gather(1, actions)) - 0.01 * entropy)).mean()

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
    def clone(self):
        new_policy = A2CPolicy(self.name, self.input_size, self.layers_sizes)
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
