import numpy as np
import torch
from typing import *


def create_nn_and_optimizer(input_size: int, layers_sizes: List, add_softmax: bool, lr: float) -> \
        Tuple[torch.nn.Sequential, torch.optim.Optimizer]:
    layers = OrderedDict()

    for id, size in enumerate(layers_sizes):
        layers["Layer_" + str(id)] = torch.nn.Linear(input_size, size)
        input_size = size

        if id != len(layers_sizes) - 1:
            layers["RLU_" + str(id)] = torch.nn.ReLU()

    if add_softmax:
        layers["Softmax"] = torch.nn.Softmax(dim=-1)

    nn = torch.nn.Sequential(layers)
    optimizer = torch.optim.Adam(nn.parameters(), lr=lr)  # Eve(nn.parameters(), lr=lr)
    return nn, optimizer