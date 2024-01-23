from ChatBot.Common.Utils import *
from ChatBot.Prompts.Prompts import *

from abc import ABCMeta, abstractmethod


class AiCoreBase(metaclass=ABCMeta):
    @abstractmethod
    def get_generated_text(self, prompt: str, max_answer_tokens, context="") -> str:
        pass

    @abstractmethod
    def get_conversation_result(self, callback, max_answer_tokens, context="") -> str:
        pass

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

            if id == 1:
                return prompt2

            return None

        self.get_conversation_result(callback, max_answer_tokens, context)
        return (answers[0], answers[1])

    def get_number_result(self, prompt, max_answer_tokens, context, less_than_val=0):
        def check_number(response):
            idx = get_idx_from_response(response)
            return idx is not None

        result = get_idx_from_response(self.get_1_or_2_steps_conversation_result(prompt,
                                     ONLY_NUMBER_PROMPT,
                                     check_number,
                                     max_answer_tokens,
                                     context))

        if result is None:
            return None

        if less_than_val != 0 and not 0 < result < less_than_val:
            return None

        return result

    def get_number_or_none_result(self, prompt, max_answer_tokens, context, less_than_val=0):
        result = get_idx_from_response(self.get_short_conversation_result(prompt, max_answer_tokens, context))

        if result is None:
            return None

        if less_than_val != 0 and not 0 < result < less_than_val:
            return None

        return result

    def get_pair_of_ids_result(self, prompt, max_answer_tokens, context):
        return get_pair_of_ids_from_response(self.get_short_conversation_result(prompt, max_answer_tokens, context))

    def get_yes_no_result(self, prompt, context):
        def check_response(response):
            response = response.lower()
            return response.startswith("yes") or \
                   response.startswith("no") or \
                   response.endswith("yes.") or \
                   response.endswith("no.")

        response = self.get_1_or_2_steps_conversation_result(prompt,
                                                           ONLY_YES_NO_PROMPT,
                                                           check_response,
                                                           2,
                                                           context)

        response = response.lower()

        if response is None:
            return None

        if response.startswith("yes") or response.endswith("yes."):
            return True

        if response.startswith("no") or response.endswith("no."):
            return False

        return None

