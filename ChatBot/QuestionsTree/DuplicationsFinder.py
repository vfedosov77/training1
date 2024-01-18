from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.QuestionsTree.QuestionsTree import QuestionsTree, NORMAL_TEXT, SELECTED_TEXT, MAX_ITEMS_IN_REQUEST
from ChatBot.Prompts.QuestionsProccesingPrompts import *
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Common.Utils import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher

from typing import List
import random
import math


class DuplicationsFinder:
    def __call__(self, tree: QuestionsTree, ai_core: AiCoreBase, dispatcher: NotificationDispatcher):
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

        topics_to_simplify.sort(key=lambda x: len(x[1]), reverse=True)

        has_changes = False

        for parents, questions in topics_to_simplify:
            start_count = len(questions)
            old_topics = get_items_with_numbers(questions)

            if len(questions) <= 8:
                while len(questions) > 2 and self._find_duplication(questions, parents, tree, ai_core, dispatcher):
                    pass
            else:
                samples_count = (len(questions) // MAX_ITEMS_IN_REQUEST + 1)
                samples_count = int(math.pow(samples_count, 1.7))

                for _ in  range(samples_count):
                    sample = random.sample(questions, min(len(questions), MAX_ITEMS_IN_REQUEST))
                    was = set(sample)

                    while len(sample) > 2 and self._find_duplication(sample, parents, tree, ai_core, dispatcher):
                        pass

                    sample = set(sample)

                    for item in was - sample:
                        questions.remove(item)

                    # Add merged questions
                    for item in sample - was:
                        questions.append(item)

            if len(questions) != start_count:
                has_changes = True
                dispatcher.on_event(f"The amount of items reduced from {start_count} to {len(questions)}",
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
    def _get_united_question(id1, id2, questions, ai_core: AiCoreBase, topic):
        context = add_project_description(CHECK_DUPLICATION_CONTEXT).replace("[TOPIC_NAME]", topic)
        prompt = CHECK_DUPLICATION_PROMPT.\
            replace("[Question1]", questions[id1 - 1]).\
            replace("[Question2]", questions[id2 - 1])

        can_remove = ai_core.get_yes_no_result(prompt, context)

        if can_remove:
            prompt = GET_ITEM_TO_REMOVE_PROMPT. \
                replace("[Question1]", questions[id1 - 1]). \
                replace("[Question2]", questions[id2 - 1])

            new_topic = ai_core.get_short_conversation_result(prompt, 50, context)
            start_tag = "RESULT: "
            idx = new_topic.find(start_tag)

            if idx != -1:
                return new_topic[idx + len(start_tag):]

        return None

    @staticmethod
    def _find_duplication(questions,
                          parents, tree: QuestionsTree,
                          ai_core: AiCoreBase,
                          dispatcher: NotificationDispatcher):
        topic = parents[-1]
        context = add_project_description(FIND_DUPLICATIONS_CONTEXT).replace("[TOPIC_NAME]", topic)
        prompt = FIND_DUPLICATIONS_PROMPT.replace("[QUESTIONS_WITH_NUMBERS]", get_items_with_numbers(questions))
        id1, id2 = ai_core.get_pair_of_ids_result(prompt, 20, context)

        if id1 is not None and id1 < len(questions) and id2 < len(questions):
            new_question = DuplicationsFinder._get_united_question(id2, id1, questions, ai_core, topic)

            if new_question is not None:
                to_remove = questions[id2 - 1]
                to_leave = questions[id1 - 1]

                tree.merge_leaf_nodes(parents, to_leave, to_remove, new_question)
                questions.remove(to_remove)
                questions[id1 - 1] = new_question

                DuplicationsFinder._add_log_info(dispatcher, to_leave, to_remove, new_question)
                return True

        return False

    @staticmethod
    def _add_log_info(dispatcher: NotificationDispatcher, to_leave, to_remove, new_question):
        message = f"Questions '{to_leave}' and '{to_remove}' are similar. United under: {new_question}"
        print(message)
        dispatcher.on_event(message, None, NORMAL_TEXT)


