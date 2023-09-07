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
from Policy import *

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

def train_step(policy: PolicyBase):
    for id in tqdm(range(10)):
        states = parallel_env.reset()
        rewards_sum = torch.zeros([num_envs, 1])
        policy.clean(num_envs)
        steps = 0

        while not policy.is_done():
            steps += 1
            actions = policy.sample_actions(states)
            next_states, rewards, done, _ = parallel_env.step(actions)
            rewards_sum += rewards.detach()

            policy.set_step_reward(next_states, rewards, done)
            states = next_states

        error = policy.get_and_reset_error()
        print("Err: " + str(error/steps))


        # zero = numpy.array([1.0, 0.1, 0.0, 0.0])
        # zero = torch.from_numpy(zero).float()
        print((rewards_sum).mean().item())

if __name__ == '__main__':
    multiprocessing.freeze_support()

    env_fns = [lambda: create_env('CartPole-v1') for _ in range(num_envs)]
    parallel_env = PreprocessEnv(ParallelEnv(env_fns))

    dims = parallel_env.observation_space.shape[0]
    actions = parallel_env.action_space.n


    print(f"State dimensions: {dims}. Actions: {actions}")
    policy = FastforwardPolicy("policy", dims, [64, 128, actions])
    train_step(policy)

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








