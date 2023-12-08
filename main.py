from PolicyTrainer import PolicyTrainer
from NnTools.Predictor import Predictor
from utils import seed_everything
from parallel_env import *
import cv2
import multiprocessing
from Policies.QPolicy import *
from Policies.PolicyWithConfidence import PolicyWithConfidence
from NnTools.BruteForce import BruteForce

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


def train(trainer: PolicyTrainer, steps_amount, callback):
    steps_counts = []

    for id in range(steps_amount):
        prev_state, next_state, actions, done = trainer.step()
        steps_counts.append(trainer.get_steps_done().mean().item())

        if callback is not None:
            callback(prev_state, next_state, actions, done)

    return sum(steps_counts) / len(steps_counts)

def train_step(policy: PolicyWithConfidence, predictor: Predictor):
    global backup

    def on_step_by_env(prev_state, next_state, actions, done):
        if not trainer.policy.is_cloned():
            pass#predictor.add_experience(prev_state, next_state, actions, done)

    policy.clean(num_envs)

    trainer = PolicyTrainer(policy, BruteForce(policy, predictor, [0, 1]))

    default_state = parallel_env.reset()
    trainer.set_environment(parallel_env, default_state)

    steps = 0

    while True:
        steps += 1
        #trainer.activate_alternatives(predictor.is_ready())

        with trainer.suppress_training():
            result = train(trainer, 400, on_step_by_env)
            print(f"Env results: {result}")

            if result > 240:
                print(f"Trained in steps {steps}")
                break

        result2 = train(trainer, 500, on_step_by_env)
        print(f"Env results at the end: {result2}")

        # if predictor.is_ready():
        #     trainer.state = default_state
        #     predictor_policy = policy.clone()
        #     trainer.policy = predictor_policy
        #     # print("Cur state: ", trainer.state)
        #     adapter = PredictorAdapter(predictor, default_state)
        #     # adapter.set_state(trainer.state)
        #     # sample_actions = predictor_policy.sample_actions(default_state)
        #     # states, rew, done, _ = parallel_env.step(sample_actions)
        #     # states2, rew2, done2, _ = adapter.step(sample_actions)
        #     # print("Env: ", states, rew, done)
        #     # print("Pred: ", states2, rew2, done2)
        #     # print("Predictor stepped")
        #
        #     trainer.push_environment(adapter)
        #     predictor_result = train(trainer, 1000, None)
        #     print(f"Predictor results: {predictor_result}")

            # trainer.pop_environment()
            # trainer.state = default_state
            # trainer.policy = policy
            # policy.copy_state_from(predictor_policy)

    return trainer.get_active_env_steps()


if __name__ == '__main__':
    multiprocessing.freeze_support()

    env_fns = [lambda: create_env('CartPole-v1') for _ in range(num_envs)]
    parallel_env = PreprocessEnv(ParallelEnv(env_fns)) #PreprocessEnv1Item(create_env('CartPole-v1'))

    dims = 4#parallel_env.observation_space.shape[0]
    actions = 2#parallel_env.action_space.n

    #print(f"State dimensions: {dims}. Actions: {actions}")
    layers = [64, 128, 64, actions]
    policy = QPolicy("policy", dims, layers)
    predictor = Predictor(actions, [1024, 256, 64, dims])

    steps_count = train_step(policy, predictor)
    print(f"Trained by simulated step count: {steps_count}")

    ev1 = create_env('CartPole-v1')
    state = ev1.reset()
    done = False

    with policy.suppress_exploratory():
        while not done:
            img = ev1.render(mode='rgb_array')
            cv2.imshow("asd", img)
            cv2.waitKey(50)
            policy.clean(1)
            act = policy.sample_actions(torch.from_numpy(state).reshape((1, dims)))
            state, _, done, _ = ev1.step(act[0].item())








