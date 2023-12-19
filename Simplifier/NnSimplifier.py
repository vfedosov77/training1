import torch
import torch.nn as nn
import torchvision.models.vision_transformer as vit
from Simplifier.Constants import *


class NnSimplifier(nn.Module):
    def __init__(self,
                 sequence_size: int,
                 output_dim: int,
                 device,
                 dropout: float = 0.0,
                 attention_dropout: float = 0.0):
        self.linear = nn.Linear(HIDDEN_DIM, output_dim)
        self.softmax = torch.nn.Softmax(dim=-1)

        self.encoder = vit.Encoder(
            sequence_size,
            ENCODER_NUM_LAYERS,
            NUM_HEADS,
            HIDDEN_DIM,
            HIDDEN_DIM * COMPRESSOR_INTERNAL_FF_DIMENSION_COEEF,
            dropout,
            attention_dropout
        )

        nn.Module.__init__(self)

        if device:
            self.encoder.to(device)
            self.linear.to(device)
            self.class_token = nn.Parameter(torch.zeros(1, 1, HIDDEN_DIM).cuda())
        else:
            self.class_token = nn.Parameter(torch.zeros(1, 1, HIDDEN_DIM))

    def forward(self, input_data: torch.Tensor):
        out = self.encoder(input_data)
        out = out[:, 0, :].squeeze(dim=1)
        linear = self.linear
        out = linear(out)
        return self.softmax(out)



