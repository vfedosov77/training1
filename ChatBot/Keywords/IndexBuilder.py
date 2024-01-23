from ChatBot.AI.AiCoreBase import AiCoreBase
from ChatBot.Common.Constants import *
from ChatBot.Common.Utils import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.Prompts.PromptUtils import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Keywords.EmbeddingChecker import EmbeddingChecker
from ChatBot.Prompts.KeywordsPrompts import *

import os


class IndexBuilder:
    def __call__(self, ai_core: AiCoreBase, storage: JSONDataStorage, dispatcher: NotificationDispatcher):
        self.ai_core = ai_core
        self.dispatcher = dispatcher
        self.checker = EmbeddingChecker()
        self.storage = storage

        keywords = []

        count = 0

        for item in storage.get_all():
            if PATH_FIELD in item and (item[KIND_FIELD] == FILE_KIND or item[KIND_FIELD] == DOCUMENT_KIND):
                keywords.extend(self._create_keywords(item[PATH_FIELD]))
                count += 1
                #if count > 20:
                #    break

        self.checker.create_storage(keywords)
        dispatcher.on_event("Keywords index was successfully built.", None, SELECTED_TEXT)
        return self.checker

    def _create_keywords(self, path):
        file_id = get_file_id(path)
        json = self.storage.get_json(file_id)

        if KEYWORDS_FIELD in json:
            return json[KEYWORDS_FIELD]

        name = os.path.basename(path)
        context = add_project_description(GET_KEYWORDS_CONTEXT).replace("[FILE_NAME]", name)

        prompt_template = GET_KEYWORDS_PROMPT if get_config().get_file_kind(path) == FILE_KIND \
            else GET_DOCUMENT_KEYWORDS_PROMPT

        prompt = prompt_template.replace("[SOURCES]", get_file_content(path))
        response = self.ai_core.get_short_conversation_result(prompt, 100, context)
        keywords = parse_numbered_items(response)
        print(keywords)
        self.dispatcher.on_event(f"Created keywords for {name}: {str(keywords)}.", response, NORMAL_TEXT)
        json[KEYWORDS_FIELD] = keywords
        self.storage.insert_json(file_id, json)
        return keywords
