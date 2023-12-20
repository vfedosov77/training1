import sys, os
import torch
from torch.utils.data import DataLoader, TensorDataset, random_split


class TrainDataStorage:
    def __init__(self):
        self.inputs: torch.Tensor = None
        self.output: torch.Tensor = None
        self.dataset: TensorDataset = None

        self.input2output = dict()

    def add(self, input: torch.Tensor, output: torch.Tensor):
        if self.inputs is None:
            self.inputs = torch.clone(input)
            self.output = torch.clone(output)
            self.dataset = TensorDataset(self.inputs, self.output)
        else:
            self.inputs = torch.cat((self.inputs, input), dim=0)
            self.output = torch.cat((self.output, output), dim=0)
            self.dataset.tensors = (self.inputs, self.output)

    def get_batches(self, size) -> "iterator":
        return DataLoader(self.dataset, batch_size=size, shuffle=True)

    def __iter__(self):
        input = self.dataset.tensors[0]
        output = self.dataset.tensors[1]

        for i in range(len(input)):
            yield input[i], output[i]

    def get_all(self):
        return self.inputs, self.output

    def is_empty(self):
        return self.inputs is None

    def size(self):
        return len(self.dataset.tensors[0])

    def save(self, path: str):
        torch.save(self.dataset, path)

    def load(self, path: str):
        if not os.path.exists(path):
            return False

        self.dataset = torch.load(path)
        return True

    def to_cuda(self):
        self.dataset.tensors = [tensor.cuda() for tensor in self.dataset.tensors]

    def split(self, count_for_second):
        subset1, subset2 = random_split(self.dataset, (self.size() - count_for_second, count_for_second))

        new_storage = TrainDataStorage()
        new_storage.dataset = TensorDataset()
        new_storage.dataset.tensors = subset2[:]
        self.dataset.tensors = subset1[:]
        return new_storage
