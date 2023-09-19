import os
import random

import numpy
import numpy as np
import torch
import gym
import matplotlib.pyplot as plt
from tqdm import tqdm
from torch import nn as nn
from torch.optim import AdamW

from Predictor import Predictor
from utils import seed_everything
from parallel_env import *
import cv2
import multiprocessing
from Policy import *
from cv2 import imshow

num_envs = os.cpu_count()


class PreprocessEnv(ParallelWrapper):
    def __init__(self, env):
        ParallelWrapper.__init__(self, env)

    def reset(self):
        state = self.venv.reset()
        return torch.from_numpy(state).float()

    def step_async(self, actions):
        actions = actions.squeeze().numpy()
        self.venv.step_async(actions)

    def step_wait(self):
        next_state, reward, done, info = self.venv.step_wait()
        next_state = torch.from_numpy(next_state).float()
        reward = torch.tensor(reward).unsqueeze(1).float()
        done = torch.tensor(done).unsqueeze(1)
        return next_state, reward, done, info

class PreprocessEnv1Item:
    def __init__(self, env):
        self.env = env

    def reset(self):
        state = self.env.reset()
        state = torch.Tensor([state])
        return state

    def step(self, actions):
        action = int(actions[0].item())
        next_state, reward, done, info = self.env.step(action)
        next_state = np.array(next_state)
        next_state = torch.Tensor([next_state])
        reward_tensor = torch.Tensor([[reward]])
        done_tensor = torch.zeros([1, 1], dtype=torch.bool)
        done_tensor[0] = done
        return next_state, reward_tensor, done_tensor, info


def create_env(env_name):
    env = gym.make(env_name)
    seed_everything(env)
    return env

backup = None

def train_step(policy: PolicyBase, predictor: Predictor):
    global backup
    policy.clean(num_envs)
    states = parallel_env.reset()

    steps_done = torch.full([num_envs, 1], 0)
    max_steps = 0

    for id in tqdm(range(2000)):
        actions = policy.sample_actions(states)
        next_states, rewards, done, _ = parallel_env.step(actions)

        steps_done = (steps_done + 1) * ~done
        max_steps = max(max_steps, torch.max(steps_done).item())

        if id % 50 == 0:
            print(f"max steps: {max_steps}")
            max_steps = 0

        well_done = steps_done > 498
        policy.set_step_reward(next_states, rewards, done, well_done)
        predictor.train(states, actions, done, next_states)

        states = next_states


if __name__ == '__main__':
    multiprocessing.freeze_support()

    env_fns = [lambda: create_env('CartPole-v1') for _ in range(num_envs)]
    parallel_env = PreprocessEnv(ParallelEnv(env_fns)) #PreprocessEnv1Item(create_env('CartPole-v1'))

    dims = 4#parallel_env.observation_space.shape[0]
    actions = 2#parallel_env.action_space.n

    print(f"State dimensions: {dims}. Actions: {actions}")
    policy = FastforwardPolicy("policy", dims, [64, 128, 64, actions])
    predictor = Predictor(1, [64, 128, 64, dims])

    train_step(policy, predictor)

    ev1 = create_env('CartPole-v1')
    state = ev1.reset()
    done = False

    while not done:
        img = ev1.render(mode='rgb_array')
        cv2.imshow("asd", img)
        cv2.waitKey(50)
        policy.clean(1)
        act = policy.sample_actions(torch.from_numpy(state))
        state, _, done, _ = ev1.step(act[0].item())








