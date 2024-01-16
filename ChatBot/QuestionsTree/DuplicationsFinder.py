from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree import QuestionsTree
from ChatBot.Prompts.QuestionsProccesingPrompts import *

from typing import List


class DuplicationsFinder:
    def __init__(self, ai_core: AiCoreBase):
        self.ai_core = ai_core

    def __call__(self, *args, **kwargs):
        tree: QuestionsTree = args[0]
        topics_to_simplify = []

        def search(it, parents: List):
            cur_topics = []

            try:
                while True:
                    item = next(it)

                    if it.has_children():
                        parents.apend(item)
                        children_it = it.to_children()
                        search(children_it, parents)
                        parents.pop()
                    else:
                        cur_topics.append(item)
            except StopIteration:
                pass

            if cur_topics:
                topics_to_simplify.append((list.copy(parents), cur_topics))

        search(iter(tree), [])

        for parents, questions in topics_to_simplify:
            self._find_duplications(questions, parents, tree)

    def _find_duplications(self, questions, parents, tree):
        context = FIND_DUPLICATIONS_CONTEXT.replace("[PROJECT_DESCRIPTION]")

