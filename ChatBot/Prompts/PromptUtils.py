project_description = None


def set_project_description(description: str):
    global project_description
    assert project_description is None, "Already assigned"
    project_description = description


def add_project_description(prompt: str):
    return prompt.replace("[PROJECT_DESCRIPTION]", project_description)