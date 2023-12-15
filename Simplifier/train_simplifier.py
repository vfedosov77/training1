import random

import torch
import torch.nn as nn
from NnTools.TrainDataStorage import TrainDataStorage
from Simplifier.NnSimplifier import NnSimplifier
from Simplifier.Weights2FormulaStorage import Weights2FormulaStorage
from Simplifier.Constants import *
from math import *
from common.utils import *
from functools import partial

APPROX_BATH_SIZE = 128
TRANSFORMER_BATCH_SIZE = 64
INPUT_SIZE = 4
NN_LAYERS = (128, 128, 32, 10)
DB_FILE_PATH = "/tmp/simplifier_db.pth"

formula2lambda = {
    FORMULA_EMPTY: (lambda x: x),
    FORMULA_SIN: (lambda x: sin(x)),
    FORMULA_COS: (lambda x: cos(x)),
    FORMULA_LOG: (lambda x: log(x)),
    FORMULA_EXP: (lambda x: exp(x)),
}


def calculate(module: nn.Module, lambdas: List, input: torch.Tensor):
    processed_vec = torch.tensor([lambdas[i](val.item()) for i, val in enumerate(input.squeeze())]).unsqueeze(0)
    return module(processed_vec)


def create_calculating_nn_with_func(formulas: List[int]):
    distortions = create_nn(len(formulas), [32, 32, NN_LAYERS[-1]], False)
    lambdas = [formula2lambda[f] for f in formulas]
    return partial(calculate, distortions, lambdas)


def train_nn_with_formulas(formulas: List[int]):
    func = create_calculating_nn_with_func(formulas)
    network, optimizer = create_nn_and_optimizer(len(formulas), NN_LAYERS)
    loss_func = torch.nn.MSELoss()
    storage = TrainDataStorage()

    for i in range(500):
        network.zero_grad()
        input_data = torch.rand([1, len(formulas)])
        expected_output_data = func(input_data)
        storage.add(input_data, expected_output_data)

    for i in range(50):
        for inp, exp_out in storage.get_batches(APPROX_BATH_SIZE):
            output_data = network(inp)
            loss = loss_func(output_data, exp_out.detach())
            loss.backward()
            optimizer.step()
            print("Approx loss: " + str(loss))

    return network


def train_simplifier(device):
    storage = Weights2FormulaStorage()

    if not storage.load(DB_FILE_PATH):
        for i in range(500):
            formulas = [random.randint(FORMULA_EMPTY, FORMULA_MAX) for _ in range(INPUT_SIZE)]
            network = train_nn_with_formulas(formulas)
            storage.add_model(INPUT_SIZE, network, formulas)

        storage.save(DB_FILE_PATH)

    loss_func = torch.nn.CrossEntropyLoss()
    one_record = next(iter(storage.get_batches(1)))[0]
    simplifier = NnSimplifier(one_record.shape[1], device)
    optimizer = torch.optim.Adam(simplifier.parameters())

    for i in range(1000):
        for input_batch, exp_output_batch in storage.get_batches(TRANSFORMER_BATCH_SIZE):
            simplifier.zero_grad()
            out = simplifier(input_batch)
            loss = loss_func(out, exp_output_batch)
            loss.backward()
            optimizer.step()
            print("Simplifier loss: " + str(loss))



