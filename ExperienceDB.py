import torch
import random
from torch.utils.data import DataLoader, TensorDataset


class ExperienceDB:
    def __init__(self):
        self.inputs: torch.Tensor = None
        self.state: torch.Tensor = None
        self.action: torch.Tensor = None
        self.next_state: torch.Tensor = None
        self.done: torch.Tensor = None
        self.reward: torch.Tensor = None
        self.dataset = None

    def add(self, input, next_state, done):
        assert self.state is None

        if self.inputs is None:
            self.inputs = torch.clone(input)
            self.next_state = torch.clone(next_state)
            self.done = torch.clone(done)
            self.dataset = TensorDataset(self.inputs, self.next_state, self.done)
        else:
            self.inputs = torch.cat((self.inputs, input), dim=0)
            self.next_state = torch.cat((self.next_state, next_state), dim=0)
            self.done = torch.cat((self.done, done), dim=0)
            self.dataset.tensors = (self.inputs, self.next_state, self.done)

    def add_ext(self, state, action, next_state, done, reward):
        if self.next_state is None:
            self.state = torch.clone(state)
            self.action = torch.clone(action)
            self.next_state = torch.clone(next_state)
            self.done = torch.clone(done)
            self.reward = torch.clone(reward)
            self.dataset = TensorDataset(self.state, self.action, self.next_state, self.done, self.reward)
        else:
            assert self.state is not None
            self.state = torch.cat((self.state, state), dim=0)
            self.action = torch.cat((self.action, action), dim=0)
            self.next_state = torch.cat((self.next_state, next_state), dim=0)
            self.done = torch.cat((self.done, done), dim=0)
            self.reward = torch.cat((self.reward, reward), dim=0)
            self.dataset.tensors = (self.state, self.action, self.next_state, self.done, self.reward)

    def get_batches(self, size) -> "iterator":
        # TODO: remove shuffle - it is not efficient
        return DataLoader(self.dataset, batch_size=size, shuffle=True)

    def is_empty(self):
        return self.next_state is None

    def size(self):
        return len(self.next_state)

