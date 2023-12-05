import torch

from Policies import PolicyWithConfidence

class PolicyTrainer:
    def __init__(self, policy: PolicyWithConfidence, get_alternative_lambda):
        self.policy: PolicyWithConfidence = policy
        self.state = None
        self.environment = None
        self.envs_stack = []
        self.steps_done = None
        self.get_alternative_lambda = get_alternative_lambda

    def push_environment(self, new_environment, state = None):
        if self.environment is not None:
            self.envs_stack.append((self.environment, torch.clone(self.state), torch.clone(self.steps_done)))

        if self.state is None:
            self.state = new_environment.reset() if state is None else state
            self.steps_done = torch.full([self.state.shape[0], 1], 0.0)
        else:
            # Real environment does not need that
            new_environment.set_state(self.state)

        self.environment = new_environment

    def pop_environment(self):
        self.environment, self.state, self.steps_done = self.envs_stack.pop()

    def step(self):
        actions = self.policy.sample_actions(self.state)

        if self.get_alternative_lambda is not None:
            is_bad = self.policy.is_state_bad(self.state)
            if is_bad.any():
                actions = self._try_alternative(is_bad, actions)

        next_state, rewards, done, _ = self.environment.step(actions)

        self.steps_done = (self.steps_done + 1.0) * ~done
        #max_steps = max(max_steps, torch.max(steps_done).item())

        #if id % 50 == 0:
        #    print(f"max steps: {max_steps}")
        #    max_steps = 0

        well_done = self.steps_done > 498.0
        self.policy.set_step_reward(self.state, next_state, actions, rewards, done, well_done)

        prev_state = self.state
        self.state = next_state
        return prev_state, next_state, actions, done

    def _try_alternative(self, is_bad, actions):
        alt_actions = self.get_alternative_lambda(self.state, is_bad, actions)
        return alt_actions

    def get_steps_done(self):
        return self.steps_done