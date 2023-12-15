import torch
import torch.nn as nn
import torchvision.models.vision_transformer as vit
from Simplifier.Constants import *


class NnSimplifier(nn.Module):
    def __init__(self, sequence_size: int, device, dropout: float = 0.0, attention_dropout: float = 0.0):
        nn.Module.__init__(self)
        self.encoder = vit.Encoder(
            sequence_size,
            ENCODER_NUM_LAYERS,
            NUM_HEADS,
            HIDDEN_DIM,
            HIDDEN_DIM * COMPRESSOR_INTERNAL_FF_DIMENSION_COEEF,
            dropout,
            attention_dropout
        )

        self.softmax = torch.nn.Softmax(dim=-1)

        if device:
            self.encoder.to(device)
            self.class_token = nn.Parameter(torch.zeros(1, 1, HIDDEN_DIM).cuda())
        else:
            self.class_token = nn.Parameter(torch.zeros(1, 1, HIDDEN_DIM))

    def forward(self, input_data: torch.Tensor):
        out = self.encoder(input_data)
        out = out[:,1,:].squeeze(dim=1)
        return self.softmax(out)



