from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.Common.Constants import *

import os
import sys
import shutil

test_db_source = os.path.join(os.curdir, "knowledge.db")
test_db_target = os.path.join(os.curdir, "MiniIndex/knowledge.db")

def create_test_db(path_to_knowledge_db):
    source = JSONDataStorage(path_to_knowledge_db)
    target = JSONDataStorage(test_db_source)
    main_topics = source.get_json(MAIN_TOPICS_ID)
    questions_to_files = source.get_json(QUESTIONS2FILES_ID)

    questions = main_topics["Image processing: Algorithms"]["Key point detection and processing"]
    new_questions2files = {q: questions_to_files[q] for q in questions}
    new_main_topics = {"Image processing: Algorithms": {"Key point detection and processing": questions}}

    target.insert_json(MAIN_TOPICS_ID, new_main_topics)
    target.insert_json(QUESTIONS2FILES_ID, new_questions2files)


if len(sys.argv) == 3 and sys.argv[1] == "-c":
    create_test_db(sys.argv[2])
    exit(0)

shutil.copy(test_db_source, test_db_target)

