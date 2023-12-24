from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig, pipeline, Conversation

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
MAX_CONVERSATION_STEPS = 5

class MistralAiCore:
    def __init__(self):
        bnb_config = BitsAndBytesConfig(
            load_in_8bit=True
        )

        self.tokenizer = AutoTokenizer.from_pretrained(MODEL_NAME, cache_dir=RESOURCES_FOLDER)
        self.model = AutoModelForCausalLM.from_pretrained(
            MODEL_NAME,
            cache_dir=RESOURCES_FOLDER,
            load_in_8bit=True,
            quantization_config=bnb_config,
            torch_dtype=torch.bfloat16,
            device_map="auto",
            trust_remote_code=True)

        # self.config = AutoConfig.from_pretrained(MODEL_NAME, cache_dir=RESOURCES_FOLDER)
        # print("Window: " + str(config.max_position_embeddings))

        self.text_generation_pipe = pipeline(
            "text-generation",
            model=self.model,
            tokenizer=self.tokenizer,
            torch_dtype=torch.bfloat16,
            device_map="auto"
        )

        self.conversational_pipe = pipeline(
            "conversational",
            model=self.model,
            tokenizer=self.tokenizer,
            torch_dtype=torch.bfloat16,
            device_map="auto"
        )

    def get_generated_text(self, prompt: str, max_answer_tokens) -> str:
        print("Prompt: " + prompt)

        sequences = self.text_generation_pipe(
            prompt,
            do_sample=False,
            max_new_tokens=max_answer_tokens,
            temperature=0.3,
            top_k=20,
            top_p=0.8,
            num_return_sequences=1,
        )
        response = sequences[0]['generated_text']
        generated_text_start = response.find(prompt) + len(prompt)
        response = response[generated_text_start:]
        response = response.replace("\n\n", "\n")
        print("Response: " + str(response))
        return response

    def get_conversation_result(self, callback, max_answer_tokens):
        conversation = Conversation(callback(0, None))

        for i in range(1, MAX_CONVERSATION_STEPS):
            print("Prompt: " + conversation.messages[-1]["content"])

            conversation = self.conversational_pipe(
                conversation,
                do_sample=False,
                max_new_tokens=max_answer_tokens,
                temperature=0.3,
                top_k=20,
                top_p=0.8)

            answer = conversation.messages[-1]["content"]
            print("Response: " + str(answer))
            prompt = callback(i, answer)

            if prompt is None:
                return answer

            conversation.add_message({"role": "user", "content": prompt})

        raise BrokenPipeError()

    def get_short_conversation_result(self, prompt, max_answer_tokens):
        def callback(id, answer):
            return prompt if id == 0 else None

        return self.get_conversation_result(callback, max_answer_tokens)

    def get_1_or_2_steps_conversation_result(self, prompt1, prompt2, check_answer_callback, max_answer_tokens):
        def callback(id, answer):
            if id == 0:
                return prompt1

            if check_answer_callback(answer):
                return None

            return prompt2

        return self.get_conversation_result(callback, max_answer_tokens)
