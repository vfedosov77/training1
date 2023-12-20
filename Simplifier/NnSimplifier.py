import torch
import torch.nn as nn
import torchvision.models.vision_transformer as vit
from Simplifier.Constants import *


class NnSimplifier(nn.Module):
    def __init__(self,
                 sequence_size: int,
                 output_dim: int,
                 device,
                 dropout: float = 0.1,
                 attention_dropout: float = 0.1):
        nn.Module.__init__(self)

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
        self.class_token = nn.Parameter(torch.zeros(1, 1, HIDDEN_DIM))

        if device:
            self.to(device)
            self.class_token = self.class_token.cuda()

    def forward(self, input_data: torch.Tensor):
        out = self.encoder(input_data)
        out = out[:, 0, :].squeeze(dim=1)
        linear = self.linear
        out = linear(out)
        return self.softmax(out)



