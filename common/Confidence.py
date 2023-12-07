from typing import List

import torch

from common.utils import create_nn_and_optimizer, create_nn


class Confidence:
    def __init__(self, input_size: int, main_nn_layers_sizes: List[int], main_nn_lr: float):
        self.threshold = 0.1
        output_size = 10
        confidence_layers = list.copy(main_nn_layers_sizes)
        confidence_layers[-1] = output_size
        self.nn, self.optimizer = create_nn_and_optimizer(input_size, confidence_layers, main_nn_lr)
        self.pivot = create_nn(input_size, confidence_layers, False)
        self.state_loss = torch.nn.MSELoss()

    def reinforce_part_of_states(self, state: torch.Tensor, is_ok: torch.Tensor):
        indices_ok = torch.nonzero(is_ok.squeeze()).squeeze(axis=1)
        input_filtered = state[indices_ok].detach()
        self.reinforce_state(input_filtered)

    def reinforce_state(self, state: torch.Tensor):
        value = self.nn(state)

        self.nn.zero_grad()
        step_loss = self.state_loss(value, self.pivot(state).detach())
        step_loss.backward()
        self.optimizer.step()

    def check_state(self, state: torch.Tensor):
        value = self.nn(state)
        pivot = self.pivot(state)
        threshold = self.threshold * torch.norm(value, dim=1)
        return (torch.norm(value - pivot, dim=1) < threshold).squeeze().detach()



