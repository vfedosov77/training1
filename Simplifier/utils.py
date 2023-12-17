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

def get_layers_vectors_cuda(module: nn.Sequential):
    for layer_id, module in enumerate(module):
        if isinstance(module, nn.Linear):
            weights: torch.Tensor = module.weight.data
            biases: torch.Tensor = module.bias.data
            neurons_count, inputs_count = weights.shape

            if inputs_count < WEIGHT_DIM:
                weights = torch.cat((weights, torch.zeros((neurons_count, WEIGHT_DIM - inputs_count)).cuda()), dim=1)

            indices = torch.Tensor([(float(layer_id) / MAX_LAYERS, float(n) / MAX_NEURONS_NUM, biases[n].item())
                                    for n in range(neurons_count)]).cuda()

            layer_data = torch.cat((indices, weights), dim=1)
            yield layer_data


def get_first_layer_input_info(first_layer: torch.Tensor, input_id: int, layer_size: int):
    first_layer_weights = [0.0, 0.0, 0.0]
    first_layer_weights.extend(first_layer[id][input_id + WEIGHTS_START_ID].item() for id in range(layer_size))
    first_layer_weights.extend(0.0 for _ in range(HIDDEN_DIM - len(first_layer_weights)))
    first_layer_biases = [0.0, 1.0, 0.0]
    first_layer_biases.extend(first_layer[id][BIAS_ID].item() for id in range(layer_size))
    first_layer_biases.extend(0.0 for _ in range(HIDDEN_DIM - len(first_layer_biases)))
    return torch.stack((torch.Tensor(first_layer_weights), torch.Tensor(first_layer_biases)))





