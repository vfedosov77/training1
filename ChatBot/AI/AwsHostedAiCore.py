from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.Common.Constants import *

import os
import json
import boto3

"""{% for message in messages %}
    {% if message['role'] == 'user' %}
        {{ bos_token + '[INST] ' + message['content'].strip() + ' [/INST]' }}
    {% elif message['role'] == 'system' %}
        {{ '<<SYS>>\\n' + message['content'].strip() + '\\n<</SYS>>\\n\\n' }}
    {% elif message['role'] == 'assistant' %}
        {{ '[ASST] '  + message['content'] + ' [/ASST]' + eos_token }}
    {% endif %}
{% endfor %}"""


# Now it is implemented for Mistral 7B
class AwsHostedAiCore(AiCoreBase):
    def __init__(self):
        aws_access_key = os.environ["AWS_ACCESS_KEY"]
        aws_secret_access_key = os.environ["AWS_SECRET_ACCESS_KEY"]

        self.client = boto3.client('runtime.sagemaker',
                                   region_name='eu-west-2',
                                   aws_access_key_id=aws_access_key,
                                   aws_secret_access_key=aws_secret_access_key)

    @staticmethod
    def _messages_to_prompt(messages):
        prompt = ""

        for message in messages:
            if message['role'] == 'user':
                prompt += "<s>[INST] " + message['content'].strip() + " [/INST]"
            elif message['role'] == 'system':
                prompt += "<<SYS>>\n" + message['content'].strip() + '\n<</SYS>>\n\n'
            elif message['role'] == 'assistant':
                prompt += "[ASST] " + message['content'] + " [/ASST]</s>"

        return prompt

    def get_conversation_result(self, callback, max_answer_tokens, context=""):
        print("Context: " + context)
        prompt = callback(0, None)

        messages = [
            {"role": "system", "content": context},
        ]

        for i in range(1, MAX_CONVERSATION_STEPS):
            print("Prompt: " + prompt)
            messages.append({"role": "user", "content": prompt})

            answer = self._process_text_request(self._messages_to_prompt(messages), max_answer_tokens)

            print("Response: " + str(answer))
            prompt = callback(i, answer)

            if prompt is None:
                return answer

            messages.append({"role": "assistant", "content": answer})

    def _process_text_request(self, text, max_length):
        payload = {
            "inputs": text,
            "parameters": {
                "do_sample": False,
                "max_length": max_length
            }}

        serialized_payload = json.dumps(payload).encode('utf-8')

        response = self.client.invoke_endpoint(
            EndpointName='mistralinstructknowlegebase',
            ContentType='application/json',
            Body=serialized_payload,
        )

        message_chunks = []
        while True:
            chunk = response['Body'].read(1024)  # Read in chunks of 1024 bytes
            if not chunk:
                break
            message_chunks.append(chunk)
        message = b''.join(message_chunks).decode('utf-8')

        #message = response['Body'].read().decode('utf-8')
        return message

    def _process_request(self, context, prompt, max_length, text_generation):
        context = '<<SYS>>\n' + context.strip() + '\n<</SYS>>\n\n' if context else ""
        prompt = prompt if text_generation else f"<s>[INST] {prompt} [/INST]</s>"
        message = context + prompt
        return self._process_text_request(message, max_length)

    def get_short_conversation_result(self, prompt, max_answer_tokens, context=""):
        return self._process_request(context, prompt, max_answer_tokens, False)

    def is_generation_preferred(self):
        return True

    def get_generated_text(self, prompt: str, max_answer_tokens, context="") -> str:
        print("Prompt: " + prompt)
        response = self._process_request(context, prompt, max_answer_tokens, True)
        print("Response: " + str(response))
        return response
