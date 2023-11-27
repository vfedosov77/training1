import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from common.utils import create_nn, create_nn_and_optimizer
from ExperienceDB import ExperienceDB


class Predictor(torch.nn.Module):
    def __init__(self, actions_count: int, layers_sizes: List[int], accepted_state_diff=0.001):
        torch.nn.Module.__init__(self)

        # Probabilities of variants
        self.future_vars = [0.0, 1.0]
        probability_layers = layers_sizes.copy()
        probability_layers[-1] = len(self.future_vars)
        probability_input_size = actions_count + layers_sizes[-1]

        self.variants_probability, self.variants_optimizer = \
            create_nn_and_optimizer(probability_input_size, probability_layers, False, 0.001)

        self.variants_loss = torch.nn.CrossEntropyLoss()

        # Predictor NN
        self.state_size = layers_sizes[-1]
        input_size = actions_count + layers_sizes[-1] + 1
        self.network = create_nn(input_size, layers_sizes, False)
        self.state_optimizer = torch.optim.Adam(self.network.parameters(), lr=0.00001)  # , betas=(0.5, 0.9))
        #self.worst_state_optimizer = torch.optim.Adam(self.network.parameters(), lr=1.0)
        self.state_loss = torch.nn.MSELoss()

        # Done NN
        self.done_network = create_nn(layers_sizes[-1], [64, 10, 1], False)
        self.done_optimizer = torch.optim.Adam(self.done_network.parameters(), lr=0.0003)
        self.done_loss = torch.nn.BCELoss()

        self.step = 0
        self.experience = ExperienceDB()
        self.tests_passed = 0
        self.accepted_state_diff = accepted_state_diff
        self.batch_size = 64

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

    def unite_state_and_actions(self, prev_states, actions):
        return torch.cat((prev_states, actions), dim=1)

    def crate_all_variants(self, state_and_actions):
        result = torch.stack([torch.cat((torch.full((len(state_and_actions), 1), var), state_and_actions), dim=1)
                              for var in self.future_vars],
                             dim=1)
        return result

    def forward(self, state_and_actions: torch.Tensor) -> (torch.Tensor, torch.Tensor):
        #probabilities = self.variants_probability(state_and_actions)
        #variants = probabilities.multinomial(1).detach()
        predicted_states: torch.Tensor = self.network(self.crate_all_variants(state_and_actions))
        #predicted_state = torch.gather(predicted_states, variants)
        done = torch.sigmoid(self.done_network(predicted_states))
        return predicted_states, done

    def _test(self, prev_states, cur_states, actions, done):
        state_and_actions = self.unite_state_and_actions(prev_states, actions)
        predicted_state, predicted_done = self(state_and_actions)
        loss = self._get_prediction_loss(predicted_state, cur_states)

        if not self.is_ready_flag and loss.item() <= self.accepted_state_diff:
            self.tests_passed += 1
            if self.tests_passed > 10:
                print("Predictor is ready")
                self.is_ready_flag = True
        else:
            self.tests_passed = 0

        print(f"Predictor: {loss}")

    def _get_variants(self, predicted_variants, variants):
        variants = variants.reshape((len(variants), 1, 1))
        variants = variants.expand(-1, -1, 4)
        result = predicted_variants.gather(1, variants)
        return result

    def _get_prediction_loss(self, predicted_variants, state):
        state = state.unsqueeze(1).detach()
        diff = torch.norm(predicted_variants - state, dim=2)
        best_variants = diff.argmin(axis=-1)
        best_predicted = self._get_variants(predicted_variants, best_variants)
        step_loss = self.state_loss(best_predicted, state)
        return step_loss

    def _train_state(self, state_and_actions: torch.Tensor, state: torch.Tensor, done: torch.Tensor):
        input = self.crate_all_variants(state_and_actions)
        predicted_variants: torch.Tensor = self.network(input)

        # Prediction with clusterization
        self.network.zero_grad()
        state3D = state.unsqueeze(1).detach()
        diff = torch.norm(predicted_variants - state3D, dim=2)
        best_variants = diff.argmin(axis=-1)

        best_predicted = self._get_variants(predicted_variants, best_variants)
        worst_predicted = self._get_variants(predicted_variants, 1 - best_variants)

        distances = torch.norm(best_predicted - worst_predicted, dim=2)
        distances_worst_target = torch.norm(worst_predicted - state3D, dim=2)
        closer_to_worst = distances_worst_target <= distances

        probabilities_variants = self.variants_probability(state_and_actions)
        best_probabilities = torch.gather(probabilities_variants, 1, best_variants.reshape((len(best_variants), 1)))

        use_worst = ((best_probabilities > 0.99) * closer_to_worst).detach()
        trained_variant = best_predicted * (~use_worst) + worst_predicted * use_worst

        step_loss = self.state_loss(trained_variant, state3D)
        step_loss.backward()
        self.state_optimizer.step()

        # Variants probabilities
        self.variants_probability.zero_grad()
        best_variants = best_variants.reshape((len(best_variants), 1))
        best_probabilities = torch.gather(probabilities_variants, 1, best_variants)
        loss_best = -torch.log(best_probabilities).mean()

        loss_best.backward()
        self.variants_optimizer.step()

        # Done
        self._train_done(state, done)

        if self.step % 500 == 0:
            print (f"Predictor loss by step: {step_loss}")

        self.step += 1

    def _train_done(self, state: torch.Tensor, done: torch.Tensor):
        self.done_network.zero_grad()
        predicted_done = torch.sigmoid(self.done_network(state.detach()))
        done_loss = self.done_loss(predicted_done, done.detach())
        done_loss.backward()
        self.done_optimizer.step()

        if self.step % 500 == 0:
            print (f"Predictor loss by done: {done_loss.item()}")

        self.step += 1
