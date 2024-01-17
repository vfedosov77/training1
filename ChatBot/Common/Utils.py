from ChatBot.Common.Configuration import get_config

from typing import List
import os, sys

MAX_SYMBOLS_TO_READ = 25000
LOCAL_PATH = os.environ.get("LOCAL_PATH")


def remove_line_number(line, sep):
    idx = line.find(sep)

    if idx == -1 or idx > 3:
        return None

    return line[idx + 1:].strip()


def parse_numbered_items(questions_str):
    idx = questions_str.find("1. ")
    sep = "."

    if idx == -1:
        idx = questions_str.find("1) ")
        sep = ")"

    if idx == -1:
        print("Error!!!!!!!!!!!!!!!!!!!!: cannot find the first question")
        return []

    questions_str = questions_str[idx:]
    return [remove_line_number(q, sep) for q in questions_str.split("\n")
            if q and str.isnumeric(q[0]) and remove_line_number(q, sep)]


def get_items_with_numbers(items: List[str]):
    return "".join(str(id + 1) + ". " + quest + "\n" for id, quest in enumerate(items))


def get_comma_separated_ids(response):
    response = response.split('\n')[0].split('(')[0].replace(".", "")
    ids = [int(idx) for idx in response.split(",") if idx.strip()]
    return ids


def get_idx_from_response(response: str):
    response = response.strip()
    result = response.strip()[:2]

    if len(result) > 1 and not str.isalnum(result[1]):
        result = result[0]

    if not str.isnumeric(result):
        items = response.split()

        if len(items) > 1:
            # Try all the items
            for item in items:
                res = get_idx_from_response(item)
                if res is not None:
                    return res

        print("Cannot parse index: " + (result if result else "None"))
        return None

    idx = int(result)
    return idx


# Looking for a "(id1, id1)" item in the response
def get_pair_of_ids_from_response(response: str):
    parts = response.split("(")

    if len(parts) < 2:
        return None, None

    parts = parts[1].split(")")
    parts = parts[0].split(",")

    if len(parts) != 2 or not str.isnumeric(parts[0]) or not str.isnumeric(parts[1].strip()):
        return None, None

    return int(parts[0]), int(parts[1].strip())


def check_if_relative_path(path):
    assert path[0] != "/" and (len(path) < 2 or path[1] != ":"), "Path is not project relative path: " + path


def get_relative_path(full_path):
    root_path = get_config().get_project_path()
    assert os.path.commonpath([root_path, full_path]) == root_path, \
        "The path must be related to the project: " + full_path
    return os.path.relpath(full_path, root_path)


def get_full_path(relative_path):
    check_if_relative_path(relative_path)
    root_path = get_config().get_project_path()
    return os.path.join(root_path, relative_path)


def get_file_content(relative_path):
    check_if_relative_path(relative_path)
    path = get_full_path(relative_path)

    try:
        with open(path) as f:
            text = f.read(MAX_SYMBOLS_TO_READ)

            if not text.strip():
                raise ValueError()
    except UnicodeDecodeError:
        try:
            with open(path) as f:
                text = f.read(-1)[:MAX_SYMBOLS_TO_READ]
        except UnicodeDecodeError:
            print("Cannot read the file content: " + path)
            raise ValueError()

    return text


def get_file_id(relative_path):
    check_if_relative_path(relative_path)
    path = relative_path.replace("\\", "/")
    return path


def get_file_id_by_full_path(path):
    assert path[0] == "/" or path[1] == ":", "It is not a full path: " + path
    return get_file_id(get_relative_path(path))


def contains_all(collection1: set, collection2: set or list):
    assert isinstance(collection2, set) or isinstance(collection2, list)

    for item in collection2:
        if item not in collection1:
            return False

    return True


