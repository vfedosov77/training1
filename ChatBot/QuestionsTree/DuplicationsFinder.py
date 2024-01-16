from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree.QuestionsTree import QuestionsTree, NORMAL_TEXT, SELECTED_TEXT, MAX_ITEMS_IN_REQUEST
from ChatBot.Prompts.QuestionsProccesingPrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Common.Utils import *

from typing import List
import random


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
        has_changes = False

        for parents, questions in topics_to_simplify:
            start_count = len(questions)
            old_topics = get_items_with_numbers(questions)

            if len(questions) <= 8:
                while len(questions) > 2 and self._find_duplication(questions, parents, tree, ai_core, callback):
                    pass
            else:
                samples_count = (len(questions) // MAX_ITEMS_IN_REQUEST + 1) * 2

                for _ in  range(samples_count):
                    sample = random.sample(questions, min(len(questions), MAX_ITEMS_IN_REQUEST))
                    was = set(sample)

                    while len(sample) > 2 and self._find_duplication(sample, parents, tree, ai_core, callback):
                        pass

                    for item in was - set(sample):
                        questions.remove(item)

            if len(questions) != start_count:
                has_changes = True
                callback(f"The amount of items reduced from {start_count} to {len(questions)}",
                         old_topics + "\n\n\n" + get_items_with_numbers(questions),
                         SELECTED_TEXT)

        return has_changes

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
    def _get_question_to_remove(id1, id2, questions, ai_core: AiCoreBase, topic):
        context = add_project_description(CHECK_DUPLICATION_CONTEXT).replace("[TOPIC_NAME]", topic)
        prompt = CHECK_DUPLICATION_PROMPT.\
            replace("[Question1]", questions[id1 - 1]).\
            replace("[Question2]", questions[id2 - 1])

        can_remove = ai_core.get_yes_no_result(prompt, context)

        if can_remove:
            prompt = GET_ITEM_TO_REMOVE_PROMPT. \
                replace("[Question1]", questions[id1 - 1]). \
                replace("[Question2]", questions[id2 - 1])

            id_to_leave = ai_core.get_number_result(prompt, 10, context, 3)

            if id_to_leave is not None:
                return 1 if id_to_leave == 2 else 2

        return None

    @staticmethod
    def _find_duplication(questions, parents, tree: QuestionsTree, ai_core: AiCoreBase, callback):
        topic = parents[-1]
        context = add_project_description(FIND_DUPLICATIONS_CONTEXT).replace("[TOPIC_NAME]", topic)
        prompt = FIND_DUPLICATIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", get_items_with_numbers(questions))
        id1, id2 = ai_core.get_pair_of_ids_result(prompt, 20, context)

        if id1 is not None and id1 < len(questions) and id2 < len(questions):
            id_to_remove = DuplicationsFinder._get_question_to_remove(id2, id1, questions, ai_core, topic)

            if id_to_remove is not None:
                id_to_leave, id_to_remove = (id2, id1) if id_to_remove == 1 else (id1, id2)
                to_remove = questions[id_to_remove - 1]
                to_leave = questions[id_to_leave - 1]

                tree.merge_leaf_nodes(parents, to_leave, to_remove)
                questions.remove(to_remove)

                DuplicationsFinder._add_log_info(callback, to_leave, to_remove)
                return True

        return False

    @staticmethod
    def _add_log_info(callback, to_leave, to_remove):
        message = f"Questions '{to_leave}' and '{to_remove}' are similar"
        print(message)
        callback(message, None, NORMAL_TEXT)


