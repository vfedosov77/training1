import os

from Gpt35AICore import Gpt35AICore
from KnowlegeBase import KnowlegeBase
from Dialog import Dialog
from Constants import *

app = Dialog()


def on_step_callback(summary, details, kind=NORMAL_TEXT):
    app.add_log_entry(summary, details, kind)


ai_core = Gpt35AICore()
base = KnowlegeBase(ai_core, on_step_callback)

base.discover_project("/home/q548040/Downloads/ParallelWorld/ParallelWorld/")
tree = base.get_tree()

app.set_provider(base)
app.mainloop()

#tree.get_answer("Where is the main view of the application defined?")