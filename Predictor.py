import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from common.utils import create_nn
from ExperienceDB import ExperienceDB


class Predictor(torch.nn.Module):
    def __init__(self, actions_count: int, layers_sizes: List[int], accepted_state_diff=0.001):
        torch.nn.Module.__init__(self)

        self.state_size = layers_sizes[-1]
        input_size = actions_count + layers_sizes[-1]

        self.network = create_nn(input_size, layers_sizes, False)
        self.done_network = create_nn(layers_sizes[-1], [64, 10, 1], False)

        self.state_optimizer = torch.optim.Adam(self.network.parameters(), lr=0.0001)#, betas=(0.5, 0.9))
        self.done_optimizer = torch.optim.Adam(self.done_network.parameters(), lr=0.0003)

        self.step = 0
        self.experience = ExperienceDB()
        self.tests_passed = 0
        self.accepted_state_diff = accepted_state_diff
        self.batch_size = 64
        self.state_loss = torch.nn.MSELoss()
        self.done_loss = torch.nn.BCELoss()

        self.is_ready_flag = False

    def is_ready(self):
        return self.is_ready_flag

    def add_experience(self,
                       prev_states: torch.Tensor,
                       cur_states: torch.Tensor,
                       actions: torch.Tensor,
                       done: torch.Tensor):
        done = done.float()

        if not self.experience.is_empty():
            self._test(prev_states, cur_states, actions, done)

        state_and_actions = self.unite_state_and_actions(prev_states, actions)

        self.experience.add(state_and_actions, cur_states, done)

        if not self.experience.is_empty():
            for input_batch, state_batch, done_batch in self.experience.get_batches(self.batch_size):
                self._train_state(input_batch, state_batch, done_batch)

                if self.is_ready_flag:
                    self._train_done(input_batch, state_batch, done_batch)

    def unite_state_and_actions(self, prev_states, actions):
        return torch.cat((prev_states, actions), dim=1)

    def forward(self, state_and_actions: torch.Tensor) -> (torch.Tensor, torch.Tensor):
        predicted_state: torch.Tensor = self.network(state_and_actions)
        done = torch.sigmoid(self.done_network(predicted_state))
        return predicted_state, done

    def _test(self, prev_states, cur_states, actions, done):
        state_and_actions = self.unite_state_and_actions(prev_states, actions)
        predicted_state, predicted_done = self(state_and_actions)
        loss = self.state_loss(predicted_state, cur_states)

        if not self.is_ready_flag and loss.item() <= self.accepted_state_diff:
            self.tests_passed += 1
            if self.tests_passed > 10:
                print("Predictor is ready")
                self.is_ready_flag = True

                for _ in range(10):
                    for input_batch, state_batch, done_batch in self.experience.get_batches(self.batch_size):
                        self._train_done(input_batch, state_batch, done_batch)
        else:
            self.tests_passed = 0

        #print(f"Predictor: {state_diff}, {predicted_done == done}")

    def _train(self, input: torch.Tensor, result: torch.Tensor):
        predicted = self.network(input.detach())
        loss = self.loss(predicted, result.detach())

        self.network.zero_grad()
        step_loss = self.state_loss(predicted_state, state.detach())
        step_loss.backward()
        self.state_optimizer.step()

        if self.step % 500 == 0:
            print (f"Predictor loss by step: {step_loss}")

        self.step += 1

    def _train_done(self, state_and_actions: torch.Tensor, state: torch.Tensor, done: torch.Tensor):
        self.done_network.zero_grad()
        predicted_done = torch.sigmoid(self.done_network(self.network(state_and_actions).detach()))
        done_loss = self.done_loss(predicted_done, done.detach())
        done_loss.backward()
        self.done_optimizer.step()

        if self.step % 500 == 0:
            print (f"Predictor loss by done: {done_loss.item()}")

        self.step += 1
