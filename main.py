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
from utils import seed_everything
from parallel_env import *
import cv2
import multiprocessing

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

def create_env(env_name):
    env = gym.make(env_name)
    seed_everything(env)
    return env

def current_policy(state):
    actions_prob = policy_ff(state)
    action_prob = actions_prob[0].item()
    action = 0 if random.random() <= action_prob else 1
    return action

def train_step(alpha, gamma=0.99):
    global policy_ff

    policy_optim = AdamW(policy_ff.parameters(), lr=alpha)
    values_optim = AdamW(values_ff.parameters(), lr=alpha)

    for _ in tqdm(range(100)):
        states = parallel_env.reset()
        done_b = torch.zeros([num_envs, 1], dtype=torch.bool)
        reswards = torch.zeros([num_envs, 1])

        while not done_b.all():
            actions = policy_ff(states)
            log_actions = torch.log(actions + 1e-6)
            action = actions.multinomial(1).detach()
            next_states, reward, done, _ = parallel_env.step(action)

            target = (gamma * values_ff(next_states) + reward)
            advantage = (target - values_ff(states)) * ~done_b
            done_b |= done
            reswards += reward

            loss_values = (advantage * advantage).mean()
            values_optim.zero_grad()
            loss_values.backward()
            values_optim.step()

            entropy = torch.sum(log_actions * actions, dim=-1, keepdim=True) * ~done_b
            loss_policy = -advantage.detach() * log_actions.gather(1, action) - 0.001 * entropy
            loss_policy = loss_policy.mean()
            policy_optim.zero_grad()
            loss_policy.backward()
            policy_optim.step()

            states = next_states

        # zero = numpy.array([1.0, 0.1, 0.0, 0.0])
        # zero = torch.from_numpy(zero).float()
        print(reswards.mean().item())

if __name__ == '__main__':
    multiprocessing.freeze_support()

    env_fns = [lambda: create_env('CartPole-v1') for _ in range(num_envs)]
    parallel_env = PreprocessEnv(ParallelEnv(env_fns))

    dims = parallel_env.observation_space.shape[0]
    actions = parallel_env.action_space.n
    policy_ff = nn.Sequential(nn.Linear(dims, 64), nn.Linear(64, 32), nn.Linear(32, actions), nn.Softmax(dim=-1))
    values_ff = nn.Sequential(nn.Linear(dims, 64), nn.Linear(64, 32), nn.Linear(32, 1))

    print(f"State dimensions: {dims}. Actions: {actions}")

    train_step(0.0003)







