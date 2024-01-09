import openai
import os, sys

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

    def get_short_conversation_result(self, prompt, max_answer_tokens, context=""):
        print("Context: " + context + " Prompt: " + prompt)

        messages = [
            {"role": "system", "content": context},
            {"role": "user", "content": prompt}
        ]

        response = self.client.chat.completions.create(
            model="gpt-3.5-turbo-1106",
            messages=messages,
            max_tokens=max_answer_tokens  # Adjust as needed
        )

        response = response.choices[0].message.content
        print("Response: " + str(response))
        return response