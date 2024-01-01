import random

from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage

import torch
import os
import pathlib as pl
import json
from typing import Dict, List

QUESTIONS_PER_REQUEST = 80


class QuestionsTree:
    def __init__(self, questions2files: Dict[str, str], ai_core):
        # TODO: one to many implementation
        self.questions2files = questions2files
        self.ai_core = ai_core
        self._make_tree()

    def _make_tree(self):
        count = len(self.questions2files)
        print(f"Questions count: {count}")
        self._group_questions(self.questions2files)

    def _group_questions(self, questions2files: Dict[str, str]) -> Dict[str, List[str]]:
        result = dict()

        all_questions = list(questions2files.keys())
        questions_to_distribute = list(questions2files.keys())

        while len(questions_to_distribute) > QUESTIONS_PER_REQUEST:
            questions_sample = random.sample(questions_to_distribute, QUESTIONS_PER_REQUEST)
            topics = self._get_topics(questions_sample)

            # TODO: optimize it - _get_questions_with_numbers is called for each topic
            for topic in topics:
                for i in range(0, len(all_questions), QUESTIONS_PER_REQUEST):
                    subset = all_questions[i: i + QUESTIONS_PER_REQUEST]

                    related = self._find_questions_related_to_topic(subset, topic)
                    result[topic] = related

        return result

    def _get_topics(self, questions: List[str]) -> List[str]:
        questions_with_numbers = self._get_questions_with_numbers(questions)
        prompt = GROUP_QUESTIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers)
        response = self.ai_core.get_short_conversation_result(prompt, 300)
        topics = [topic for topic in response.split("\n") if topic and str.isdecimal(topic[0])]
        topics = [topic.replace(str(id + 1) + ". ", "") for id, topic in enumerate(topics)]
        print("Found topics: " + str(topics))
        return topics

    def _get_questions_with_numbers(self, questions: List[str]):
        return "".join(str(id + 1) + ". " + quest + "\n" for id, quest in enumerate(questions))

    def _find_questions_related_to_topic(self, questions: List[str], topic):
        questions_with_numbers = self._get_questions_with_numbers(questions)

        prompt = TOPICS_QUSTIONS_PROMPT.replace("[TOPIC]", topic).\
            replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers)

        response = self.ai_core.get_short_conversation_result(prompt, 300)
        ids = [int(idx) for idx in response.split(",") if idx.strip()]
        return [questions[idx] for idx in ids]

    def _split_group(self, topic, topics2questions):
        questions: List[str] = topics2questions[topic]



