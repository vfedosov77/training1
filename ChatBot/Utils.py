from typing import List


def parse_numbered_items(questions_str):
    idx = questions_str.find("1. ")

    if idx == -1:
        print("Error!!!!!!!!!!!!!!!!!!!!: cannot find the first question")
        return []

    questions_str = questions_str[idx:]
    return [q.split(".")[1] for q in questions_str.split("\n") if q and str.isnumeric(q[0]) and 0 < q.find(".") < 3]


def get_items_with_numbers(self, items: List[str]):
    return "".join(str(id + 1) + ". " + quest + "\n" for id, quest in enumerate(items))


def get_comma_separated_ids(response):
    response = response.split('\n')[0].split('(')[0].replace(".", "")
    ids = [int(idx) for idx in response.split(",") if idx.strip()]
    return ids