from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage

import os
import pathlib as pl
import json

USER_PROMPT = f"context: ```{input}``` \n\n output: "

AI_REQUEST = ""
MAX_SYMBOLS_TO_READ = 50000
NODE_1 = "node_1"
NODE_2 = "node_2"
EDGE = "edge"

FILE_KIND = "File"
DIRECTORY_KIND = "Directory"
QUESTIONS_FIELD = "questions"
KIND_FIELD = "kind"
DESCRIPTION_FIELD = "description"
PATH_FIELD = "path"

class KnowlegeGraph:
    def __init__(self, ai_core):
        self.storage: JSONDataStorage = None
        self.files_info = dict()
        self.dirs_info = dict()
        self.ai_core = ai_core
        self.code_suffices = {"cpp", "c", "h", "hpp", "java", "py"}
        self.doc_suffices = {"txt", "md"}

    def discover_project(self, project_path: str):
        self.storage = JSONDataStorage(os.path.join(project_path, "knowledge.db"))

        def dfs(path):
            children = []

            for child in os.listdir(path):
                child_path = os.path.join(path, child)

                try:
                    if os.path.isdir(child_path):
                        children.append(dfs(child_path))
                    else:
                        children.append(self._process_file(child_path))
                except ValueError:
                    pass

            return self._process_dir(path, children)

        dfs(project_path)
        self._create_questions(project_path)

    def _create_questions(self, project_path):
        def dfs(path):
            for child in os.listdir(path):
                child_path = os.path.join(path, child)

                if os.path.isdir(child_path):
                    dfs(child_path)
                else:
                    self._generate_file_questions(child_path)

        dfs(project_path)

    @staticmethod
    def _get_max_answer_tokens_count(file_size):
        return file_size // 10

    @staticmethod
    def _get_id(path):
        return path

    @staticmethod
    def _create_json_for_path(path, kind, description):
        return {PATH_FIELD: path, KIND_FIELD: kind, DESCRIPTION_FIELD: description}

    def _process_dir(self, path, children):
        dir_id = self._get_id(path)

        if not children:
            raise ValueError()

        if self.storage.get_json(dir_id):
            print("Info was found for the directory: " + path)
            return dir_id

        files_descriptions = []

        for child in children:
            obj_json = self.storage.get_json(self._get_id(child))

            if json is None:
                raise RuntimeError("Not found ID which must be presented!")

            files_descriptions.append(obj_json[KIND_FIELD] + " " + os.path.basename(child) + ": " +
                                      obj_json[DESCRIPTION_FIELD] + "\n\n")

        descriptions = "".join(files_descriptions)
        dir_name = os.path.basename(path)

        prompt = DIRECTORY_SUMMARY_PROMPT.replace("[FILES_DESCRIPTION]", descriptions).\
            replace("[DIRECTORY_NAME]", dir_name)

        response = self.ai_core.get_short_conversation_result(prompt, 1500)
        self.storage.insert_json(dir_id, self._create_json_for_path(path, DIRECTORY_KIND, response))
        return dir_id

    @staticmethod
    def _get_file_content(path):
        with open(path) as f:
            text = f.read(MAX_SYMBOLS_TO_READ)

            if not text.strip():
                raise ValueError()

        return text

    def _process_file(self, path):
        suffix = pl.Path(path).suffix.lower()[1:]
        is_code = suffix in self.code_suffices
        is_doc = suffix in self.doc_suffices

        if not is_code and not is_doc:
            raise ValueError()

        file_id = self._get_id(path)

        if self.storage.get_json(file_id):
            print("Info was found for the file: " + path)
            return file_id

        text = self._get_file_content(path)
        response = self.ai_core.get_short_conversation_result(FILE_SUMMARY_PROMPT + text, 1500)
        self.storage.insert_json(file_id, self._create_json_for_path(path, FILE_KIND, response))

        return file_id

    def _get_children(self, path):
        children = []

        for child in os.listdir(path):
            item_json = self.storage.get_json(self._get_id(child))

            if item_json:
                children.append(item_json)

        return children

    def _get_parent_json(self, path):
        parent = os.path.dirname(path)
        folder_json = self.storage.get_json(self._get_id(parent))

        if folder_json is None or folder_json[KIND_FIELD] != DIRECTORY_KIND:
            raise ValueError("Cannot find the folder json!!!!!!!!!! " + parent)

        if len(self._get_children(parent)) < 3:
            try:
                folder_json = self._get_parent_json(parent)
            except ValueError:
                pass

        return folder_json

    def _generate_file_questions(self, path):
        file_name = os.path.basename(path)

        def check_response(response: str):
            if response.find(file_name) != -1:
                return False

            class_name = file_name.replace("_", "").lower()

            if response.lower().find(class_name) != -1:
                return False

            return True

        file_id = self._get_id(path)
        file_json = self.storage.get_json(file_id)

        if file_json is None or file_json[KIND_FIELD] != FILE_KIND:
            return

        #if QUESTIONS_FIELD in file_json:
        #    print("Found questions for the file " + path)
        #    return

        folder_desc = self._get_parent_json(path)[DESCRIPTION_FIELD]

        prompt1 = FILES_QUESTIONS_PROMPT.replace("[FILE_NAME]", file_name).\
            replace("[PROJECT_DESCRIPTION]", "This project is an augmented reality engine for Android devices.").\
            replace("[PARENT_FOLDER_DESCRIPTION]", folder_desc).replace("[SOURCES]", self._get_file_content(path))

        prompt2 = FILE_QUESTIONS_ADDITIONAL_PROMPT.replace("[FILE_NAME]", file_name)

        response = self.ai_core.get_1_or_2_steps_conversation_result(
            prompt1, prompt2, check_response, 2000)

        file_json[QUESTIONS_FIELD] = response
        self.storage.insert_json(file_id, file_json)