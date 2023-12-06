from typing import List

import torch

from Policies.PolicyBase import PolicyBase

from common.utils import create_nn_and_optimizer
from common.Confidence import Confidence
from collections import deque

class PolicyWithConfidence(PolicyBase):
    def __init__(self, policy: PolicyBase, input_size: int, layers_sizes: List[int], lr: float):
        self.bad_steps_count = 2

        self.policy = policy
        self.bad_state_confidence = Confidence(input_size, layers_sizes, lr)
        self.last_states = None
        PolicyBase.__init__(self, "PolicyWithConfidence")

    def clean(self, threads_count: int):
        PolicyBase.clean(self, threads_count)
        self.policy.clean(threads_count)


    def sample_actions(self, state: torch.Tensor) -> List[int]:
        return self.policy.sample_actions(state)

    def set_step_reward(self,
                        prev_state: torch.Tensor,
                        new_state: torch.Tensor,
                        actions: torch.Tensor,
                        reward: torch.Tensor,
                        done: torch.Tensor,
                        well_done: torch.Tensor):
        PolicyBase.set_step_reward(self, prev_state, new_state, actions, reward, done, well_done)
        self.policy.set_step_reward(prev_state, new_state, actions, reward, done, well_done)
        self._update_confidence(done, prev_state, well_done)

    def is_state_bad(self, state: torch.Tensor):
        return self.bad_state_confidence.check_state(state)

    def _update_confidence(self, done, prev_state, well_done):
        done = done & (~well_done)
        prev_state_3d = prev_state.unsqueeze(0)

        if self.last_states is None:
            self.last_states = torch.clone(prev_state_3d)
        else:
            if len(self.last_states) >= self.bad_steps_count:
                self.last_states = torch.cat((self.last_states[1:], prev_state_3d), dim=0)
            else:
                self.last_states = torch.cat((self.last_states, prev_state_3d), dim=0)

        if done.any():
            done_ids = torch.nonzero(done.squeeze() == True).squeeze(axis=1)
            bad_states = self.last_states[:, done_ids]
            bad_states = bad_states.reshape((bad_states.shape[0] * bad_states.shape[1], bad_states.shape[2]))
            self.bad_state_confidence.reinforce_state(bad_states)

    def _update_values(self,
                       prev_state: torch.Tensor,
                       actions: torch.Tensor,
                       new_state: torch.Tensor,
                       reward: torch.Tensor,
                       done: torch.Tensor,
                       well_done: torch.Tensor) -> torch.Tensor:
        pass

    def _update_policy_nn(self,
                          prev_state: torch.Tensor,
                          new_state: torch.Tensor,
                          actions: torch.Tensor,
                          reward_advantage: torch.Tensor):
        pass

    def _sample_actions_impl(self, state) -> torch.Tensor:
        pass
