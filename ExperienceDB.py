import torch
import random
from torch.utils.data import DataLoader, TensorDataset


class ExperienceDB:
    def __init__(self):
        self.inputs: torch.Tensor = None
        self.next_state: torch.Tensor = None
        self.done: torch.Tensor = None
        self.dataset = None

    def add(self, input, next_state, done):
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

    def get_batches(self, size) -> "iterator":
        # TODO: remove shuffle - it is not efficient
        return DataLoader(self.dataset, batch_size=size, shuffle=True)

    def is_empty(self):
        return self.inputs is None


