import ChatBot.Common.Utils as u
from ChatBot.Common.Constants import *

import os
from typing import List
import pathlib as pl


class Configuration:
    def __init__(self, path, project_description, black_list_dirs: List[str]):
        self.path = u.linux_style_path(path)
        self.project_description = project_description
        self.black_list_dirs = [f.replace("\\", "/") for f in black_list_dirs]

        if self.path.endswith("/"):
            self.path = self.path[: -1]

        self.code_suffices = {"cpp", "c", "h", "hpp", "java", "py"}
        self.doc_suffices = {"md"}


    def get_project_description(self):
        return self.project_description

    def get_project_path(self):
        return self.path

    def is_path_excluded(self, path):
        return any(path.startswith(d) for d in self.black_list_dirs)

    def get_file_kind(self, path):
        suffix = pl.Path(path).suffix.lower()[1:]

        if suffix in self.code_suffices:
            return FILE_KIND

        if suffix in self.doc_suffices:
            return DOCUMENT_KIND

        return UNKNOWN_KIND


config: Configuration = None


def get_config():
    return config

def set_app_config(app_config: Configuration):
    global config
    config = app_config






