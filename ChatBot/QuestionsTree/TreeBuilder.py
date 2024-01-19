from ChatBot.QuestionsTree.QuestionsTree import QuestionsTree
from ChatBot.QuestionsTree.QuestionsChecker import QuestionsChecker
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Common.Constants import *
from ChatBot.Common.Utils import *
from ChatBot.Prompts.TopicsTreePrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree.DuplicationsFinder import DuplicationsFinder

from typing import Dict, Set


class TreeBuilder:
    def __call__(self, ai_core: AiCoreBase, storage: JSONDataStorage, dispatcher):
        questions2files = storage.get_json(QUESTIONS2FILES_CHECKED_ID)

        if questions2files is None:
            questions2files = self._fill_questions2files(storage)
            #questions2files = storage.get_json(QUESTIONS2FILES_ID)

            if questions2files is not None:
                QuestionsChecker()(questions2files, ai_core, dispatcher)
                storage.insert_json(QUESTIONS2FILES_CHECKED_ID, prepare_for_storage(questions2files))

        if questions2files:
            questions2files = remove_duplications(questions2files)
            questions_simplified = True
        else:
            questions2files = self._fill_questions2files(storage)
            questions_simplified = False

        main_topics = self._fill_topics()
        main_topics = self._make_tree(main_topics, questions2files, storage, ai_core)

        tree = QuestionsTree(questions2files, main_topics, ai_core, storage, dispatcher)

        if not questions_simplified and DuplicationsFinder()(tree, ai_core, dispatcher):
            self._commit_changes(storage, main_topics, questions2files)

        return tree


    @staticmethod
    def _commit_changes(storage, main_topics, questions2files=None):
        storage.insert_json(MAIN_TOPICS_ID, main_topics)

        if questions2files:
            storage.insert_json(QUESTIONS2FILES_ID, prepare_for_storage(questions2files))

    @staticmethod
    def _fill_questions2files(storage):
        questions2files: Dict[str, Set] = dict()

        for item in storage.get_all():
            if PATH_FIELD in item and QUESTIONS_FIELD in item:
                path = item[PATH_FIELD]
                questions2files.update({question: {path} for question in item[QUESTIONS_FIELD] if question.strip()})

        return questions2files

    @staticmethod
    def _fill_topics():
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

        return main_topics

    @staticmethod
    def _merge_trees(new_structure: dict, old_topics: dict, ai_core: AiCoreBase, storage: JSONDataStorage):
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
                    TreeBuilder._distribute_questions(items2, items1, ai_core)
                    result[t1] = items1
                    has_changes = True
                else:
                    result[t1] = items2
        except StopIteration:
            pass

        if has_changes:
            TreeBuilder._commit_changes(storage, result)

        return result

    @staticmethod
    def _make_tree(main_topics, questions2files, storage: JSONDataStorage, ai_core: AiCoreBase):
        old_main_topics = storage.get_json(MAIN_TOPICS_ID)

        if old_main_topics:
            print("Loaded topics:")
            TreeBuilder._print_topics(old_main_topics)
            return TreeBuilder._merge_trees(main_topics, old_main_topics, ai_core, storage)

        count = len(questions2files)
        print(f"Questions count: {count}")

        all_questions = set(questions2files.keys())
        TreeBuilder._distribute_questions(all_questions, main_topics, ai_core)

        TreeBuilder._commit_changes(storage, main_topics)

        #topics2questions = TreeBuilder._group_questions(questions2files)
        #TreeBuilder._fill_main_topics(topics2questions)
        return main_topics

    @staticmethod
    def _distribute_questions( questions, topics, ai_core: AiCoreBase):
        for question in questions:
            detected = TreeBuilder._get_topic_for_question(question, topics, ai_core)

            if detected:
                topic = topics[detected]
                if isinstance(topic, dict):
                    TreeBuilder._distribute_questions([question], topic, ai_core)
                else:
                    topic.append(question)
            else:
                topics["Other"].append(question)

        print("Questions are distributed:")
        TreeBuilder._print_topics(topics)

    @staticmethod
    def _get_topic_for_question(question: str, topics_dict: dict, ai_core: AiCoreBase) -> str:
        topics = list(topics_dict.keys())

        prompt = add_project_description(TOPIC_FOR_QUESTION_PROMPT).\
            replace("[TOPICS_WITH_NUMBERS]", get_items_with_numbers(topics)).\
            replace("[QUESTION]", question.strip())

        result = ai_core.get_generated_text(prompt, 2)

        topic_id = get_idx_from_response(result)
        return topics[topic_id - 1] if topic_id < len(topics) else None

    @staticmethod
    def _print_topics( topics):
        for topic, questions in topics.items():
            print(topic + ". " + str(len(questions)) + ": " + str(questions))