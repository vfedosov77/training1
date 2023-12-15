from NnTools.TrainDataStorage import TrainDataStorage
import torch
from torch import nn
from Constants import *
from typing import List


class Weights2FormulaStorage(TrainDataStorage):
    def __init__(self):
        TrainDataStorage.__init__(self)

    def add_model(self, module: nn.Sequential, inputs_formulas: List[int]):
        input = None

        for layer_id, module in enumerate(module):
            if isinstance(module, nn.Linear):
                weights: torch.Tensor = module.weight.data
                biases: torch.Tensor = module.bias.data
                neurons_count, inputs_count = weights.shape

                if inputs_count < WEIGHT_DIM:
                    weights = weights.cat(weights, torch.zeros((neurons_count, WEIGHT_DIM - inputs_count)))

                indices = torch.Tensor(((float(layer_id) / MAX_LAYERS, float(n) / MAX_NEURONS_NUM, biases[n].item())
                                        for n in range(neurons_count)))

                layer_data = torch.cat((indices, weights), dim=1)

                if input is None:
                    input = layer_data
                else:
                    input = torch.cat((input, layer_data), dim=0)

        output = torch.Tensor(((formula for id, formula in enumerate(inputs_formulas)))



