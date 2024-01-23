from ChatBot.AI.AiCoreBase import AiCoreBase

from transformers import AutoModelForCausalLM, AutoTokenizer, BitsAndBytesConfig, pipeline, Conversation
import torch
import os


RESOURCES_FOLDER = "/content/drive/MyDrive/temp/Resources/"
MODEL_NAME = "mistralai/Mistral-7B-Instruct-v0.2"

DEVICE = "cuda"
MAX_CONVERSATION_STEPS = 5


class MistralAiCore(AiCoreBase):
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

    def is_generation_preferred(self):
        return True

    def get_generated_text(self, prompt: str, max_answer_tokens, context) -> str:
        context = '<<SYS>>\\n' + context.strip() + '\\n<</SYS>>\\n\\n' if context else ""
        message = context + prompt
        print("Prompt: " + message)

        sequences = self.text_generation_pipe(
            message,
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
                top_p=0.8,
                pad_token_id=self.conversational_pipe.tokenizer.eos_token_id)

            answer = conversation.messages[-1]["content"]
            print("Response: " + str(answer))
            prompt = callback(i, answer)

            if prompt is None:
                return answer

            conversation.add_message({"role": "user", "content": prompt})

        raise BrokenPipeError()


