from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree.QuestionsTree import QuestionsTree, NORMAL_TEXT, SELECTED_TEXT, MAX_ITEMS_IN_REQUEST
from ChatBot.Prompts.QuestionsProccesingPrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Common.Utils import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher, NotificationDispatcherStub
from ChatBot.QuestionsTree.FileQuestionChecker import FileQuestionsChecker, get_file_questions_checker

from typing import List
import random
import math


class QuestionsChecker:
    def __call__(self, questions2files: dict, ai_core: AiCoreBase, dispatcher: NotificationDispatcher):
        fake_dispatcher = NotificationDispatcherStub()
        for question, files in [(k, v) for k, v in questions2files.items()]:
            assert len(files) == 1
            file = next(iter(files))

            passed = False

            for i in range(2):
                if get_file_questions_checker()(file, question, fake_dispatcher, 20) is not None:
                    passed = True
                    break

            if not passed:
                dispatcher.on_event(f"Question '{question}' will be removed.", None, NORMAL_TEXT)
                del questions2files[question]
            else:
                dispatcher.on_event(f"Question '{question}' is passed.", None, SELECTED_TEXT)

