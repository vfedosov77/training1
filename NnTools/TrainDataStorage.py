import torch
from torch.utils.data import DataLoader, TensorDataset


class TrainDataStorage:
    def __init__(self):
        self.inputs: torch.Tensor = None
        self.output: torch.Tensor = None
        self.dataset = None

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

    def get_all(self):
        return self.inputs, self.output

    def is_empty(self):
        return self.inputs is None

    def size(self):
        return len(self.inputs)