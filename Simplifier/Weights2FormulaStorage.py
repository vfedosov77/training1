from NnTools.TrainDataStorage import TrainDataStorage
from Simplifier.Constants import *
from Simplifier.utils import *
import torch
from torch import nn
from typing import List


class Weights2FormulaStorage(TrainDataStorage):
    def __init__(self):
        TrainDataStorage.__init__(self)

    def add_model_with_one_formula(self, module: nn.Sequential, input_id: int, input_formula: int):
        input_data = None

        for layer_data in get_layers_vectors(module):
            if input_data is None:
                input_data = layer_data[input_id].reshape((1, HIDDEN_DIM))
            else:
                input_data = torch.cat((input_data, layer_data), dim=0)

        output_data = torch.zeros((HIDDEN_DIM, 1))
        output_data[input_formula] = 1.0

        self.add(input_data, output_data)

    def add_model(self, input_size: int, module: nn.Sequential, input_formulas: List[int]):
        layers = list(get_layers_vectors_cuda(module))

        for input_id in range(input_size):
            input_id_layer = torch.full((1, HIDDEN_DIM), float(input_id)/MAX_NEURONS_NUM).cuda()
            all_tensors = [input_id_layer]
            all_tensors.extend(layers)
            input_data = torch.cat(all_tensors, dim=0)

            output_data = torch.zeros((1, HIDDEN_DIM))
            output_data[0][input_formulas[input_id]] = 1.0

            self.add(input_data.unsqueeze(0), output_data)



