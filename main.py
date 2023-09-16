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
from cv2 import imshow

num_envs = os.cpu_count()


class PreprocessEnv(ParallelWrapper):
    def __init__(self, env):
        ParallelWrapper.__init__(self, env)

    def reset(self):
        state = self.venv.reset()
        for i in range(state.shape[0]):
            state[i][0] = 0.0
            state[i][1] = 0.0

        return torch.from_numpy(state).float()

    def step_async(self, actions):
        actions = actions.squeeze().numpy()
        self.venv.step_async(actions)

    def step_wait(self):
        next_state, reward, done, info = self.venv.step_wait()

        for i in range(next_state.shape[0]):
            next_state[i][0] = 0.0
            next_state[i][1] = 0.0

        next_state = torch.from_numpy(next_state).float()
        reward = torch.tensor(reward).unsqueeze(1).float()
        done = torch.tensor(done).unsqueeze(1)
        return next_state, reward, done, info

class PreprocessEnv1Item:
    def __init__(self, env):
        self.env = env

    def reset(self):
        state = self.env.reset()
        state[0] = 0.0
        state[1] = 0.0
        state = torch.Tensor([state])
        return state

    def step(self, actions):
        action = int(actions[0].item())
        next_state, reward, done, info = self.env.step(action)
        next_state = np.array(next_state)
        next_state[0] = 0.0
        next_state[1] = 0.0
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

def train_step(policy: PolicyBase):
    global backup

    for id in tqdm(range(60)):
        states = parallel_env.reset()
        rewards_sum = torch.zeros([num_envs, 1])
        policy.clean(num_envs)
        steps = 0

        while not policy.is_done():
            steps += 1
            actions = policy.sample_actions(states)
            next_states, rewards, done, _ = parallel_env.step(actions)
            rewards = rewards * ~done.detach()
            rewards_sum += rewards.detach()

            policy.set_step_reward(next_states, rewards, done)
            states = next_states

        #next_states = torch.tensor([0.0, 0.0, 0.0, 1.0]*50).resize(50, 4)
        policy.set_fault_step(next_states)

        error = policy.get_and_reset_error()
        print("Err: " + str(error/steps))

        # zero = numpy.array([1.0, 0.1, 0.0, 0.0])
        # zero = torch.from_numpy(zero).float()
        average_reward = (rewards_sum).mean().item()
        print(average_reward)

        if id == 1:
            backup = policy.clone()
            print("!!!!!!!!!!!!!Backup!!!!!!!!!!!!")


if __name__ == '__main__':
    multiprocessing.freeze_support()

    env_fns = [lambda: create_env('CartPole-v1') for _ in range(num_envs)]
    parallel_env = PreprocessEnv(ParallelEnv(env_fns)) #PreprocessEnv1Item(create_env('CartPole-v1'))

    dims = 4#parallel_env.observation_space.shape[0]
    actions = 2#parallel_env.action_space.n

    print(f"State dimensions: {dims}. Actions: {actions}")
    policy = FastforwardPolicy("policy", dims, [64, 128, 64, actions])
    train_step(policy)

    ev1 = create_env('CartPole-v1')
    state = ev1.reset()
    done = False

    image1 = numpy.full((100, 100), 1.0)
    image2 = numpy.full((100, 100), 1.0)

    for i in range(100):
        for j in range(100):
            state = numpy.zeros([4])
            state[2] = (i - 50.0) * 0.1
            state[3] = (j - 50.0) * 0.03
            state = torch.from_numpy(state).float()
            actions1 = policy.policy(state)
            actions2 = backup.policy(state)

            if actions1[0] > 0.5:
                image1[i][j] = 0.0

            if actions2[0] > 0.5:
                image2[i][j] = 0.0

        image1[i][99] = 0.7
        image1[i][97] = 0.7
        image1[i][98] = 0.7

    cv2.imshow("1", image1)
    cv2.imshow("2", image2)
    cv2.waitKey()


    while not done:
        img = ev1.render(mode='rgb_array')
        cv2.imshow("asd", img)
        cv2.waitKey(50)
        policy.clean(1)
        act = policy.sample_actions(torch.from_numpy(state))
        state, _, done, _ = ev1.step(act[0].item())








