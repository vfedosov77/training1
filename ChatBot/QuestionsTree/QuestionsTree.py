from ChatBot.Prompts.TopicsTreePrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Common.Utils import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree.FileQuestionChecker import FileQuestionsChecker, get_file_questions_checker

import os
from typing import Dict, List, Set
from ChatBot.Common.Constants import *


QUESTIONS_PER_REQUEST = 70
QUESTIONS_PER_REQUEST_FOR_TOPIC = 20
QUESTIONS_FOR_TOPICS_MAPPING = 10
MAIN_TOPICS_COUNT = 10


QUESTIONS_FOR_MAIN_TOPICS = QUESTIONS_PER_REQUEST * 2


class QuestionsTree:
    class Iterator:
        def __init__(self, topics: Dict or List):
            self.topics = topics
            self.is_dict = isinstance(topics, dict)
            self.iter = iter(self.topics.keys()) if self.is_dict else iter(self.topics)
            self.current_item = None

        def __next__(self):
            self.current_item = next(self.iter)
            return self.current_item

        def has_subtopics(self):
            return self.is_dict

        def to_children(self):
            children = self.topics[self.current_item]
            assert isinstance(children, list) or isinstance(children, dict)
            return QuestionsTree.Iterator(children)

    def __init__(self,
                 questions2files: Dict[str, Set[str]],
                 main_topics: Dict[str, List[str]],
                 ai_core: AiCoreBase,
                 storage: JSONDataStorage,
                 dispatcher: NotificationDispatcher):
        # TODO: one to many implementation
        self.questions2files = questions2files
        self.ai_core = ai_core
        self.storage: JSONDataStorage = storage
        self.main_topics: Dict[str, List[str]] = main_topics
        self.dispatcher = dispatcher
        self.checked_files = set()

        print("Loaded topics:")
        print_topics(main_topics)

    def get_answer(self, question: str, chat_history = None):
        try:
            self.checked_files.clear()
            return self._get_answer(question, self.main_topics)
        finally:
            self.checked_files.clear()

    def __iter__(self):
        return QuestionsTree.Iterator(self.main_topics)

    def remove_node(self, path: List[str], item: str, check_if_leaf=True):
        cur_item = self._get_node_by_path(path)

        if isinstance(cur_item, dict):
            assert not check_if_leaf
            del cur_item[item]
        else:
            cur_item.remove(item)

        del self.questions2files[item]

    def change_question(self, path: List[str], item: str, new_question: str):
        if item == new_question:
            return

        cur_item = self._get_node_by_path(path)

        if isinstance(cur_item, dict):
            assert False, "Not implemented"

        cur_item[cur_item.index(item)] = new_question
        self.questions2files[new_question] = self.questions2files[item]
        del self.questions2files[item]

    def merge_leaf_nodes(self, path: List[str], node_1: str, node_2: str, merged_value: str):
        files = self.questions2files[node_2]
        self.questions2files[node_1].update(files)
        self.change_question(path, node_1, merged_value)
        self.remove_node(path, node_2)

    def get_corresponding_item(self, question, items):
        with_numbers = get_items_with_numbers(items)
        items_count = len(items)
        context = add_project_description(WHICH_TOPIC_IS_CLOSEST_CONTEXT)
        prompt = WHICH_TOPIC_IS_CLOSEST_PROMT.replace("[QUESTION]", question). \
            replace("[TOPICS_WITH_NUMBERS]", with_numbers)

        self._on_step(f"Find corresponding topic from {items_count} items.", prompt)
        result = self.ai_core.get_short_conversation_result(prompt, 20, context)
        idx = get_idx_from_response(result)
        self._on_step(f"Selected item with id={idx}.", None)
        return idx

    def _get_node_by_path(self, path: List[str]):
        cur_item = self.main_topics

        for item in path:
            cur_item = cur_item[item]

        assert cur_item
        return cur_item

    def _get_answer(self, question: str, topics_dict: dict, ignore_topic = None):
        topics = [t for t in topics_dict.keys() if t != ignore_topic]
        topic_id = self.get_corresponding_item(question, topics)

        if topic_id is None:
            #topic_id = self._get_corresponding_item(question, topics)
            #if topic_id is None:
            self._on_step("Error during the request processing", None)
            print("Some error during the request processing")
            return None

        topic = topics[topic_id - 1]
        questions = topics_dict[topic]

        answer = None

        if isinstance(questions, dict):
            answer = self._get_answer(question, questions, None)
        else:
            questions = [q for q in questions if not contains_all(self.checked_files, self.questions2files[q])]

            while len(questions) > 0:
                cur_questions = questions[:MAX_ITEMS_IN_REQUEST]
                question_id = self.get_corresponding_item(question, cur_questions)

                if question_id is not None and question_id < len(cur_questions):
                    cur_question = cur_questions[question_id - 1]

                    for file in self.questions2files[cur_question]:
                        if file not in self.checked_files:
                            result, path = get_file_questions_checker()(file, question, self.dispatcher)
                            self.checked_files.add(file)

                            if result:
                                answer = (result, path)
                                print("Answer: " + result + " File: " + path)
                                return answer

                cur_questions = set(cur_questions)
                questions = [q for q in questions
                             if q not in cur_questions and
                             not contains_all(self.checked_files, self.questions2files[q])]

        if answer is None and ignore_topic is None:
            return self._get_answer(question, topics_dict, topic)

        return answer

    def _on_step(self, short_name, description, kind=NORMAL_TEXT):
        self.dispatcher.on_event(short_name, description, kind)


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





