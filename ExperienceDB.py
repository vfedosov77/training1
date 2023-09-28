import torch
import random
from torch.utils.data import DataLoader, TensorDataset


class ExperienceDB:
    def __init__(self):
        self.inputs: torch.Tensor = None
        self.results: torch.Tensor = None
        self.dataset = None

    def add(self, input, result):
        if self.inputs is None:
            self.inputs = torch.clone(input)
            self.results = torch.clone(result)
            self.dataset = TensorDataset(self.inputs, self.results)
        else:
            self.inputs = torch.cat((self.inputs, input), dim=0)
            self.results = torch.cat((self.results, result), dim=0)
            self.dataset.tensors = (self.inputs, self.results)

    def get_batches(self, size) -> "iterator":
        # TODO: remove shuffle - it is not efficient
        return DataLoader(self.dataset, batch_size=size, shuffle=True)

    def is_empty(self):
        return self.inputs is None


