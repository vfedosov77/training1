from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig, pipeline, AutoConfig

import torch
import os

RESOURCES_FOLDER = "/content/drive/MyDrive/temp/Resources/"
MODEL_NAME = "mistralai/Mistral-7B-Instruct-v0.2"

DEVICE = "cuda"
TAG_PREFIX = "<|im_start|>"
TAG_SUFFIX = "<|im_end|>\n"
SYS_FORMAT = TAG_PREFIX + "system\n{}" + TAG_SUFFIX
USER_FORMAT = TAG_PREFIX + "user\n{}" + TAG_SUFFIX
ASSIST_FORMAT = TAG_PREFIX + "assistant\n"
INPUT_TEXT_FORMAT = SYS_FORMAT + USER_FORMAT + ASSIST_FORMAT


class MistralAiCore:
    def __init__(self):
        bnb_config = BitsAndBytesConfig(
            load_in_4bit=True,
            bnb_4bit_quant_type="nf4",
            bnb_4bit_use_double_quant=True,
        )

        self.tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, cache_dir=RESOURCES_FOLDER)
        self.model = AutoModelForCausalLM.from_pretrained(
            MODEL_NAME,
            cache_dir=RESOURCES_FOLDER,
            load_in_4bit=True,
            quantization_config=bnb_config,
            torch_dtype=torch.bfloat16,
            device_map="auto",
            trust_remote_code=True)

        # self.config = AutoConfig.from_pretrained(MODEL_NAME, cache_dir=RESOURCES_FOLDER)
        # print("Window: " + str(config.max_position_embeddings))

        self.pipe = pipeline(
            "text-generation",
            model=self.model,
            tokenizer=self.tokenizer,
            torch_dtype=torch.bfloat16,
            device_map="auto"
        )

    def get_response(self, sys_prompt: str, user_prompt: str) -> str:
        prompt = self._create_message(sys_prompt, user_prompt)
        print("Prompt: " + prompt)

        sequences = self.pipe(
            prompt,
            do_sample=True,
            max_new_tokens=100,
            temperature=0.7,
            top_k=50,
            top_p=0.95,
            num_return_sequences=1,
        )
        response = sequences[0]['generated_text']
        print("Response: " + response)
        return response

    @staticmethod
    def _create_message(sys_prompt: str, user_prompt: str):
        return INPUT_TEXT_FORMAT.format(sys_prompt, user_prompt)