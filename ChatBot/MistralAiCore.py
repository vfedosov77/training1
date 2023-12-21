from ai_core.ai_core_base import AiCoreBase
from transformers import AutoModelForCausalLM, AutoTokenizer
import torch
import os


device = "cuda"

messages = [
    {"role": "user", "content": "What is your favourite condiment?"},
    {"role": "assistant", "content": "Well, I'm quite partial to a good squeeze of fresh lemon juice. It adds just the right amount of zesty flavour to whatever I'm cooking up in the kitchen!"},
    {"role": "user", "content": "Do you have mayonnaise recipes?"}
]

TOKENIZER_FILE = "/home/q548040/adp/ai/data/temp.bin"

class MistralAiCore(AiCoreBase):
    def __init__(self):
        AiCoreBase.__init__(self)
        self.model = AutoModelForCausalLM.from_pretrained("mistralai/Mistral-7B-Instruct-v0.2")

        if os.path.exists(TOKENIZER_FILE):
            self.tokenizer = torch.load(TOKENIZER_FILE)
        else:
            self.tokenizer = AutoTokenizer.from_pretrained("mistralai/Mistral-7B-Instruct-v0.2")

        torch.save(self.tokenizer, TOKENIZER_FILE)

    def process_request(self, request: str) -> str:
        encodeds = self.tokenizer.apply_chat_template(messages, return_tensors="pt")

        # model_inputs = encodeds.to(device)

        #for i, layer in enumerate(self.model.base_model.layers):
        #    if i < 7:
        #        layer.to(device)
        #self.model.to(device)

        generated_ids = self.model.generate(encodeds, max_new_tokens=1000, do_sample=True)
        decoded = self.tokenizer.batch_decode(generated_ids)
        print(decoded[0])
        return decoded[0]
