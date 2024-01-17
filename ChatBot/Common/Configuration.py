import os


class Configuration:
    def __init__(self, path, project_description):
        self.path = path
        self.project_description = project_description

    def get_project_description(self):
        return self.project_description

    def get_project_path(self):
        return self.path


config: Configuration = None


def get_config():
    return config

def set_app_config(app_config: Configuration):
    global config
    config = app_config






