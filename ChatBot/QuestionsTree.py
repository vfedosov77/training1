import random

from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Utils import *

import torch
import os
import pathlib as pl
import json
from typing import Dict, List, Set
import itertools

QUESTIONS_PER_REQUEST = 70
QUESTIONS_PER_REQUEST_FOR_TOPIC = 20
QUESTIONS_FOR_TOPICS_MAPPING = 10
MAIN_TOPICS_COUNT = 10
FOUND_TOPICS_ID = "__MAIN_TOPICS__"
MAIN_TOPICS_ID = "_MAIN_TOPICS_"

QUESTIONS_FOR_MAIN_TOPICS = QUESTIONS_PER_REQUEST * 2


class QuestionsTree:
    def __init__(self, questions2files: Dict[str, str], ai_core, proj_description, storage: JSONDataStorage):
        # TODO: one to many implementation
        self.questions2files = questions2files
        self.ai_core = ai_core
        self.proj_description = proj_description
        self.storage: JSONDataStorage = storage
        self._make_tree()
        self.main_topics: Dict[str, List[str]] = None

    def get_topic_for_question(self, question: str) -> str:
        topics = list(self.main_topics.keys())

        prompt = TOPIC_FOR_QUESTION_PROMPT.replace("[PROJECT_DESCRIPTION]", self.proj_description).\
            replace("[TOPICS_WITH_NUMBERS]", self._get_items_with_numbers(topics)).\
            replace("[QUESTION]", question)

        result = self.ai_core.get_generated_text(prompt, 2)

        try:
            topic_id = int(result.split(".")[0].split(",")[0])
        except ValueError:
            print("Cannot parse topic_id: " + (result if result else "None"))
            return None

        return topics[topic_id - 1] if topic_id < len(topics) else None

    def _make_tree(self):
        count = len(self.questions2files)
        print(f"Questions count: {count}")
        topics2questions = self._group_questions(self.questions2files)
        self._fill_main_topics(topics2questions)

    def _fill_main_topics(self, topics2questions):
        self.main_topics = self.storage.get_json(MAIN_TOPICS_ID)

        if self.main_topics is not None:
            return

        scores = {topic: 0 for topic in topics2questions.keys()}
        topics = list(topics2questions.keys())
        topics2own_questions = {t: [] for t in topics}
        distributed_questions = set()

        for topic, questions in topics2questions.items():
            for question in questions[:QUESTIONS_FOR_TOPICS_MAPPING]:
                random.shuffle(topics)
                topics.remove(topic)
                topics.insert(0, topic)
                self.main_topics = {t: topics2questions[t] for t in topics[:MAIN_TOPICS_COUNT]}

                detected = self.get_topic_for_question(question)

                if detected == topic:
                    scores[topic] += 1
                    topics2own_questions[topic].append(question)
                    distributed_questions.add(question)
                elif detected:
                    scores[detected] -= 1

        topics2scores = [(topic, score) for topic, score in scores.items()]
        list.sort(topics2scores, key=lambda x: x[1], reverse=True)
        self.main_topics = {topic: topics2own_questions[topic] for topic, _ in topics2scores[:MAIN_TOPICS_COUNT]}
        print("Main topics:")
        self._print_topics(self.main_topics)
        self.storage.insert_json(MAIN_TOPICS_ID, self.main_topics)

        all_questions = set(self.questions2files.keys())
        all_questions -= set(distributed_questions)

        self._distribute_questions(all_questions)

        self.storage.insert_json(MAIN_TOPICS_ID, self.main_topics)

    def _distribute_questions(self, questions):
        self.main_topics["Other"] = []

        for question in questions:
            detected = self.get_topic_for_question(question)

            if detected:
                self.main_topics[detected].append(question)
            else:
                self.main_topics["Other"].append(question)

        print("Questions are distributed:")
        self._print_topics(self.main_topics)

    def _group_questions(self, questions2files: Dict[str, str]) -> Dict[str, List[str]]:
        result = self.storage.get_json(FOUND_TOPICS_ID)
        result_required_size = MAIN_TOPICS_COUNT * 3

        if False and result is not None:
            print("Loaded topics:")
            self._print_topics(result)

            if len(result) >= result_required_size:
                return result
        else:
            result = dict()

        all_questions = list(questions2files.keys())
        random.shuffle(all_questions)
        questions_to_distribute = set(all_questions[:(QUESTIONS_FOR_MAIN_TOPICS * 2) // 3])
        all_questions = all_questions[:QUESTIONS_FOR_MAIN_TOPICS]

        while len(result) < result_required_size:
            questions_sample = random.sample(questions_to_distribute, QUESTIONS_PER_REQUEST)
            topics = self._get_topics(questions_sample)

            # TODO: optimize it - _get_items_with_numbers is called for each topic
            for topic in topics:
                topics_questions = []
                too_wide = False

                for i in range(0, len(all_questions), QUESTIONS_PER_REQUEST_FOR_TOPIC):
                    subset = all_questions[i: i + QUESTIONS_PER_REQUEST_FOR_TOPIC]

                    related = self._find_questions_related_to_topic(subset, topic)
                    topics_questions.extend(related)

                    if len(related) > QUESTIONS_PER_REQUEST_FOR_TOPIC // 2:
                        too_wide = True
                        print("Too many related items - the topic is not distinct")
                        break

                if not too_wide and (len(all_questions) // 3) > len(topics_questions) >= QUESTIONS_FOR_TOPICS_MAPPING:
                    result[topic] = topics_questions

                    for question in related:
                        if question in questions_to_distribute:
                            questions_to_distribute.remove(question)

                    self.storage.insert_json(FOUND_TOPICS_ID, result)

                    print("Found " + str(len(result)) + " topics.")

        print("Detected topics:")
        self._print_topics(result)
        return result

    def _print_topics(self, topics):
        for topic, questions in topics.items():
            print(topic + ": " + str(questions))

    def _get_topics(self, questions: List[str]) -> List[str]:
        def remove_extra_info(topic):
            topic = topic.split(",")[0]
            topic = topic.split("(")[0]
            return topic

        questions_with_numbers = self._get_items_with_numbers(questions)

        prompt = GROUP_QUESTIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers).\
            replace("[PROJECT_DESCRIPTION]", self.proj_description)

        response = self.ai_core.get_generated_text(prompt, 60)
        topics = [remove_extra_info(topic) for topic in parse_numbered_items(response)]
        print("Found topics: " + str(topics))
        return topics

    def _get_items_with_numbers(self, items: List[str]):
        return "".join(str(id + 1) + ". " + quest + "\n" for id, quest in enumerate(items))

    @staticmethod
    def _get_ids(response):
        response = response.split('\n')[0].split('(')[0].replace(".", "")
        ids = [int(idx) for idx in response.split(",") if idx.strip()]
        return ids

    def _find_questions_related_to_topic(self, questions: List[str], topic):
        questions_with_numbers = self._get_items_with_numbers(questions)

        prompt = TOPICS_QUESTIONS_PROMPT.replace("[TOPIC]", topic).\
            replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers).\
            replace("[PROJECT_DESCRIPTION]", self.proj_description)

        response = self.ai_core.get_generated_text(prompt, 50)

        ids = self._get_ids(response)
        return [questions[idx - 1] for idx in ids if idx <= len(questions)]

    def _split_group(self, topic, topics2questions):
        questions: List[str] = topics2questions[topic]



