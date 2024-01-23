from ChatBot.KnowledgeBase import KnowledgeBase
from ChatBot.UI.Dialog import Dialog
from ChatBot.Common.Constants import *
from ChatBot.Common.Configuration import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Keywords.EmbeddingChecker import EmbeddingChecker
from ChatBot.AI.Gpt35AICore import Gpt35AICore

from collections import defaultdict
import sys, os


def on_step_callback(summary, details, kind=NORMAL_TEXT):
    global answer_found
    if summary.startswith("Found the answer:"):
        answer_found = True

    # app.add_log_entry(summary, details, kind)


def test():
    global answer_found

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
        answer_found = False
        app.add_role_message("Test",question)
        result = base.get_answer(question, [])

        if answer_found:
            correct_count += 1
            results[question] += 0.5

        answer_found = False
        app.add_role_message("Test", question)
        result = base.get_answer(question, [])

        if answer_found:
            correct_count += 1
            results[question] += 0.5

    print("Correct count = " + str(correct_count))

    for question in questions:
        val = results[question]
        print(f"{question}: {val}")







embeddings_ai = EmbeddingChecker()
embeddings = embeddings_ai.embed_text("Where the camera focus is processed? Where it is done?")

#with torch.no_grad():
#    outputs = model(**inputs)
#    # embeddings from the last layer
#    embeddings = outputs.last_hidden_state



path = sys.argv[1]
db_path = os.path.join(path, DB_FILE_NAME)
storage = JSONDataStorage(db_path)
project_description = storage.get_json(PROJECT_DESCRIPTION_ID)
folders_to_exclude = storage.get_json(FOLDERS_TO_EXCLUDE_ID)
assert project_description, "Index is broken"
set_app_config(Configuration(path, project_description, folders_to_exclude))
dispatcher = NotificationDispatcher()

answer_found = False

ai_core = Gpt35AICore()
base = KnowledgeBase(ai_core, dispatcher, storage)
app = Dialog(base, dispatcher)
dispatcher.add_events_observer(on_step_callback)
base.open_project()

test()

app.mainloop()