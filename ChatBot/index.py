from ChatBot.AI.Gpt35AICore import Gpt35AICore
from KnowlegeBase import KnowlegeBase
from ChatBot.UI.IndexUI import IndexUI
from ChatBot.Common.Configuration import *
from ChatBot.Common.Utils import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher


def usage():
    print(
        """Usage: python index.py PATH_TO_PROJECT PROJECT_DESCRIPTION
    IMPORTANT: Please take into account that PROJECT_DESCRIPTION will be used by GPT API and must be informative.""")
    exit(-1)


if len(sys.argv) != 3:
    usage()

path = sys.argv[1]
project_description = sys.argv[2]

if not os.path.exists(path) or not os.path.isdir(path):
    print("Cannot find project directory: " + path)
    usage()

set_app_config(Configuration(path, project_description))

dispatcher = NotificationDispatcher()
app = IndexUI(dispatcher)

ai_core = Gpt35AICore()
base = KnowlegeBase(ai_core, dispatcher)
base.index_project()

app.mainloop()