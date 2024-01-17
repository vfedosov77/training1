from ChatBot.AI.Gpt35AICore import Gpt35AICore
from KnowlegeBase import KnowlegeBase
from ChatBot.UI.Dialog import Dialog
from ChatBot.Common.Constants import *
from ChatBot.Common.Configuration import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.JSONDataStorage import JSONDataStorage

import sys, os

def usage():
    print(
        """Usage: python main.py PATH_TO_PROJECT""")
    exit(-1)


if len(sys.argv) != 2:
    usage()

path = sys.argv[1]

if not os.path.exists(path) or not os.path.isdir(path):
    print("Cannot find project directory: " + path)
    usage()

db_path = os.path.join(path, DB_FILE_NAME)

if not os.path.exists(db_path):
    print("Cannot find project index file: " + db_path + ". Please execute index.py script to create the index.")
    exit(-1)

storage = JSONDataStorage(db_path)
project_description = storage.get_json(PROJECT_DESCRIPTION_ID)
folders_to_exclude = storage.get_json(FOLDERS_TO_EXCLUDE_ID)
assert project_description, "Index is broken"
set_app_config(Configuration(path, project_description, folders_to_exclude))

dispatcher = NotificationDispatcher()
app = Dialog(dispatcher)
answer_found = False

ai_core = Gpt35AICore()
base = KnowlegeBase(ai_core, dispatcher, storage)

base.open_project()

#test()

app.mainloop()

#tree.get_answer("Where is the main view of the application defined?")