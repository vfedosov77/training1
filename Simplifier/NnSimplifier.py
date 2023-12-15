import torch
import torch.nn as nn
import torchvision.models.vision_transformer as vit


class NnSimplifier:
    def __init__(self, device, dropout: float = 0.0, attention_dropout: float = 0.0):
        self.encoder = vit.Encoder(
            MAX_NEURONS_NUM,
            ENCODER_NUM_LAYERS,
            NUM_HEADS,
            HIDDEN_DIM,
            HIDDEN_DIM * COMPRESSOR_INTERNAL_FF_DIMENSION_COEEF,
            dropout,
            attention_dropout
        )

        self.encoder.to(device)
        self.class_token = nn.Parameter(torch.zeros(1, 1, HIDDEN_DIM).cuda())

    def train(self, nn: nn.Module, preprocessing_kind: torch.Tensor):


