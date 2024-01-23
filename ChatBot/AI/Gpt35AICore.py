import openai
from ChatBot.Common.Constants import *
from ChatBot.AI.AiCoreBase import AiCoreBase


class Gpt35AICore(AiCoreBase):
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

    def get_short_question_result(self, prompt, max_answer_tokens, context="") -> str:
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

    def get_conversation_result(self, callback, max_answer_tokens, context="") -> str:
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

            if cur_len > 8000:
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


