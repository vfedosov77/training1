from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.Prompts.PromptUtils import *
from ChatBot.Prompts.TopicsTreePrompts import *
from ChatBot.Common.Utils import *
from ChatBot.Common.Constants import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.JSONDataStorage import JSONDataStorage

checked_files = set()


class FileQuestionsChecker:
    def __init__(self, ai_core: AiCoreBase, storage: JSONDataStorage):
        self.ai_core = ai_core
        self.storage = storage

    def __call__(self, path, question, dispatcher: NotificationDispatcher, max_tockens=100) -> str:
        if path in checked_files:
            return None, None

        checked_files.add(path)

        found_answer = False

        def check_answer(answer):
            nonlocal found_answer

            if answer.find("NOTHING") == -1 or answer.find("o needs for ") != -1 or len(answer) > 200:
                header = "Found the answer: " if answer.find("NOTHING") == -1 else "Found a relevant info: "
                dispatcher.on_event(header + answer.replace("__NOTHING__", "NOT_SURE"), file_content, SELECTED_TEXT)
                found_answer = True
                return False

            return True

        file_content = get_file_content(path)
        parent = os.path.dirname(path)
        parent_info = self.storage.get_json(get_file_id(parent))
        parent_desc = parent_info[DESCRIPTION_FIELD] if parent_info else "No description available"
        file_name = os.path.basename(path)

        context = add_project_description(CHECK_THE_FILE_CONTEXT) \
            .replace("[PARENT_FOLDER_DESCRIPTION]", parent_desc.replace("\n", " "))

        prompt = CHECK_THE_FILE_PROMPT.replace("[QUESTION]", question). \
            replace("[FILE_NAME]", file_name). \
            replace("[SOURCES]", file_content)

        dispatcher.on_event(f"Check file {file_name} content.",
                                 "Context: \n" + context + "\nPrompt:\n" + prompt,
                                 NORMAL_TEXT)

        result = self.ai_core.get_1_or_2_steps_conversation_result(prompt,
                                                                   ONLY_RELATED_ITEM_NAME_PROMPT,
                                                                   check_answer,
                                                                   max_tockens,
                                                                   context)

        if found_answer and result != "__NOTHING__":
            dispatcher.on_event(result, file_content, ITEM_TO_HIGHLIGHT)
            return result

        dispatcher.on_event(f"No relevant info was found.", result, NORMAL_TEXT)
        return None


instance: FileQuestionsChecker = None


def get_file_questions_checker():
    return instance


def set_file_questions_checker(checker: FileQuestionsChecker):
    global instance
    instance = checker


def clear_processed_files():
    checked_files.clear()


def get_processed_files():
    return checked_files


