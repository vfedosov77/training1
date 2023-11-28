import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from common.utils import create_nn
from ExperienceDB import ExperienceDB
from torch.optim.lr_scheduler import StepLR, ExponentialLR


class Predictor(torch.nn.Module):
    def __init__(self, actions_count: int, layers_sizes: List[int], accepted_state_diff=0.001):
        torch.nn.Module.__init__(self)

        self.state_size = layers_sizes[-1]
        input_size = actions_count + layers_sizes[-1]

        self.network = create_nn(input_size, layers_sizes, False)
        self.done_network = create_nn(layers_sizes[-1], [64, 10, 1], False)

        self.state_optimizer = torch.optim.Adam(self.network.parameters(), lr=0.001, betas=(0.5, 0.9))
        self.done_optimizer = torch.optim.Adam(self.done_network.parameters(), lr=0.0003)
        self.scheduller = ExponentialLR(self.state_optimizer, gamma=0.9, verbose=False)

        self.step = 0
        self.experience = ExperienceDB()
        self.tests_passed = 0
        self.accepted_state_diff = accepted_state_diff
        self.batch_size = 23
        self.state_loss = torch.nn.MSELoss()
        self.done_loss = torch.nn.BCELoss()

        self.is_ready_flag = False
        self.state_norm_coefficients = torch.from_numpy(np.array([1.0/2.4, 1.0, 1.0/0.209, 1.0])).float().detach()


    def is_ready(self):
        return self.is_ready_flag

    def _normalize_state(self, state):
        return state * self.state_norm_coefficients

    def add_experience(self,
                       prev_states: torch.Tensor,
                       cur_states: torch.Tensor,
                       actions: torch.Tensor,
                       done: torch.Tensor):
        done = done.float()

        #if not self.experience.is_empty():
        #    self._test(prev_states, cur_states, actions, done)
        norm_prev = self._normalize_state(prev_states)
        norm_actions = actions * 2.0 - 1.0
        state_and_actions = self.unite_state_and_actions(norm_prev, norm_actions)
        cur_states_norm = self._normalize_state(cur_states)
        self.experience.add(state_and_actions, cur_states_norm, done)

        if self.experience.size() >= self.batch_size * 500:
            for i in range(10000):
                self.scheduller.step()
                for input_batch, state_batch, done_batch in self.experience.get_batches(self.batch_size):
                    self.experience.check_if_determinate(input_batch, state_batch)
                    self._train(input_batch, state_batch, done_batch)

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
        else:
            self.tests_passed = 0

        #print(f"Predictor: {state_diff}, {predicted_done == done}")

    def _train(self, input: torch.Tensor, curent_state: torch.Tensor, done: torch.Tensor):
        # Prediction
        predicted = self.network(input.detach())

        self.network.zero_grad()
        step_loss = self.state_loss(predicted, curent_state.detach())
        step_loss.backward()
        self.state_optimizer.step()

        # Done
        self._train_done(curent_state, done)

        if self.step % 500 == 0:
            lr = self.scheduller.get_last_lr()
            print (f"Predictor loss by step: {step_loss}, by rate {lr}")

        self.step += 1

    def _train_done(self, state: torch.Tensor, done: torch.Tensor):
        self.done_network.zero_grad()
        predicted_done = torch.sigmoid(self.done_network(state.detach()))
        done_loss = self.done_loss(predicted_done, done.detach())
        done_loss.backward()
        self.done_optimizer.step()

        if self.step % 500 == 0:
            print (f"Predictor loss by done: {done_loss.item()}")
