import numpy as np
import torch
from typing import *


def create_nn_and_optimizer(input_size: int, layers_sizes: List, lr: float, add_softmax: bool = False, add_sigmoid: bool = False) -> \
        Tuple[torch.nn.Sequential, torch.optim.Optimizer]:
    layers = OrderedDict()

    for id, size in enumerate(layers_sizes):
        layers["Layer_" + str(id)] = torch.nn.Linear(input_size, size)
        input_size = size

        if id != len(layers_sizes) - 1:
            layers["RLU_" + str(id)] = torch.nn.ReLU()

    if add_softmax:
        layers["Softmax"] = torch.nn.Softmax(dim=-1)
    elif add_sigmoid:
        layers["Sigmoid"] = torch.nn.Sigmoid()

    nn = torch.nn.Sequential(layers)
    optimizer = torch.optim.Adam(nn.parameters(), lr=lr)  # Eve(nn.parameters(), lr=lr)
    return nn, optimizer


def create_nn(input_size: int, layers_sizes: List, add_softmax: bool) -> \
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
    return nn


def get_correlation(tensor1: torch.Tensor, tensor2: torch.Tensor):
    # Mean of the tensors
    mean1 = torch.mean(tensor1)
    mean2 = torch.mean(tensor2)

    # Subtract means
    tensor1_adjusted = tensor1 - mean1
    tensor2_adjusted = tensor2 - mean2

    # Calculate the correlation
    divider = torch.sqrt(torch.sum(tensor1_adjusted ** 2)) * torch.sqrt(torch.sum(tensor2_adjusted ** 2))
    correlation = torch.sum(tensor1_adjusted * tensor2_adjusted) / divider

    return correlation

def get_binary_equality(tensor1: torch.Tensor, tensor2: torch.Tensor):
    count = len(torch.nonzero((tensor1 == tensor2).squeeze(axis=1)))
    return count / len(tensor1)

