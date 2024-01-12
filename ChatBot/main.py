import os

from Gpt35AICore import Gpt35AICore
from KnowlegeBase import KnowlegeBase
from Dialog import Dialog
from Constants import *
from ChatBot.Utils import *
from collections import defaultdict


app = Dialog()


def test():
    questions = ["Where is the main view of the application defined?",
        "I have a bug: the camera focus does not work.",
        "Is there any code related to data bases?",
        "Where application stores data?",
        "In which folder application stores data?",
        "How the native code is integrated with Android?",
        "Where matrix multiplication is processed?",
        "How images keypoints are compared?",
        "What Series class is responsible for?",
        "What Figure class is responsible for?",
        "Where keypoints are drawn on a picture in the project?",
        "How the camera focus is processed?"]

    correct_count = 0
    results = defaultdict(float)

    for question in questions:
        result = base.get_answer(question, [])

        if result is not None:
            correct_count += 1
            results[question] += 0.5

        result = base.get_answer(question, [])

        if result is not None:
            correct_count += 1
            results[question] += 0.5

    print("Correct count = " + str(correct_count))

    for question in questions:
        val = results[question]
        print(f"{question}: {val}")


def on_step_callback(summary, details, kind=NORMAL_TEXT):
    app.add_log_entry(summary, details, kind)


ai_core = Gpt35AICore()
base = KnowlegeBase(ai_core, on_step_callback)

base.discover_project(get_local_path("/home/q548040/Downloads/ParallelWorld/ParallelWorld/"))
tree = base.get_tree()

test()
exit(0)

app.set_provider(base)
app.mainloop()

#tree.get_answer("Where is the main view of the application defined?")