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


class KnowlegeGraph:
    def __init__(self, ai_core):
        self.storage: JSONDataStorage = None
        self.files_info = dict()
        self.dirs_info = dict()
        self.ai_core = ai_core
        self.code_suffices = {"cpp", "h", "hpp", "java", "py"}
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

    @staticmethod
    def _get_max_answer_tokens_count(file_size):
        return file_size // 10

    @staticmethod
    def _get_id(path):
        return path

    @staticmethod
    def _create_json_for_path(path, kind, description):
        return {"path": path, "kind": kind, "description": description}

    def _process_dir(self, path, children):
        dir_id = self._get_id(path)

        if self.storage.get_json(dir_id):
            print("Info was found for the directory: " + path)
            return dir_id

        files_descriptions = []

        for child in children:
            obj_json = self.storage.get_json(self._get_id(child))

            if json is None:
                raise RuntimeError("Not found ID which must be presented!")

            files_descriptions.append(obj_json["kind"] + " " + os.path.basename(child) + ": " +
                                      obj_json["description"] + "\n\n")

        descriptions = "".join(files_descriptions)
        dir_name = os.path.basename(path)

        prompt = DIRECTORY_SUMMARY_PROMPT.replace("[FILES_DESCRIPTION]", descriptions).\
            replace("[DIRECTORY_NAME]", dir_name)

        response = self.ai_core.get_response(prompt, 1500)
        response = response[len(prompt):]
        response = response.replace("\n\n", "\n")
        self.storage.insert_json(dir_id, self._create_json_for_path(path, "Directory", response))
        return dir_id

    def _process_file(self, path):
        suffix = pl.Path(path).suffix.lower()[1:]
        is_code = suffix in self.code_suffices
        is_doc = suffix in self.doc_suffices
        print(suffix)

        if not is_code and not is_doc:
            raise ValueError()

        file_id = self._get_id(path)

        if self.storage.get_json(file_id):
            print("Info was found for the file: " + path)
            return file_id

        with open(path) as f:
            text = f.read(MAX_SYMBOLS_TO_READ)

            if not text.strip():
                raise ValueError()

            response = self.ai_core.get_response(FILE_SUMMARY_PROMPT + text, 1500)
            response = response[len(FILE_SUMMARY_PROMPT):]
            response = response.replace("\n\n", "\n")
            self.storage.insert_json(file_id, self._create_json_for_path(path, "File", response))

        return file_id
