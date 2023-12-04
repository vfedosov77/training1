from typing import List

import torch

from common.utils import create_nn_and_optimizer


class Confidence:
    def __init__(self, input_size: int, main_nn_layers_sizes: List[int], main_nn_lr: float):
        self.threshold = 0.1
        output_size = 10
        confidence_layers = list.copy(main_nn_layers_sizes)
        confidence_layers[-1] = output_size
        self.nn, self.optimizer = create_nn_and_optimizer(input_size, confidence_layers, False, main_nn_lr)
        self.pivot = create_nn(input_size, layers_sizes, False)

    def reinforce_part_of_states(self, self, state: torch.Tensor, is_ok: torch.Tensor):
        indices_ok = torch.nonzero(is_ok.squeeze()).squeeze()
        input_filtered = input[indices_ok].detach()
        self.reinforce_state(input_filtered)

    def reinforce_state(self, state: torch.Tensor):
        value = self.confidence(state)

        self.confidence.zero_grad()
        step_loss = self.state_loss(value, self.pivot(state).detach())
        step_loss.backward()
        self.confidence_optimizer.step()

    def check_state(self, state: torch.Tensor):
        value = self.confidence(state)
        return (torch.norm(value - self.pivot(state), dim=1) < self.threshold).squeeze().detach()



