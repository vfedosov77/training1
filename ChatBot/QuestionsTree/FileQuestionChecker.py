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

    def __call__(self, path, question, dispatcher: NotificationDispatcher, max_tokens=100):
        kind = get_config().get_file_kind(path)

        if kind == FILE_KIND:
            return self.check_source_file(path, question, dispatcher, max_tokens)
        elif kind == DOCUMENT_KIND:
            return self.check_document(path, question, dispatcher, max_tokens)

        raise NotImplementedError()

    def check_source_file(self, path, question, dispatcher: NotificationDispatcher, max_tokens) -> str:
        if path in checked_files:
            return None, None

        checked_files.add(path)

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

        return self._get_related_item_in_file(prompt, context, file_name, file_content, dispatcher, max_tokens)

    def check_document(self, path, question, dispatcher: NotificationDispatcher, max_tokens) -> str:
        if path in checked_files:
            return None, None

        checked_files.add(path)

        file_content = get_file_content(path)
        file_name = os.path.basename(path)

        context = add_project_description(CHECK_DOCUMENT_CONTEXT)
        prompt = CHECK_DOCUMENT_PROMPT.replace("[QUESTION]", question). \
            replace("[FILE_NAME]", file_name). \
            replace("[SOURCES]", file_content)

        return self._find_related_text(prompt, context, file_name, file_content, dispatcher, max_tokens)

    @staticmethod
    def _check_answer(answer):
        if answer.find("NOTHING") == -1 or answer.find("o needs for ") != -1 or len(answer) > 200:
            return True

        return False

    def _get_related_item_in_file(self,
                    prompt,
                    context,
                    file_name,
                    content,
                    dispatcher: NotificationDispatcher,
                    max_tokens) -> str:
        log_details = "Context: \n" + context + "\nPrompt:\n" + prompt
        found_answer = False

        def check_answer(answer):
            nonlocal found_answer

            if self._check_answer(answer):
                header = "Found the answer: " if answer.find("NOTHING") == -1 else "Found a relevant info: "
                dispatcher.on_event(header + answer.replace("__NOTHING__", "NOT_SURE"), log_details, SELECTED_TEXT)
                found_answer = True
                return False

            return True

        dispatcher.on_event(f"Check file {file_name} content.", log_details, NORMAL_TEXT)

        result = self.ai_core.get_1_or_2_steps_conversation_result(prompt,
                                                                   ONLY_RELATED_ITEM_NAME_PROMPT,
                                                                   check_answer,
                                                                   max_tokens,
                                                                   context)

        if found_answer and result != "__NOTHING__":
            dispatcher.on_event(result, content, ITEM_TO_HIGHLIGHT)
            return result

        dispatcher.on_event(f"No relevant info was found.", result, NORMAL_TEXT)
        return None

    def _find_related_text(self,
                    prompt,
                    context,
                    file_name,
                    content,
                    dispatcher: NotificationDispatcher,
                    max_tokens) -> str:
        log_details = "Context: \n" + context + "\nPrompt:\n" + prompt
        dispatcher.on_event(f"Check file {file_name} content.", log_details, NORMAL_TEXT)

        result = self.ai_core.get_short_conversation_result(prompt,
                                                            max_tokens,
                                                            context)
        if self._check_answer(result):
            header = "Found the answer: " if result.find("NOTHING") == -1 else "Found a relevant info: "
            dispatcher.on_event(header + result.replace("__NOTHING__", "NOT_SURE"), log_details, SELECTED_TEXT)
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


