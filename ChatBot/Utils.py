def parse_numbered_items(questions_str):
    idx = questions_str.find("1. ")

    if idx == -1:
        print("Error!!!!!!!!!!!!!!!!!!!!: cannot find the first question")
        return []

    questions_str = questions_str[idx:]
    return [q.split(".")[1] for q in questions_str.split("\n") if q and str.isnumeric(q[0]) and 0 < q.find(".") < 3]