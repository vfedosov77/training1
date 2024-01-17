from ChatBot.Common.Configuration import get_config


def add_project_description(prompt: str):
    return prompt.replace("[PROJECT_DESCRIPTION]", get_config().get_project_description())