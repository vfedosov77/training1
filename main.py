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

from utils import test_policy_network, seed_everything, plot_stats, plot_action_probs
import cv2

env = gym.make('CartPole-v0')

dims = env.observation_space.shape[0]
actions = env.action_space.n

print(f"State dimensions: {dims}. Actions: {actions}")
print(f"Sample state: {env.reset()}")


fast_forward = nn.Sequential(nn.Linear(4, 64), nn.Linear(64, 32), nn.Linear(32, actions), nn.Softmax(dim=-1))


def get_episode(policy, gamma):
    episode = []
    state = env.reset()

    while True:
        state = torch.from_numpy(state)
        act = policy(state)
        next_state, reward, done, info = env.step(act)

        episode.append((state, act, reward))

        if done:
            return episode

        state = next_state


def create_batches(batch_size, amount, policy, gamma=0.99):
    size = batch_size * amount
    steps = []
    episodes_count = 0

    while len(steps) < size:
        items = get_episode(policy, gamma)

        G = 0.0

        for state, act, reward in reversed(items):
            G = gamma * G + reward
            steps.append((state, act, G))

            if len(steps) == size:
                break

        episodes_count += 1

    print (f"Average eposode size: {len(steps) / episodes_count}")

    random.shuffle(steps)

    batches = []

    for i in range(0, size, batch_size):
        batches.append(steps[i: i + batch_size])

    return batches



def current_policy(state):
    actions_prob = fast_forward(state)
    action_prob = actions_prob[0].item()
    action = 0 if random.random() <= action_prob else 1
    return action


def train_step(alpha, gamma=0.99):
    global fast_forward

    batches = create_batches(64, 64, current_policy, gamma)
    optim = AdamW(fast_forward.parameters(), lr=alpha)

    for batch in batches:
        optim.zero_grad()
        rewards = []
        entropias = []

        for state, action, G in batch:
            actions_probs = fast_forward(state)
            log_prob = torch.log(actions_probs)
            action_log_prob = log_prob[action]
            #gamma_t = gamma ** step
            entropy = torch.sum(log_prob * actions_probs)
            entropias.append((entropy * 0.1).reshape(1, 1))
            rewards.append((G * action_log_prob).reshape(1, 1))


        rewards = torch.sum(torch.cat(rewards))
        entropias = torch.sum(torch.cat(entropias))
        loss = -rewards #- 0.001 * entropias
        fast_forward.zero_grad()
        loss.backward()
        optim.step()
        zero = numpy.array([1.0, 0.1, 0.0, 0.0])
        zero = torch.from_numpy(zero).float()
        #print(fast_forward(zero))


for i in range(50):
    train_step(0.0003)

while True:
    state = env.reset()

    while True:
        state = torch.from_numpy(state)
        act = current_policy(state)
        next_state, reward, done, info = env.step(act)

        if done:
            state = env.reset()
            continue

        img = env.render(mode='rgb_array')
        cv2.imshow("asd", img)
        cv2.waitKey(50)

        state = next_state





