import os, sys
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.Common.Configuration import get_config
from ChatBot.Common.Utils import *


class KeywordsIndex:
    def __init__(self, path, code_files_suffices, dispatcher: NotificationDispatcher):
        self.path = path
        self.code_files_suffices: set = code_files_suffices
        self.dispatcher = dispatcher

    def find_keyword(self, keyword: str):
        # TODO: temporary approach - the index is required here
        for root, dirs, files in os.walk(self.path, topdown=False):
            for name in files:
                if name.split(".")[-1] in self.code_files_suffices:
                    path = os.path.join(root, name)

                    relative_path = get_relative_path(path)

                    if get_config().is_path_excluded(relative_path):
                        continue

                    try:
                        with open(path)  as f:
                            content = get_file_content(relative_path)

                            if content.find(keyword) != -1:
                                yield path
                    except Exception:
                        pass

