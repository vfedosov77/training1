from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree.QuestionsTree import QuestionsTree, NORMAL_TEXT, SELECTED_TEXT
from ChatBot.Prompts.QuestionsProccesingPrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Common.Utils import *

from typing import List


class DuplicationsFinder:
    def __call__(self, tree: QuestionsTree, ai_core: AiCoreBase, callback):
        topics_to_simplify = []

        def search(it, parents: List):
            cur_topics = []

            try:
                while True:
                    item = next(it)

                    if it.has_subtopics():
                        parents.append(item)
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
            start_count = len(questions)
            old_topics = get_items_with_numbers(questions)
            while len(questions) > 2 and self._find_duplication(questions, parents, tree, ai_core, callback):
                pass

            if len(questions) != start_count:

                callback(f"The amount of items reduced from {start_count} to {len(questions)}",
                         old_topics + "\n\n\n" + get_items_with_numbers(questions),
                         SELECTED_TEXT)

    @staticmethod
    def _check_if_questions_similar(id1, id2, questions, tree: QuestionsTree):
        question = questions[id1 - 1]

        id_to_exclude = id1 - 1
        step_id = 0

        while id_to_exclude < len(questions) and step_id < 3:
            if id_to_exclude < id2:
                id2 -= 1

            questions = questions[:id_to_exclude] + questions[id_to_exclude + 1:]
            id_to_exclude = tree.get_corresponding_item(question, questions)

            if id_to_exclude is None:
                return False

            if id_to_exclude == id2:
                return True

            id_to_exclude -= 1
            step_id += 1

        return False

    @staticmethod
    def _find_duplication(questions, parents, tree: QuestionsTree, ai_core: AiCoreBase, callback):
        topic = parents[-1]
        context = add_project_description(FIND_DUPLICATIONS_CONTEXT).replace("[TOPIC_NAME]", topic)
        prompt = FIND_DUPLICATIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", get_items_with_numbers(questions))
        id1, id2 = ai_core.get_pair_of_ids_result(prompt, 20, context)

        if id1 is not None and id1 < len(questions) and id2 < len(questions):
            if DuplicationsFinder._check_if_questions_similar(id2, id1, questions, tree):
                id_to_remove = id2 if len(questions[id2]) < len(questions[id1]) else id1
                id_to_leave = id2 if id_to_remove == id1 else id1
                message = f"Questions '{questions[id_to_leave - 1]}' and '{questions[id_to_remove - 1]}' are similar"
                print(message)
                callback(message, None, NORMAL_TEXT)
                to_remove = questions[id_to_remove - 1]
                parents.append(to_remove)
                tree.remove_node(parents)
                questions.remove(to_remove)
                parents.pop()
                return True

        return False


