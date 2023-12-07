from typing import List

import torch
from Policies.PolicyBase import PolicyBase
from NnTools.DangerEstimator import DangerEstimator


class PolicyWithConfidence(PolicyBase):
    def __init__(self, policy: PolicyBase, input_size: int, layers_sizes: List[int], lr: float):
        self.bad_steps_count = 4

        self.policy = policy
        self.danger = DangerEstimator(input_size, layers_sizes)
        self.last_states = None
        PolicyBase.__init__(self, "PolicyWithConfidence")

    def activate_exploratory(self, is_active):
        self.policy.activate_exploratory(is_active)

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
        return self.danger.check_state(state)

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

        danger_training_set = None
        not_done_ids_count = 0

        if not done.all():
            not_done_ids = torch.nonzero(~done.squeeze()).squeeze(axis=1)
            danger_training_set = self.last_states[0, not_done_ids]
            not_done_ids_count = len(danger_training_set)

        if done.any():
            done_ids = torch.nonzero(done.squeeze()).squeeze(axis=1)
            bad_states = self.last_states[:, done_ids]
            bad_states = bad_states.reshape((bad_states.shape[0] * bad_states.shape[1], bad_states.shape[2]))

            if danger_training_set is None:
                danger_training_set = bad_states
            else:
                danger_training_set = torch.cat((danger_training_set, bad_states), dim=0)

        assert danger_training_set is not None
        is_dangerous = torch.Tensor([i >= not_done_ids_count for i in range(len(danger_training_set))]).\
            reshape((len(danger_training_set), 1))

        self.danger.update_states(danger_training_set, is_dangerous)

        # estimated = self.danger.check_state(danger_training_set)
        # correlation = get_binary_equality(estimated.float(), is_ok.float())
        # print("Cor: " + str(correlation))

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
