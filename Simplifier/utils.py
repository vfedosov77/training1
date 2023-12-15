import torch
from torch import nn
from Simplifier.Constants import *


def get_layers_vectors(module: nn.Sequential):
    for layer_id, module in enumerate(module):
        if isinstance(module, nn.Linear):
            weights: torch.Tensor = module.weight.data
            biases: torch.Tensor = module.bias.data
            neurons_count, inputs_count = weights.shape

            if inputs_count < WEIGHT_DIM:
                weights = torch.cat((weights, torch.zeros((neurons_count, WEIGHT_DIM - inputs_count))), dim=1)

            indices = torch.Tensor([(float(layer_id) / MAX_LAYERS, float(n) / MAX_NEURONS_NUM, biases[n].item())
                                    for n in range(neurons_count)])

            layer_data = torch.cat((indices, weights), dim=1)
            yield layer_data
