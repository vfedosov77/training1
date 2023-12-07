import torch

from Policies.QPolicy import QPolicy
from NnTools.Predictor import Predictor
from typing import List


class BruteForce:
    def __init__(self, policy: QPolicy, predictor: Predictor, actions_set: List[int]):
        self.policy = policy
        self.predictor = predictor
        self.actions = actions_set

    def _make_policy_steps(self, state: torch.Tensor, done: torch.Tensor, n_steps: int):
        max_q_value = self.policy.get_q_value(state) * done

        with self.policy.suppress_exploratory():
            policy_state = state
            policy_done = done
            actions = self.policy.sample_actions(policy_state)
            first_actions = actions

            for i in range(n_steps):
                policy_state, next_done = self.predictor.predict(policy_state, actions)
                policy_done = policy_done | next_done
                q_value = self.policy.get_q_value(policy_state) * (~next_done)
                max_q_value = torch.max(max_q_value, q_value)[0]
                actions = self.policy.sample_actions(policy_state)

        return max_q_value, first_actions

    def _explore(self, state, done, n_steps):
        actions_shape = (len(state), 1)
        max_q = self.policy.get_q_value(state) * (~done)

        if n_steps == 0:
            return max_q

        for action in self.actions:
            action = torch.full(actions_shape, action)
            new_state, new_done = self.predictor.predict(state, action)
            new_done = new_done | done
            child_q = self._explore(new_state, new_done, n_steps - 1)
            max_q = torch.max(child_q, max_q)[0]

        return max_q

    def check_alternative(self, state: torch.Tensor, done: torch.Tensor, n_steps: int):
        policy_q, first_actions = self._make_policy_steps(state, done, n_steps)
        not_policy_state, new_done = self.predictor.predict(state, 1 - first_actions)
        new_done = new_done | done
        not_policy_q = self._explore(not_policy_state, new_done, n_steps - 1)
        use_alternative = not_policy_q > (policy_q * 1.5)
        return use_alternative





