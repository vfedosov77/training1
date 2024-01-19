from ChatBot.AI.Gpt35AICore import Gpt35AICore
from KnowledgeBase import KnowledgeBase
from ChatBot.UI.IndexUI import IndexUI
from ChatBot.Common.Configuration import *
from ChatBot.Common.Constants import *
from ChatBot.Common.Utils import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.JSONDataStorage import JSONDataStorage


def usage():
    print(
        """Usage: python index.py PATH_TO_PROJECT PROJECT_DESCRIPTION DIRS_TO_EXCLUDE
    PATH_TO_PROJECT: FULL path to project.
    PROJECT_DESCRIPTION: A short description of the project. It will be used by GPT API for the project analysis.
    DIRS_TO_EXCLUDE: List of directories to exclude. Relative path from the project directory must be used. 
    IMPORTANT: Please take into account that PROJECT_DESCRIPTION will be used by GPT API and must be informative.""")
    exit(-1)


if len(sys.argv) < 3:
    usage()

path = linux_style_path(sys.argv[1])
project_description = sys.argv[2]

if not os.path.exists(path) or not os.path.isdir(path):
    print("Cannot find project directory: " + path)
    usage()

folders_to_exclude = sys.argv[3:] if len(sys.argv) > 3 else []
folders_to_exclude = [linux_style_path(f) for f in folders_to_exclude]

for folder in folders_to_exclude:
    if not check_if_relative_path(folder):
        print("Relative project paths must be used as DIRS_TO_EXCLUDE")
        usage()

storage = JSONDataStorage(os.path.join(path, DB_FILE_NAME))
set_app_config(Configuration(path, project_description, folders_to_exclude))
storage.insert_json(PROJECT_DESCRIPTION_ID, project_description)
storage.insert_json(FOLDERS_TO_EXCLUDE_ID, folders_to_exclude)

dispatcher = NotificationDispatcher()
app = IndexUI(dispatcher)

ai_core = Gpt35AICore()
base = KnowledgeBase(ai_core, dispatcher, storage)
base.index_project()

base

app.mainloop()