import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from common.utils import create_nn_and_optimizer
from ExperienceDB import ExperienceDB


class Predictor:
    def __init__(self, actions_count: int, layers_sizes: List[int], accepted_state_diff=0.1):
        self.state_size = layers_sizes[-1]
        input_size = actions_count + layers_sizes[-1]
        layers_sizes[-1] += 1 # done prediction
        self.network, self.optimizer = create_nn_and_optimizer(input_size, layers_sizes, False, 0.0001)
        layers_sizes[-1] = 1
        self.step = 0
        self.experience = ExperienceDB()
        self.test_passed = False
        self.accepted_state_diff = accepted_state_diff
        self.batch_size = 32
        self.loss = torch.nn.MSELoss()

    def is_ready(self):
        return self.test_passed

    def add_experience(self,
                       prev_states: torch.Tensor,
                       cur_states: torch.Tensor,
                       actions: torch.Tensor,
                       done: torch.Tensor):
        if not self.experience.is_empty():
            self._test(actions, cur_states, done, prev_states)

        input = torch.cat((prev_states, actions), dim=1)
        result = torch.cat((cur_states, done), dim=1).detach()

        self.experience.add(input, result)
        # self._train(input, result)

        if not self.experience.is_empty():
            for input_batch, results_batch in self.experience.get_batches(self.batch_size):
                self._train(input_batch, results_batch)

    def predict(self, cur_state: torch.Tensor, actions) -> (torch.Tensor, torch.Tensor):
        state_and_actions = torch.cat((cur_state, actions), dim=1)
        predicted: torch.Tensor = self.network(state_and_actions)
        return predicted[:,:self.state_size], predicted[:, self.state_size:]


    def _test(self, actions, cur_states, done, prev_states):
        predicted_state, predicted_done = self.predict(prev_states, actions)
        predicted_done = predicted_done > 0.5
        state_diff = torch.abs(predicted_state - cur_states).mean()
        self.test_passed = (state_diff <= self.accepted_state_diff and torch.all(predicted_done == done)).item()

        #print(f"Predictor: {state_diff}, {predicted_done == done}")

    def _train(self, input: torch.Tensor, result: torch.Tensor):
        predicted = self.network(input)
        loss = self.loss(predicted, result)

        self.network.zero_grad()
        loss.backward()
        self.optimizer.step()

        #if self.step % 50 == 0:
        #    print (f"Predictor loss: {loss.item()}")

        self.step += 1



