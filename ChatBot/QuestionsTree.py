import random

from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Utils import *

import torch
import os
import pathlib as pl
import json
from typing import Dict, List, Set
from ChatBot.Constants import *


QUESTIONS_PER_REQUEST = 70
QUESTIONS_PER_REQUEST_FOR_TOPIC = 20
QUESTIONS_FOR_TOPICS_MAPPING = 10
MAIN_TOPICS_COUNT = 10


QUESTIONS_FOR_MAIN_TOPICS = QUESTIONS_PER_REQUEST * 2


class QuestionsTree:
    def __init__(self, questions2files: Dict[str, str], ai_core, proj_description, storage: JSONDataStorage, callback):
        # TODO: one to many implementation
        self.questions2files = questions2files
        self.ai_core = ai_core
        self.proj_description = proj_description
        self.storage: JSONDataStorage = storage
        self.main_topics: Dict[str, List[str]] = None
        self.callback = callback
        self._fill_topics()
        self._make_tree()
        self._fill_questions2files()
        self.checked_files = set()

    def get_answer(self, question: str, chat_history = None):
        try:
            self.checked_files.clear()
            return self._get_answer(question, self.main_topics)
        finally:
            self.checked_files.clear()

    def _get_corresponding_item(self, question, items, items_count):
        context = WHICH_TOPIC_IS_CLOSEST_CONTEXT.replace("[PROJECT_DESCRIPTION]", self.proj_description)
        prompt = WHICH_TOPIC_IS_CLOSEST_PROMT.replace("[QUESTION]", question). \
            replace("[TOPICS_WITH_NUMBERS]", items)

        self._on_step(f"Find corresponding topic from {items_count} items.", prompt)
        result = self.ai_core.get_short_conversation_result(prompt, 2, context)
        idx = get_idx_from_response(result)
        self._on_step(f"Selected item with id={idx}.", None)
        return idx

    def _get_answer(self, question: str, topics_dict: dict, ignore_topic = None):
        topics = [t for t in topics_dict.keys() if t != ignore_topic]
        topic_id = self._get_corresponding_item(question, get_items_with_numbers(topics), len(topics))
        topic = topics[topic_id - 1]
        questions = topics_dict[topic]

        answer = None

        if isinstance(questions, dict):
            answer = self._get_answer(question, questions, None)
        else:
            questions = [q for q in questions if self.questions2files[q] not in self.checked_files]

            while len(questions) > 0:
                cur_questions = questions[:10]
                question_id = self._get_corresponding_item(question, get_items_with_numbers(cur_questions), len(cur_questions))

                if question_id is not None and question_id < len(cur_questions):
                    cur_question = cur_questions[question_id - 1]

                    result, path = self.check_file(self.questions2files[cur_question], question)

                    if result:
                        answer = (result, path)
                        print("Answer: " + result + " File: " + path)
                        break

                cur_questions = set(cur_questions)
                questions = [q for q in questions
                             if q not in cur_questions and self.questions2files[q] not in self.checked_files]

        if answer is None and ignore_topic is None:
            return self._get_answer(question, topics_dict, topic)

        return answer

    def _on_step(self, short_name, description, kind=NORMAL_TEXT):
        if self.callback:
            self.callback(short_name, description, kind)

    def _get_topic_for_question(self, question: str, topics_dict: dict) -> str:
        topics = list(topics_dict.keys())

        prompt = TOPIC_FOR_QUESTION_PROMPT.replace("[PROJECT_DESCRIPTION]", self.proj_description).\
            replace("[TOPICS_WITH_NUMBERS]", get_items_with_numbers(topics)).\
            replace("[QUESTION]", question.strip())

        result = self.ai_core.get_generated_text(prompt, 2)

        topic_id = get_idx_from_response(result)
        return topics[topic_id - 1] if topic_id < len(topics) else None

    def check_file(self, path, question):
        found_answer = False

        def check_answer( answer):
            nonlocal found_answer

            if answer.find("__NOTHING__") == -1 or answer.find("o needs for ") != -1 or len(answer) > 200:
                header = "Found the answer: " if answer.find("__NOTHING__") == -1 else "Found a relevant info: "
                self._on_step(header + answer.replace("__NOTHING__", "NOT_SURE"), file_content, SELECTED_TEXT)
                found_answer = True
                return False

            return True

        file_content = get_file_content(path)
        parent = os.path.dirname(path)
        parent_info = self.storage.get_json(get_file_id(parent))
        parent_desc = parent_info[DESCRIPTION_FIELD] if parent_info else "No description available"
        file_name = os.path.basename(path)

        context = CHECK_THE_FILE_CONTEXT.replace("[PROJECT_DESCRIPTION]", self.proj_description)\
            .replace("[PARENT_FOLDER_DESCRIPTION]", parent_desc)

        prompt = CHECK_THE_FILE_PROMPT.replace("[QUESTION]", question). \
            replace("[FILE_NAME]", file_name).\
            replace("[SOURCES]", file_content)

        self._on_step(f"Check file {file_name} content.", "Context: \n" + context + "\nPrompt:\n" + prompt)

        result = self.ai_core.get_1_or_2_steps_conversation_result(prompt,
                                                                   ONLY_RELATED_ITEM_NAME_PROMPT,
                                                                   check_answer,
                                                                   100,
                                                                   context)

        self.checked_files.add(path)

        if found_answer and result != "__NOTHING__":
            self._on_step(result, file_content, ITEM_TO_HIGHLIGHT)
            return result, path

        self._on_step(f"No relevant info was found.", result)
        return None, None

    def _fill_questions2files(self):
        for item in self.storage.get_all():
            if PATH_FIELD in item and QUESTIONS_FIELD in item:
                path = item[PATH_FIELD]
                self.questions2files.update({question: path for question in item[QUESTIONS_FIELD]})

    def _fill_topics(self):
        # TODO: must be created automatically
        topics = ["Android UI",
            "Native code integration",
            "Video processing",
            "Mathematics",
            "Image processing: Algorithms",
            "Image processing: Data structures",
            "Security",
            "Rendering",
            "Sensors",
            "Filesystem",
            "Other"]

        main_topics = {t: [] for t in topics}

        main_topics["Native code integration"] = {"Integrating C++ code with Java using the Java Native Interface (JNI)": [],
                                                       "Compiler-specific questions": [],
                                                       "Platform-specific questions": [],
                                                       "Other": []}

        main_topics["Image processing: Algorithms"] = {
            "Key point detection and processing": [],
            "Gradient calculation and processing": [],
            "Real world objects and planes detection": [],
            "Plotting and visualization": [],
            "Other": []}

        main_topics["Image processing: Data structures"] = {
            "Data structures to work with the whole picture": [],
            "Geometry primitives": [],
            "Picture features - keypoints, descriptors etc.": [],
            "Other": []}

        self.main_topics = main_topics

    def _merge_trees(self, new_structure: dict, old_topics: dict):
        it1 = iter(new_structure.items())
        it2 = iter(old_topics.items())
        result = dict()
        has_changes = False

        try:
            while True:
                t1, items1 = next(it1)
                t2, items2 = next(it2)

                if t1 != t2:
                    assert False, "Not implemented"

                if isinstance(items1, dict) and isinstance(items2, list):
                    self._distribute_questions(items2, items1)
                    result[t1] = items1
                    has_changes = True
                else:
                    result[t1] = items2
        except StopIteration:
            pass

        self.main_topics = result

        if has_changes:
            self.storage.insert_json(MAIN_TOPICS_ID, self.main_topics)


    def _make_tree(self):
        main_topics = self.storage.get_json(MAIN_TOPICS_ID)
        #main_topics = None
        if main_topics:
            print("Loaded topics:")
            self._print_topics(main_topics)
            self._merge_trees(self.main_topics, main_topics)
            return

        count = len(self.questions2files)
        print(f"Questions count: {count}")

        all_questions = set(self.questions2files.keys())
        self._distribute_questions(all_questions, self.main_topics)

        self.storage.insert_json(MAIN_TOPICS_ID, self.main_topics)

        #topics2questions = self._group_questions(self.questions2files)
        #self._fill_main_topics(topics2questions)

    def _distribute_questions(self, questions, topics):
        for question in questions:
            detected = self._get_topic_for_question(question, topics)

            if detected:
                topic = topics[detected]
                if isinstance(topic, dict):
                    self._distribute_questions([question], topic)
                else:
                    topic.append(question)
            else:
                topics["Other"].append(question)

        print("Questions are distributed:")
        self._print_topics(topics)

    def _print_topics(self, topics):
        for topic, questions in topics.items():
            print(topic + ". " + str(len(questions)) + ": " + str(questions))

    # def _find_questions_related_to_topic(self, questions: List[str], topic):
    #     questions_with_numbers = get_items_with_numbers(questions)
    #
    #     prompt = TOPICS_QUESTIONS_PROMPT.replace("[TOPIC]", topic). \
    #         replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers). \
    #         replace("[PROJECT_DESCRIPTION]", self.proj_description)
    #
    #     response = self.ai_core.get_generated_text(prompt, 50)
    #
    #     ids = self._get_ids(response)
    #     return [questions[idx - 1] for idx in ids if idx <= len(questions)]

    # def _fill_main_topics(self, topics2questions):
    #     self.main_topics = self.storage.get_json(MAIN_TOPICS_ID)
    #
    #     if self.main_topics is not None:
    #         return
    #
    #     scores = {topic: 0 for topic in topics2questions.keys()}
    #     topics = list(topics2questions.keys())
    #     topics2own_questions = {t: [] for t in topics}
    #     distributed_questions = set()
    #
    #     for topic, questions in topics2questions.items():
    #         for question in questions[:QUESTIONS_FOR_TOPICS_MAPPING]:
    #             random.shuffle(topics)
    #             topics.remove(topic)
    #             topics.insert(0, topic)
    #             self.main_topics = {t: topics2questions[t] for t in topics[:MAIN_TOPICS_COUNT]}
    #
    #             detected = self.get_topic_for_question(question)
    #
    #             if detected == topic:
    #                 scores[topic] += 1
    #                 topics2own_questions[topic].append(question)
    #                 distributed_questions.add(question)
    #             elif detected:
    #                 scores[detected] -= 1
    #
    #     topics2scores = [(topic, score) for topic, score in scores.items()]
    #     list.sort(topics2scores, key=lambda x: x[1], reverse=True)
    #     self.main_topics = {topic: topics2own_questions[topic] for topic, _ in topics2scores[:MAIN_TOPICS_COUNT]}
    #     print("Main topics:")
    #     self._print_topics(self.main_topics)
    #     self.storage.insert_json(MAIN_TOPICS_ID, self.main_topics)
    #
    #     all_questions = set(self.questions2files.keys())
    #     all_questions -= set(distributed_questions)
    #
    #     self._distribute_questions(all_questions)
    #
    #     self.storage.insert_json(MAIN_TOPICS_ID, self.main_topics)



    # def _group_questions(self, questions2files: Dict[str, str]) -> Dict[str, List[str]]:
    #     result = self.storage.get_json(FOUND_TOPICS_ID)
    #     result_required_size = MAIN_TOPICS_COUNT * 3
    #
    #     if False and result is not None:
    #         print("Loaded topics:")
    #         self._print_topics(result)
    #
    #         if len(result) >= result_required_size:
    #             return result
    #     else:
    #         result = dict()
    #
    #     all_questions = list(questions2files.keys())
    #     random.shuffle(all_questions)
    #     questions_to_distribute = set(all_questions[:(QUESTIONS_FOR_MAIN_TOPICS * 2) // 3])
    #     all_questions = all_questions[:QUESTIONS_FOR_MAIN_TOPICS]
    #
    #     while len(result) < result_required_size:
    #         questions_sample = random.sample(questions_to_distribute, QUESTIONS_PER_REQUEST)
    #         topics = self._get_topics(questions_sample)
    #
    #         # TODO: optimize it - _get_items_with_numbers is called for each topic
    #         for topic in topics:
    #             topics_questions = []
    #             too_wide = False
    #
    #             for i in range(0, len(all_questions), QUESTIONS_PER_REQUEST_FOR_TOPIC):
    #                 subset = all_questions[i: i + QUESTIONS_PER_REQUEST_FOR_TOPIC]
    #
    #                 related = self._find_questions_related_to_topic(subset, topic)
    #                 topics_questions.extend(related)
    #
    #                 if len(related) > QUESTIONS_PER_REQUEST_FOR_TOPIC // 2:
    #                     too_wide = True
    #                     print("Too many related items - the topic is not distinct")
    #                     break
    #
    #             if not too_wide and (len(all_questions) // 3) > len(topics_questions) >= QUESTIONS_FOR_TOPICS_MAPPING:
    #                 result[topic] = topics_questions
    #
    #                 for question in related:
    #                     if question in questions_to_distribute:
    #                         questions_to_distribute.remove(question)
    #
    #                 self.storage.insert_json(FOUND_TOPICS_ID, result)
    #
    #                 print("Found " + str(len(result)) + " topics.")
    #
    #     print("Detected topics:")
    #     self._print_topics(result)
    #     return result

    # def _get_topics(self, questions: List[str]) -> List[str]:
    #     def remove_extra_info(topic):
    #         topic = topic.split(",")[0]
    #         topic = topic.split("(")[0]
    #         return topic
    #
    #     questions_with_numbers = self._get_items_with_numbers(questions)
    #
    #     prompt = GROUP_QUESTIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", questions_with_numbers).\
    #         replace("[PROJECT_DESCRIPTION]", self.proj_description)
    #
    #     response = self.ai_core.get_generated_text(prompt, 60)
    #     topics = [remove_extra_info(topic) for topic in parse_numbered_items(response)]
    #     print("Found topics: " + str(topics))
    #     return topics





