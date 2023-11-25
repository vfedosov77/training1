import numpy as np
import torch
import copy
import io
from typing import *
from diagrams import *
from overrides import overrides
from common.utils import create_nn_and_optimizer
from Policies.PolicyBase import PolicyBase
from ExperienceDB import ExperienceDB


class QPolicy(PolicyBase):
    def __init__(self, name: str, input_size: int, layers_sizes: List[int]):
        self.qvalues, self.qvalues_optim = create_nn_and_optimizer(input_size, layers_sizes, False, 0.0001)
        self.target_q_network = copy.deepcopy(self.qvalues).eval()
        self.values_lost = torch.nn.MSELoss()
        self.buffer = ExperienceDB()
        self.batch_size = 64
        self.exploratiry_probability = 0.0
        PolicyBase.__init__(self, name)

    def _update_values(self,
                       prev_state: torch.Tensor,
                       actions: torch.Tensor,
                       new_state: torch.Tensor,
                       reward: torch.Tensor,
                       done: torch.Tensor,
                       well_done: torch.Tensor) -> torch.Tensor:
        self.buffer.add_ext(prev_state, actions, new_state, done, reward)

        if self.buffer.size() >= self.batch_size * 10:
            state, actions, new_state, done, reward = next(iter(self.buffer.get_batches(self.batch_size)))
            next_values = self.target_q_network(new_state)
            next_value = torch.max(next_values, dim=-1, keepdim=True)[0].detach()

            loss = self.values_lost(self.qvalues(prev_state)[actions], reward + next_value * ~done)
            self.qvalues.zero_grad()
            loss.backward()
            self.qvalues_optim.step()

            if self.step % 20 == 0:
                self.target_q_network.load_state_dict(self.qvalues.state_dict())

                show_policy_2d(lambda x, y: self.qvalues(torch.tensor((0.0, 0.0, x, y))), 0, (-1.0, 1.0), (-1.0, 1.0))
                show_values_2d(lambda x, y: self.qvalues(torch.tensor((0.0, 0.0, x, y)))[1], (-1.0, 1.0), (-1.0, 1.0))

        return reward


    def _update_policy_nn(self,
                          prev_state: torch.Tensor,
                          new_state: torch.Tensor,
                          actions: torch.Tensor,
                          reward_advantage: torch.Tensor):
        pass

    def _sample_actions_impl(self, state) -> torch.Tensor:
        use_exploratory = torch.rand(len(state), 1) < self.exploratiry_probability
        next_values = self.qvalues(state)
        next_value = torch.argmax(next_values, dim=-1, keepdim=True)
        exploratory = torch.randint(0, next_values.shape[1], (next_values.shape[0], 1))
        actions = ((next_value * ~use_exploratory) + (exploratory * use_exploratory)).detach()
        return actions

    def clone(self):
        pass
