from typing import List


MAX_SYMBOLS_TO_READ = 25000


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
    result = response.strip()[:2]

    if len(result) > 1 and not str.isalnum(result[1]):
        result = result[0]

    if not str.isnumeric(result):
        print("Cannot parse index: " + (result if result else "None"))
        return None

    idx = int(result)
    return idx


def get_local_path(path):
    return path.replace("/content/drive/MyDrive/Sources/", "/home/q548040/Downloads/ParallelWorld/")


def get_file_content(path):
    path = get_local_path(path)

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

def get_file_id(path):
    path = path.replace("/home/q548040/Downloads/ParallelWorld/", "/content/drive/MyDrive/Sources/")
    return path
