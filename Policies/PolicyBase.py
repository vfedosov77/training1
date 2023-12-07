import numpy as np
import torch
from abc import abstractmethod, ABCMeta
from typing import *


class PolicyBase(metaclass=ABCMeta):
    def __init__(self, name: str):
        self.name = name
        self.gamma = 0.999
        self.threads_count = -1
        self.step = 0
        self.cloned = False

    def activate_exploratory(self, is_active: bool):
        pass

    def is_cloned(self):
        return self.cloned

    def clean(self, threads_count: int):
        self.threads_count = threads_count
        self.step = 0

    def sample_actions(self, state: torch.Tensor) -> List[int]:
        return self._sample_actions_impl(state)

    def set_step_reward(self,
                        prev_state: torch.Tensor,
                        new_state: torch.Tensor,
                        actions: torch.Tensor,
                        reward: torch.Tensor,
                        done: torch.Tensor,
                        well_done: torch.Tensor):
        reward_advantage = self._update_values(prev_state, actions, new_state, reward, done, well_done)
        self._update_policy_nn(prev_state, new_state, actions, reward_advantage)

        self.step += 1

    @abstractmethod
    def _update_values(self,
                       prev_state: torch.Tensor,
                       actions: torch.Tensor,
                       new_state: torch.Tensor,
                       reward: torch.Tensor,
                       done: torch.Tensor,
                       well_done: torch.Tensor) -> torch.Tensor:
        pass

    @abstractmethod
    def _update_policy_nn(self,
                          prev_state: torch.Tensor,
                          new_state: torch.Tensor,
                          actions: torch.Tensor,
                          reward_advantage: torch.Tensor):
        pass

    @abstractmethod
    def _sample_actions_impl(self, state) -> torch.Tensor:
        pass

    def clone(self):
        raise NotImplementedError()

    def copy_state_from(self, policy: "PolicyBase"):
        raise NotImplementedError()