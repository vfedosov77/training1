import torch
from typing import *
from common.utils import create_nn
from NnTools.ExperienceDB import ExperienceDB
from torch.optim.lr_scheduler import ExponentialLR


class Predictor(torch.nn.Module):
    def __init__(self, actions_count: int, layers_sizes: List[int], accepted_state_diff=0.001):
        torch.nn.Module.__init__(self)

        self.state_size = layers_sizes[-1]
        self.actions_count = actions_count

        input_size = 1 + layers_sizes[-1]

        self.network = create_nn(input_size, layers_sizes, False)
        self.done_network = create_nn(layers_sizes[-1], [128, 16, 1], False)

        lr = 0.001
        betas = (0.5, 0.9)

        # self.confidence = Confidence(input_size, layers_sizes, lr)

        self.state_optimizer = torch.optim.Adam(self.network.parameters(), lr=lr, betas=betas)
        self.done_optimizer = torch.optim.Adam(self.done_network.parameters(), lr=0.0003)
        self.scheduler = ExponentialLR(self.state_optimizer, gamma=0.98, verbose=False)

        self.step = 0
        self.experience = ExperienceDB()
        self.tests_passed = 0
        self.accepted_state_diff = accepted_state_diff
        self.batch_size = 256
        self.state_loss = torch.nn.MSELoss()
        self.done_loss = torch.nn.BCELoss()

        self.is_ready_flag = False
        #self.state_norm_coefficients = torch.from_numpy(np.array([1.0/2.4, 1.0, 1.0/0.209, 1.0])).float().detach()


    def is_ready(self):
        return self.is_ready_flag

    # def _normalize_state(self, state):
    #     return state# * self.state_norm_coefficients

    def add_experience(self,
                       prev_states: torch.Tensor,
                       cur_states: torch.Tensor,
                       actions: torch.Tensor,
                       done: torch.Tensor):
        done = done.float()
        # norm_prev = self._normalize_state(prev_states)
        # norm_actions = actions * 2.0 - 1.0
        state_and_actions = self.unite_state_and_actions(prev_states, actions)
        # cur_states_norm = self._normalize_state(cur_states)
        self.experience.add(state_and_actions, cur_states, done)

        if not self.is_ready_flag:
            if self.experience.size() >= self.batch_size * 20:
                self._train_to_get_ready()
        else:
            strange_input, strange_state, strange_done = self._get_strange(state_and_actions, cur_states, done)

            if strange_input is not None:
                it = iter(self.experience.get_batches(self.batch_size))
                for input_batch, state_batch, done_batch in (next(it), next(it)):
                    self._train(input_batch, state_batch, done_batch)
                    self._train(strange_input, strange_state, strange_done)

    def _train_to_get_ready(self):
        iterations_count = 250
        for i in range(iterations_count):
            self.scheduler.step()
            for input_batch, state_batch, done_batch in self.experience.get_batches(self.batch_size):
                self._train(input_batch, state_batch, done_batch)
        self.is_ready_flag = True

    def unite_state_and_actions(self, prev_states, actions):
        return torch.cat((prev_states, actions), dim=1)

    def predict(self, prev_states, actions):
        return self.forward(self.unite_state_and_actions(prev_states, actions))

    def forward(self, state_and_actions: torch.Tensor) -> (torch.Tensor, torch.Tensor):
        predicted_state: torch.Tensor = self.network(state_and_actions)
        done = torch.sigmoid(self.done_network(predicted_state))
        # confidence_value = self.confidence.check_state(state_and_actions)
        done = done > 0.5
        return predicted_state.detach(), done.detach()  # , confidence_value

    def find_best_action(self, state: torch.Tensor, default_action: torch.Tensor, is_state_bad_lambda, max_depth=3):
        best = -1

        for i in range(self.actions_count):
            state_and_action = torch.cat((state, torch.full((1,), i)), dim=0)
            next_state = self.network(state_and_action)
            is_bad = is_state_bad_lambda(next_state)
            is_done = self._get_done(next_state).item()

            if not is_bad and not is_done:
                if i == default_action.item():
                    return default_action

                best = i

        if best == -1:
            return default_action

        return torch.full((1,), best)

    def _train(self, input: torch.Tensor, curent_state: torch.Tensor, done: torch.Tensor):
        # Prediction
        predicted = self.network(input.detach())

        state_ok = torch.norm(predicted - curent_state, dim=1).detach() < 0.1

        self.network.zero_grad()
        step_loss = self.state_loss(predicted, curent_state.detach())
        step_loss.backward()
        self.state_optimizer.step()

        # Done
        done_ok = self._train_done(curent_state, done)

        # Confidence
        # self._train_confidence(input, state_ok.squeeze(), done_ok.squeeze())

        if self.step % 500 == 0:
            lr = self.scheduler.get_last_lr()
            print (f"Predictor loss by step: {step_loss}, by rate {lr}")

        self.step += 1

    # def _train_confidence(self, input: torch.Tensor, state_ok: torch.Tensor, done_ok: torch.Tensor):
    #     ok = done_ok * state_ok
    #
    #     if torch.any(ok):
    #         indices_ok = torch.nonzero(ok).detach()
    #         indices_ok = indices_ok.squeeze()
    #         self.confidence.reinforce_part_of_states(input, indices_ok)

    def _get_done(self, predicted: torch.Tensor):
        done = torch.sigmoid(self.done_network(predicted))
        return (done > 0.5).detach()

    def _train_done(self, state: torch.Tensor, done: torch.Tensor):
        self.done_network.zero_grad()
        predicted_done = torch.sigmoid(self.done_network(state.detach()))
        done_loss = self.done_loss(predicted_done, done.detach())
        done_loss.backward()
        self.done_optimizer.step()

        if self.step % 500 == 0:
            print (f"Predictor loss by done: {done_loss.item()}")

        return (predicted_done > 0.5) == done

    def _get_strange(self, input: torch.Tensor, curent_state: torch.Tensor, done: torch.Tensor):
        predicted, predicted_done = self.forward(input)
        diff = torch.norm((predicted - curent_state), dim=1)
        wrong_state_ids: torch.Tensor = torch.nonzero(diff > 0.1).squeeze(axis=1)
        strange_ids = set(wrong_state_ids.tolist())
        wrong_done_ids = torch.nonzero((predicted_done != done).squeeze(axis=1)).squeeze(axis=1)
        strange_ids.update(wrong_done_ids.tolist())

        if not strange_ids:
            return None, None, None

        strange_ids = list(strange_ids)
        result_shape = (len(strange_ids), -1)

        return input[strange_ids].reshape(result_shape),\
            curent_state[strange_ids].reshape(result_shape), \
            done[strange_ids].reshape(result_shape)

