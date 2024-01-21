import os, sys
from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.Common.Constants import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher, NotificationDispatcherStub
from ChatBot.Common.Configuration import get_config
from ChatBot.Common.Utils import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Keywords.EmbeddingChecker import EmbeddingChecker
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Prompts.KeywordsPrompts import *

from collections import defaultdict

from ChatBot.QuestionsTree.FileQuestionChecker import get_file_questions_checker


class KeywordsIndex:
    def __init__(self, ai_core: AiCoreBase, storage: JSONDataStorage, dispatcher: NotificationDispatcher):
        self.ai_core = ai_core
        self.storage: JSONDataStorage = storage
        self.dispatcher = dispatcher
        self.checker = EmbeddingChecker()

        self.keywords2files = defaultdict(list)

        for item in storage.get_all():
            if PATH_FIELD in item and KEYWORDS_FIELD in item:
                for keyword in item[KEYWORDS_FIELD]:
                    self.keywords2files[keyword].append(item[PATH_FIELD])

    def get_answer(self, question: str):
        context = add_project_description(KEYPOINTS_FROM_QUESTION_CONTEXT)
        prompt = KEYPOINTS_FROM_QUESTION_PROMPT.replace("[QUESTION]", question)
        response = self.ai_core.get_short_conversation_result(prompt, 20, context)
        question_keywords = parse_numbered_items(response)
        question_keywords.append(question)

        found_keywords = set()

        for keyword in question_keywords:
            found_keywords.update(self.checker.find_keyword_matches(keyword))

        files2count = defaultdict(int)

        for keyword in found_keywords:
            for file in self.keywords2files[keyword]:
                files2count[file] += 1

        files2count = [(k, v) for k, v in files2count.items()]
        files2count.sort(key=lambda x: x[1], reverse=True)
        files2count = files2count[:3]

        for file, _ in files2count:
            result = get_file_questions_checker()(file, question, self.dispatcher)
            if result:
                # self.dispatcher.on_event("Found the answer: " + result, None, SELECTED_TEXT)
                return result, file

        return None, None



    # def bruteforce(self, keyword: str):
    #     # TODO: temporary approach - the index is required here
    #     for root, dirs, files in os.walk(self.path, topdown=False):
    #         for name in files:
    #             if name.split(".")[-1] in self.code_files_suffices:
    #                 path = os.path.join(root, name)
    #
    #                 relative_path = get_relative_path(path)
    #
    #                 if get_config().is_path_excluded(relative_path):
    #                     continue
    #
    #                 try:
    #                     with open(path)  as f:
    #                         content = get_file_content(relative_path)
    #
    #                         if content.find(keyword) != -1:
    #                             yield relative_path
    #                 except Exception:
    #                     pass

