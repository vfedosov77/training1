from typing import List
from PolicyBase import PolicyBase
from common.utils import create_nn_and_optimizer

class PolicyWithConfidence(PolicyBase):
    def __init__(self, policy: PolicyBase, input_size: int, layers_sizes: List[int]):
        self.policy = policy
        self.nn, self.optimizer = create_nn_and_optimizer(input_size, layers_sizes, False)

        PolicyBase.__init__("PolicyWithConfidence")
