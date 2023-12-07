import torch

from NnTools.Predictor import Predictor


class PredictorAdapter:
    def __init__(self, predictor: Predictor, defaultState: torch.Tensor, step_reward=1.0):
        self.defaultState = defaultState
        self.state = defaultState
        self.predictor = predictor
        self.step_reward = torch.full([self.defaultState.shape[0], 1], step_reward)

    def reset(self):
        self.state = self.defaultState
        return self.state

    def set_state(self, state: torch.Tensor):
        self.state = state

    def step(self, actions):
        state, done, confidence = self.predictor.predict(self.state, actions)
        self.state = state #(state * ~done) + (self.defaultState * done)
        reward = self.step_reward * ~done

        return self.state, reward, done, None

