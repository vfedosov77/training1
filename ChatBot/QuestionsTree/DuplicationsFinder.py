from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree import QuestionsTree
from ChatBot.Prompts.QuestionsProccesingPrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Common.Utils import *

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
        topic = parents[-1]
        context = add_project_description(FIND_DUPLICATIONS_CONTEXT).replace("[TOPIC_NAME]", topic)
        prompt = FIND_DUPLICATIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", get_items_with_numbers(questions))
        id1, id2 = self.ai_core.get_pair_of_ids_result(prompt, 20, context)

        if id1 is not None

