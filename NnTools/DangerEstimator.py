from typing import List

import torch

from common.utils import create_nn_and_optimizer, create_nn


class DangerEstimator:
    def __init__(self, input_size: int, main_nn_layers_sizes: List[int]):
        layers = list.copy(main_nn_layers_sizes)
        layers[-1] = 1
        self.nn, self.optimizer = create_nn_and_optimizer(input_size, layers, 0.001, add_sigmoid=True)
        self.state_loss = torch.nn.BCELoss()

    def update_states(self, state: torch.Tensor, is_dangerous: torch.Tensor):
        value = self.nn(state)
        self.nn.zero_grad()
        step_loss = self.state_loss(value, is_dangerous.detach())
        step_loss.backward()
        self.optimizer.step()

    def check_state(self, state: torch.Tensor):
        value = self.nn(state)
        return value > 0.5



