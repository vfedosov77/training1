import numpy as np
import torch
from abc import abstractmethod, ABCMeta
from typing import *

from overrides import overrides


class PolicyBase(metaclass=ABCMeta):
    def __init__(self, name: str):
        self.name = name
        self.last_state: torch.Tensor = None
        self.last_actions: List[int] = None
        self.gamma = 0.999
        self.done: torch.Tensor = None

    def clean(self, threads_count: int):
        if self.done:
            del self.done

        self.done = torch.Tensor([threads_count])

    def sample_actions(self, state: torch.Tensor) -> List[int]:
        self.last_state = state
        self.last_actions = self.sample_actions_impl(state)

    def set_step_reward(self, new_state: torch.Tensor, reward: torch.Tensor, done: torch.Tensor):
        reward_advantage = self.update_values(new_state, reward)
        self.update_policy_nn(new_state, reward_advantage)
        self.done |= done

    @abstractmethod
    def update_values(self, new_state: torch.Tensor, reward: torch.Tensor) -> torch.Tensor:
        pass

    @abstractmethod
    def update_policy_nn(self, new_state: torch.Tensor, reward_advantage: torch.Tensor):
        pass

    @abstractmethod
    def sample_actions_impl(self, state) -> List[int]:
        pass


class FastforwardPolicy(PolicyBase):
    def __init__(self, name: str, input_size: int, layers_sizes: List[int]):
        self.policy, self.policy_optim = self._create_nn_and_optimizer(input_size, layers_sizes, True)
        layers_sizes[-1] = 1
        self.values, self.values_optim = self._create_nn_and_optimizer(input_size, layers_sizes, False)
        PolicyBase.__init__(self, name)

    def _create_nn_and_optimizer(self, input_size: int, layers_sizes: List, add_softmax: bool) -> \
            Tuple[torch.nn.Sequential, torch.optim.Optimizer]:
        layers = OrderedDict()

        for id, size in enumerate(layers_sizes):
            layers["Layer_" + str(id)] = torch.nn.Linear(input_size, size)
            input_size = size

        if add_softmax:
            layers["Softmax"] = torch.nn.Softmax(input_size)

        nn = torch.nn.Sequential(layers)
        optimizer = torch.optim.Adam(nn.parameters())
        return nn, optimizer

    @overrides
    def update_values(self, new_state: torch.Tensor, reward: torch.Tensor) -> torch.Tensor:
        advantage = self.values(self.new_state) * self.gamma + reward - self.values(self.last_state)
        loss: torch.Tensor = advantage * advantage
        self.policy_optim.zero_grad()
        loss.backward()
        self.policy_optim.step()
        return advantage


    @overrides
    def update_policy_nn(self, new_state: torch.Tensor, reward_advantage):
        log_actions = torch.log(self.last_actions)
        entropy = torch.sum(log_actions * self.last_actions) * ~self.done
        self.policy_optim.zero_grad()
        loss: torch.Tensor = -reward_advantage.detach() * log_actions.gather(1, self.last_actions) - 0.001 * entropy
        loss.backward()
        self.policy_optim.step()

    @overrides
    def sample_actions_impl(self, state) -> List[int]:
        return self.policy(state).multinomial(1).detach()