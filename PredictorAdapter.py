import torch

from Predictor import Predictor


class PredictorAdapter:
    def __init__(self, predictor: Predictor, defaultState: torch.Tensor, step_reward=1.0):
        self.defaultState = defaultState
        self.state = defaultState
        self.predictor = predictor
        self.step_reward = torch.full([self.defaultState.shape[0], 1], step_reward)

    def reset(self):
        state = self.defaultState
        return state

    def set_state(self, state: torch.Tensor):
        self.state = state

    def step(self, actions):
        result = self.predictor.predict(self.state, actions)
        state = result[:,:-1]
        done = result[:,-1] > 0.5

        self.state = (state * ~done) + (self.defaultState * done)
        reward = self.step_reward * ~done

        return self.state, reward, done, None

