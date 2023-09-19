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
        self.confidence, self.confidence_optimizer = create_nn_and_optimizer(input_size, layers_sizes, False, 0.0001)
        self.confidence_loss_function = torch.nn.BCEWithLogitsLoss()
        self.step = 0
        self.confidence_threshold = 0.03

    def train(self, prev_state: torch.Tensor, actions: torch.Tensor, done: torch.Tensor, cur_state: torch.Tensor):
        state_and_actions = torch.cat((prev_state, actions), dim=1)
        state_and_done = torch.cat((prev_state, done), dim=1).detach()
        predicted = self.network(state_and_actions)
        diff = (predicted - state_and_done)
        squared_diff = (diff * diff)

        loss = squared_diff.mean()

        # self.network.zero_grad()
        loss.backward()
        self.optimizer.step()

        # Confidence
        target_confidence = 1.0 * (squared_diff.mean(dim=1).resize(prev_state.shape[0], 1) < self.confidence_threshold)
        cur_confidence = self.confidence(state_and_actions)
        confidence_loss = self.confidence_loss_function(cur_confidence, target_confidence)

        self.confidence.zero_grad()
        confidence_loss.backward()
        self.confidence_optimizer.step()

        if self.step % 50 == 0:
            print (f"Predictor loss: {loss.item()}, confidence loss: {confidence_loss.item()}")

        self.step += 1
