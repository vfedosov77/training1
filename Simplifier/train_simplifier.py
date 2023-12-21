import random

import os
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
TRANSFORMER_BATCH_SIZE = 128
INPUT_SIZE = 4
NN_LAYERS = (125, 125, 32, 10)
SIMPLIFIER_SEQUENCE = sum(NN_LAYERS) + 1

IS_LINUX = os.path.exists("/tmp")

DB_FILE_PATH = "/home/q548040/projects/training1/simplifier_db.pth" if IS_LINUX else "C:\\Users\\vfedo\\OneDrive\\Documents\\simplifier_db.pth"
SIMPLIFIER_FILE_PATH = "/home/q548040/projects/training1/simplifier.pth" if IS_LINUX else "C:\\Users\\vfedo\\OneDrive\\Documents\\simplifier.pth"
RESOURCES_FOLDER = os.environ['RESOURCES_PATH'] if 'RESOURCES_PATH' in os.environ else None

if RESOURCES_FOLDER is not None:
    DB_FILE_PATH = os.path.join(RESOURCES_FOLDER, "simplifier_db.pth")
    SIMPLIFIER_FILE_PATH = os.path.join(RESOURCES_FOLDER, "simplifier.pth")

formula2lambda = {
    FORMULA_EMPTY: (lambda x: x),
    FORMULA_SIN: (lambda x: sin(x)),
    FORMULA_COS: (lambda x: cos(x)),
    FORMULA_LOG: (lambda x: log(x)),
    FORMULA_EXP: (lambda x: exp(x)),
}


def calculate_cuda(module: nn.Module, lambdas: List, input: torch.Tensor):
    processed_vec = torch.tensor([lambdas[i](val.item()) for i, val in enumerate(input.squeeze())]).unsqueeze(0).cuda()
    return module(processed_vec)

def calculate(module, lambdas: List, input: torch.Tensor):
    processed_vec = torch.tensor([lambdas[i](val.item()) for i, val in enumerate(input.squeeze())]).unsqueeze(0)
    return module(processed_vec)

#test
#test_res = calculate(lambda x: x, [formula2lambda[1], formula2lambda[2]], torch.Tensor([[0.0, 0.0]]))
#assert test_res[0, 0].item() == 0.0
#assert test_res[0, 1].item() == 1.0

def create_calculating_nn_with_func(formulas: List[int], device):
    distortions = torch.nn.Linear(INPUT_SIZE, NN_LAYERS[-1]).cuda() #create_nn(INPUT_SIZE, [20, 20, NN_LAYERS[-1]], False, device)
    lambdas = [formula2lambda[f] for f in formulas]
    return partial(calculate, distortions, lambdas) if device is None else \
        partial(calculate_cuda, distortions, lambdas)


def train_nn_with_formulas(formulas: List[int], device):
    func = create_calculating_nn_with_func(formulas, device)
    network, optimizer = create_nn_and_optimizer(INPUT_SIZE, NN_LAYERS, device=device)
    loss_func = torch.nn.MSELoss()
    storage = TrainDataStorage()

    for i in range(500):
        input_data = torch.rand([1, INPUT_SIZE])

        if device:
            input_data = input_data.cuda()

        expected_output_data = func(input_data)
        storage.add(input_data, expected_output_data)

    for i in range(50):
        for inp, exp_out in storage.get_batches(APPROX_BATH_SIZE):
            network.zero_grad()
            output_data = network(inp)
            loss = loss_func(output_data, exp_out.detach())
            loss.backward()
            optimizer.step()

    print("Approx last loss: " + str(loss))
    return network


def fill_random_storage(device):
    storage = Weights2FormulaStorage()

    for i in range(100):
        input =  torch.rand([1, 169, 128])
        output = torch.zeros((1, 128))
        output[0][random.randint(0, 4)] = 1.0
        storage.add(input, output)

    if device:
        storage.to_cuda()

    return storage


def create_test_storage(device):
    print("Create test storage")
    storage = Weights2FormulaStorage()

    random.seed(3)

    for i in range(20):
        formulas = [random.randint(FORMULA_EMPTY, FORMULA_MAX) for _ in range(INPUT_SIZE)]
        network = train_nn_with_formulas(formulas, device)
        storage.add_model(INPUT_SIZE, network, formulas)

    # storage.load(DB_FILE_PATH)

    return storage


def test_by_db(simplifier, storage, steps: int):
    score = 0
    done = 0
    for input, output in storage:
        res = simplifier(input.unsqueeze(0)).squeeze()
        id = torch.argmax(res)
        id2 = torch.argmax(output)

        if id == id2:
            score += 1

        done += 1
        if steps == done:
            break

    print("Simplifier test result: " + str(score) + " of " + str(steps))


def train_simplifier(device):
    lr = 0.0003
    test_storage = None

    if os.path.exists(SIMPLIFIER_FILE_PATH):
        simplifier = torch.load(SIMPLIFIER_FILE_PATH)
        lr = 0.00001
        test_storage = create_test_storage(device)
        #test_by_db(simplifier, test_storage, storage.size())
        #return simplifier
    else:
        simplifier = NnSimplifier(SIMPLIFIER_SEQUENCE, FORMULA_MAX + 1, device)

    storage = Weights2FormulaStorage()

    if not storage.load(DB_FILE_PATH):
        for i in range(3000):
            formulas = [random.randint(FORMULA_EMPTY, FORMULA_MAX) for _ in range(INPUT_SIZE)]
            network = train_nn_with_formulas(formulas, device)
            storage.add_model(INPUT_SIZE, network, formulas)

            if i % 100 == 0:
                print(f"Added {i} models into DB. Saved")
                storage.save(DB_FILE_PATH)

        storage.save(DB_FILE_PATH)

    if device:
        storage.to_cuda()

    # storage = fill_random_storage(device)

    loss_func = torch.nn.MSELoss() #torch.nn.CrossEntropyLoss()
    #one_record = next(iter(storage.get_batches(1)))[0]

    # nn, opt = create_nn_and_optimizer(169*128, [100, 100, 10, 6], add_softmax=True, device=device, lr=0.001)

    if test_storage is None:
        test_storage = storage.split(100)

    optimizer = torch.optim.Adam(simplifier.parameters(), lr=lr, betas=(0.5, 0.9))
    diff = torch.nn.MSELoss()
    j = 0
    for i in range(3000):
        test_by_db(simplifier, test_storage, 100)

        for input_batch, exp_output_batch in storage.get_batches(TRANSFORMER_BATCH_SIZE):
            simplifier.zero_grad()
            out = simplifier(input_batch)
            exp_output_batch = exp_output_batch[:, 0:5]
            loss = loss_func(out, exp_output_batch.detach())
            loss.backward()
            optimizer.step()

            # nn.zero_grad()
            # exp_output_batch = exp_output_batch[:, 0:6]
            # out = nn(input_batch.reshape((len(input_batch), -1)))
            # loss = loss_func(out, exp_output_batch.detach())
            # loss.backward()
            # opt.step()

            if j % 30 == 0:
                print("Simplifier loss: " + str(diff(out, exp_output_batch)))

            j += 1

        if i % 10 == 9:
            print("Saved")
            torch.save(simplifier, SIMPLIFIER_FILE_PATH)

    return simplifier


def local_main():
    train_simplifier(torch.device("cuda") if torch.cuda.is_available() else None)




