import openai
import os, sys

from ChatBot.Utils import *
from ChatBot.Promts import *

MAX_CONVERSATION_STEPS=5


class Gpt35AICore:
    def __init__(self):
        # Key from OPENAI_API_KEY env
        self.client = openai.OpenAI()

    def is_generation_preferred(self):
        return False

    def get_generated_text(self, prompt: str, max_answer_tokens) -> str:
        print("Prompt: " + prompt)
        response = self.client.completions.create(
            model="gpt-3.5-turbo-instruct",
            prompt=prompt,
            max_tokens=max_answer_tokens
        )
        response = response.choices[0].text.strip()
        print("Response: " + str(response))
        return response

    def get_short_question_result(self, prompt, max_answer_tokens, context=""):
        print("Context: " + context + "\nPrompt: " + prompt)

        prompt = context +  "\n\nQuestion: " + prompt

        response = self.client.completions.create(
            model="gpt-3.5-turbo-instruct",
            prompt=prompt,
            max_tokens=max_answer_tokens,  # Adjust as needed
            temperature=0.0
        )

        response = response.choices[0].text.strip()
        print("Response: " + str(response))
        return response

    def get_conversation_result(self, callback, max_answer_tokens, context=""):
        print("Context: " + context)
        prompt = callback(0, None)
        cur_len = len(context)

        messages = [
            {"role": "system", "content": context},
        ]

        for i in range(1, MAX_CONVERSATION_STEPS):
            print("Prompt: " + prompt)
            cur_len += len(prompt)

            messages.append({"role": "user", "content": prompt})

            if cur_len > 10000:
                response = self.client.chat.completions.create(
                    model="gpt-3.5-turbo-1106",
                    messages=messages,
                    max_tokens=max_answer_tokens
                )
            else:
                response = self.client.chat.completions.create(
                    model="gpt-3.5-turbo",
                    messages=messages,
                    max_tokens=max_answer_tokens
                )

            answer = response.choices[0].message.content
            print("Response: " + str(answer))

            prompt = callback(i, answer)

            if prompt is None:
                return answer

            cur_len += len(answer)
            received_message = response.choices[0].message
            messages.append({"role": received_message.role, "content": received_message.content})

        raise BrokenPipeError()

    def get_short_conversation_result(self, prompt, max_answer_tokens, context=""):
        def callback(id, answer):
            return prompt if id == 0 else None

        return self.get_conversation_result(callback, max_answer_tokens, context)

    def get_1_or_2_steps_conversation_result(self,
                                             prompt1,
                                             prompt2,
                                             check_answer_callback,
                                             max_answer_tokens,
                                             context=""):
        def callback(id, answer):
            if id == 0:
                return prompt1

            if id > 1 or check_answer_callback(answer):
                return None

            return prompt2

        return self.get_conversation_result(callback, max_answer_tokens, context)

    def get_2_steps_conversation_result(self,
                                         prompt1,
                                         prompt2,
                                         max_answer_tokens,
                                         context=""):
        answers = []

        def callback(id, answer):
            answers.append(answer)

            if id == 0:
                return prompt1

            return prompt2

        self.get_conversation_result(callback, max_answer_tokens, context)
        return (answers[0], answers[1])

    def get_number_result(self, prompt, max_answer_tokens, context):
        def check_number(response):
            idx = get_idx_from_response(response)
            return idx is not None

        return self.get_1_or_2_steps_conversation_result(prompt,
                                                         ONLY_NUMBER_PROMPT,
                                                         check_number,
                                                         max_answer_tokens,
                                                         context)
