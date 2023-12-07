import torch

from Policies import PolicyWithConfidence
from NnTools.BruteForce import BruteForce


class PolicyTrainer:
    def __init__(self, policy: PolicyWithConfidence, brute_force: BruteForce):
        self.policy: PolicyWithConfidence = policy
        self.brute_force = brute_force
        self.state = None
        self.environment = None
        self.steps_done = None
        self.use_alternatives = False
        self.done = None

    def activate_alternatives(self, use_alternatives):
        self.use_alternatives = use_alternatives

    def set_environment(self, environment, state=None):
        self.state = environment.reset() if state is None else state
        self.steps_done = torch.full([self.state.shape[0], 1], 0.0)
        self.done = torch.full([self.state.shape[0], 1], False)
        self.environment = environment

    def step(self):
        actions = self.policy.sample_actions(self.state)

        if self.brute_force and self.use_alternatives:
            have_better = self.brute_force.check_alternative(self.state, self.done, 5)
            actions = actions * (~have_better) + (1 - actions) * have_better

        next_state, rewards, done, _ = self.environment.step(actions)

        self.steps_done = (self.steps_done + 1.0) * ~done
        #max_steps = max(max_steps, torch.max(steps_done).item())

        #if id % 50 == 0:
        #    print(f"max steps: {max_steps}")
        #    max_steps = 0

        well_done = self.steps_done > 450.0
        self.policy.set_step_reward(self.state, next_state, actions, rewards, done, well_done)

        prev_state = self.state
        self.state = next_state
        self.done = done
        return prev_state, next_state, actions, done

    def get_steps_done(self):
        return self.steps_done
