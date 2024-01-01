import random

from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage

import torch
import os
import pathlib as pl
import json
from typing import Dict, List, Set

QUESTIONS_PER_REQUEST = 70
QUESTIONS_FOR_TOPICS_MAPPING = 10
MAIN_TOPICS_COUNT = 10
MAIN_TOPICS_ID = "__MAIN_TOPICS__"
QUESTIONS_FOR_MAIN_TOPICS = QUESTIONS_PER_REQUEST * 2


class QuestionsTree:
    def __init__(self, questions2files: Dict[str, str], ai_core, proj_description, storage: JSONDataStorage):
        # TODO: one to many implementation
        self.questions2files = questions2files
        self.ai_core = ai_core
        self.proj_description = proj_description
        self.storage: JSONDataStorage = storage
        self._make_tree()

    def _make_tree(self):
        count = len(self.questions2files)
        print(f"Questions count: {count}")
        questions = self._group_questions(self.questions2files)

    def _group_questions(self, questions2files: Dict[str, str]) -> Dict[str, List[str]]:
        result = self.storage.get_json(MAIN_TOPICS_ID)

        if result is not None:
            return result

        result = dict()

        all_questions = list(questions2files.keys())
        random.shuffle(all_questions)
        all_questions = all_questions[QUESTIONS_FOR_MAIN_TOPICS:]
        questions_to_distribute = set(all_questions)

        while len(result) < MAIN_TOPICS_COUNT * 2:
            questions_sample = random.sample(questions_to_distribute, QUESTIONS_PER_REQUEST)
            topics = self._get_topics(questions_sample)

            # TODO: optimize it - _get_questions_with_numbers is called for each topic
            for topic in topics:
                topics_questions = []
                too_wide = False

                for i in range(0, len(all_questions), QUESTIONS_PER_REQUEST):
                    subset = all_questions[i: i + QUESTIONS_PER_REQUEST]

                    related = self._find_questions_related_to_topic(subset, topic)
                    topics_questions.extend(related)

                    if len(related) > QUESTIONS_PER_REQUEST // 2:
                        too_wide = True
                        break

                if not too_wide and len(all_questions) // 3 > len(topics_questions) >= QUESTIONS_FOR_TOPICS_MAPPING:
                    result[topic] = topics_questions

                    for question in related:
                        questions_to_distribute.remove(question)

        self.storage.insert_json(MAIN_TOPICS_ID, result)

        print("Fond topics:")
        for topic, questions in result:
            print(topic + ": " + str(questions))

        return result

    def _get_topics(self, questions: List[str]) -> List[str]:
        def check_response(response):
            try:
                topics = [topic for topic in response.split("\n") if topic and str.isdecimal(topic[0])]
                return len(topics) <= 7
            except Exception:
                return False

        def remove_extra_info(topic):
            topic = topic.split(",")[0]
            topic = topic.split("(")[0]
            return topic

        questions_with_numbers = self._get_questions_with_numbers(questions)

        prompt = GROUP_QUESTIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers).\
            replace("[PROJECT_DESCRIPTION]", self.proj_description)

        response = self.ai_core.get_1_or_2_steps_conversation_result(prompt,
                                                                     ONLY_A_FEW_ITEMS_PROMPT,
                                                                     check_response,
                                                                     100)

        topics = [remove_extra_info(topic) for topic in response.split("\n") if topic and str.isdecimal(topic[0])]
        topics = [topic.replace(str(id + 1) + ". ", "") for id, topic in enumerate(topics)]
        print("Found topics: " + str(topics))
        return topics

    def _get_questions_with_numbers(self, questions: List[str]):
        return "".join(str(id + 1) + ". " + quest + "\n" for id, quest in enumerate(questions))

    @staticmethod
    def _get_ids(response):
        response = response.split('\n')[0].split('(')[0].replace(".", "")
        ids = [int(idx) for idx in response.split(",") if idx.strip()]
        return ids

    def _find_questions_related_to_topic(self, questions: List[str], topic):
        def check_response(response):
            try:
                self._get_ids(response)
            except Exception:
                return False

            return True

        questions_with_numbers = self._get_questions_with_numbers(questions)

        prompt = TOPICS_QUSTIONS_PROMPT.replace("[TOPIC]", topic).\
            replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers)

        response = self.ai_core.get_1_or_2_steps_conversation_result(prompt,
                                                                     ONLY_COMMA_SEPARATED_PROMPT,
                                                                     check_response,
                                                                     100)

        ids = self._get_ids(response)
        return [questions[idx - 1] for idx in ids]

    def _split_group(self, topic, topics2questions):
        questions: List[str] = topics2questions[topic]



