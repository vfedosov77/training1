import ChatBot.Common.Utils as u

import os
from typing import List


class Configuration:
    def __init__(self, path, project_description, black_list_dirs: List[str]):
        self.path = u.linux_style_path(path)
        self.project_description = project_description
        self.black_list_dirs = [f.replace("\\", "/") for f in black_list_dirs]

        if self.path.endswith("/"):
            self.path = self.path[: -1]


    def get_project_description(self):
        return self.project_description

    def get_project_path(self):
        return self.path

    def is_path_excluded(self, path):
        return any(path.startswith(d) for d in self.black_list_dirs)


config: Configuration = None


def get_config():
    return config

def set_app_config(app_config: Configuration):
    global config
    config = app_config






