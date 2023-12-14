import numpy as np
import torch
import io
from abc import abstractmethod, ABCMeta
from typing import *
from diagrams import *
from common.utils import create_nn, create_nn_and_optimizer
from NnTools.ExperienceDB import ExperienceDB


class ProbabilitiesDistribution(torch.nn.Module):
    def __init__(self, input_size: int, layers_sizes: List[int], accepted_state_diff=0.001):
        torch.nn.Module.__init__(self)

        # Probabilities of variants
        probability_layers = layers_sizes.copy()

        self.variants_probability, self.variants_optimizer = \
            create_nn_and_optimizer(input_size, probability_layers, False, 0.001)

        self.variants_loss = torch.nn.CrossEntropyLoss()

        # Predictor NN
        self.state_size = input_size
        self.network = create_nn(input_size, layers_sizes, False)
        self.state_optimizer = torch.optim.Adam(self.network.parameters(), lr=0.00001)  # , betas=(0.5, 0.9))
        #self.worst_state_optimizer = torch.optim.Adam(self.network.parameters(), lr=1.0)
        self.state_loss = torch.nn.MSELoss()

        self.step = 0
        self.experience = ExperienceDB()
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

    def forward(self, state_and_actions: torch.Tensor) -> torch.Tensor:
        predicted_states: torch.Tensor = self.network(self.crate_all_variants(state_and_actions))
        return predicted_states

    def get_distribution(self, state_and_actions: torch.Tensor):
        return self.variants_probability(state_and_actions).detach()

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

        self.step += 1
