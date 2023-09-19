import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from common.utils import create_nn_and_optimizer


class Predictor:
    def __init__(self, actions_count: int, layers_sizes: List[int]):
        input_size = actions_count + layers_sizes[-1]
        layers_sizes[-1] += 1 # done prediction
        self.network, self.optimizer = create_nn_and_optimizer(input_size, layers_sizes, False, 0.0001)
        layers_sizes[-1] = 1
        self.step = 0

    def train(self, prev_state: torch.Tensor, actions: torch.Tensor, done: torch.Tensor, cur_state: torch.Tensor):
        state_and_actions = torch.cat((prev_state, actions), dim=1)
        state_and_done = torch.cat((cur_state, done), dim=1).detach()
        predicted = self.network(state_and_actions)
        diff = (predicted - state_and_done)
        squared_diff = (diff * diff)

        loss = squared_diff.mean()

        # self.network.zero_grad()
        loss.backward()
        self.optimizer.step()

        if self.step % 50 == 0:
            print (f"Predictor loss: {loss.item()}")

        self.step += 1
