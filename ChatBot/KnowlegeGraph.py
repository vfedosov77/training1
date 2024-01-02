from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.QuestionsTree import QuestionsTree

import torch
import os
import pathlib as pl
import json

USER_PROMPT = f"context: ```{input}``` \n\n output: "

AI_REQUEST = ""
MAX_SYMBOLS_TO_READ = 25000
NODE_1 = "node_1"
NODE_2 = "node_2"
EDGE = "edge"

FILE_KIND = "File"
DIRECTORY_KIND = "Directory"
QUESTIONS_FIELD = "questions"
KIND_FIELD = "kind"
DESCRIPTION_FIELD = "description"
PATH_FIELD = "path"
BROCKEN_KIND = "brocken"

PROJECT_DESCRIPTION = "This project is an augmented reality engine for Android devices."

SHORT_FOLDER_DESCRIPTION_SIZE = 3000
SHORT_FILES_DESCRIPTION_SIZE = 5000


class KnowlegeGraph:
    def __init__(self, ai_core):
        self.storage: JSONDataStorage = None
        self.files_info = dict()
        self.dirs_info = dict()
        self.ai_core = ai_core
        self.code_suffices = {"cpp", "c", "h", "hpp", "java", "py"}
        self.doc_suffices = {"txt", "md"}
        self.tree = None

        #self.paths_to_fix = {"/content/drive/MyDrive/Sources/ParallelWorld/jni"}

    def get_graph(self):
        items = self.storage.get_all()
        questions = dict()

        for item in items:
            if QUESTIONS_FIELD in item:
                questions.update({question: item[PATH_FIELD] for question in item[QUESTIONS_FIELD]})

        self.tree = QuestionsTree(questions, self.ai_core, PROJECT_DESCRIPTION, self.storage)

    def discover_project(self, project_path: str):
        self.storage = JSONDataStorage(os.path.join(project_path, "knowledge.db"))

        def dfs(path):
            children = []

            for child in os.listdir(path):
                child_path = os.path.join(path, child)

                if "opencv-2.4.6.1" in child_path:
                    continue

                try:
                    if os.path.isdir(child_path):
                        children.append(dfs(child_path))
                    else:
                        children.append(self._process_file(child_path))
                except ValueError:
                    pass
                except RuntimeError as e:
                    torch.cuda.empty_cache()
                    print(f"Cannot process {child_path} because of the lack of the GPU memory")

                    self.storage.insert_json(self._get_id(child_path),
                                             self._create_json_for_path(child_path, BROCKEN_KIND, ""))
            try:
                return self._process_dir(path, children, False)
            except RuntimeError as e:
                torch.cuda.empty_cache()
                print(f"Cannot process {child_path} because of the lack of the GPU memory. Will try to create a "
                      f"shorter version of the request.")
                return self._process_dir(path, children, True)

        dfs(project_path)
        self._create_questions(project_path)

    @staticmethod
    def _parse_questions(questions_str):
        idx = questions_str.find("1. ")

        if idx == -1:
            print("Error!!!!!!!!!!!!!!!!!!!!: cannot find the first question")
            return []

        questions_str = questions_str[idx:]
        return [q.split(".")[1] for q in questions_str.split("\n") if q and str.isnumeric(q[0]) and 0 < q.find(".") < 3]

    def _create_questions(self, project_path):
        def dfs(path):
            for child in os.listdir(path):
                child_path = os.path.join(path, child)

                if os.path.isdir(child_path):
                    dfs(child_path)
                else:
                    suffix = pl.Path(child).suffix.lower()[1:]

                    if suffix in self.code_suffices:
                        try:
                            self._generate_file_questions(child_path)
                        except ValueError:
                            pass

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

    def _process_dir(self, path, children, make_short_request):
        dir_id = self._get_id(path)

        if not children:
            raise ValueError()

        print("Dir: " + path)

        dir_json = self.storage.get_json(dir_id)
        if dir_json:
            if dir_json[KIND_FIELD] == BROCKEN_KIND:
                make_short_request = True
            else:
                print("Info was found for the directory: " + path)
                return dir_id

        files_descriptions = []

        if make_short_request:
            descriptions = self._create_short_children_descriptions(children, files_descriptions)
        else:
            descriptions = self._create_children_descriptions(children, files_descriptions)

        dir_name = os.path.basename(path)

        prompt = DIRECTORY_SUMMARY_PROMPT.replace("[FILES_DESCRIPTION]", descriptions).\
            replace("[DIRECTORY_NAME]", dir_name)

        response = self.ai_core.get_short_conversation_result(prompt, 1500)
        self.storage.insert_json(dir_id, self._create_json_for_path(path, DIRECTORY_KIND, response))
        return dir_id

    def _create_children_descriptions(self, children, files_descriptions):
        for child in children:
            obj_json = self.storage.get_json(self._get_id(child))

            if json is None:
                raise RuntimeError("Not found ID which must be presented!")

            files_descriptions.append(obj_json[KIND_FIELD] + " '" + os.path.basename(child) + "': " +
                                      obj_json[DESCRIPTION_FIELD].replace("\n", "") + "\n\n")
        descriptions = "".join(files_descriptions)
        return descriptions

    def _create_short_children_descriptions(self, children, files_descriptions):
        max_simbols_per_file = SHORT_FILES_DESCRIPTION_SIZE // len(children)

        for child in children:
            obj_json = self.storage.get_json(self._get_id(child))

            if json is None:
                raise RuntimeError("Not found ID which must be presented!")

            desc = obj_json[DESCRIPTION_FIELD][:max_simbols_per_file].replace("\n", "")
            files_descriptions.append(obj_json[KIND_FIELD] + " '" + os.path.basename(child) + "': " +
                                      desc + "...\n\n")
        descriptions = "".join(files_descriptions)
        return descriptions

    @staticmethod
    def _get_file_content(path):
        try:
            with open(path) as f:
                text = f.read(MAX_SYMBOLS_TO_READ)

                if not text.strip():
                    raise ValueError()
        except UnicodeDecodeError:
            try:
                with open(path) as f:
                    text = f.read(-1)[:MAX_SYMBOLS_TO_READ]
            except UnicodeDecodeError:
                print("Cannot read the file content: " + path)
                raise ValueError()

        return text

    def _process_file(self, path):
        suffix = pl.Path(path).suffix.lower()[1:]
        is_code = suffix in self.code_suffices
        is_doc = suffix in self.doc_suffices

        if not is_code:# and not is_doc:
            raise ValueError()

        print("File: " + path)

        file_id = self._get_id(path)
        file_json = self.storage.get_json(file_id)

        if file_json:
            if file_json[KIND_FIELD] == BROCKEN_KIND:
                print("File was not parsed because of memory error previously: " + path)
                raise ValueError()

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

    def _generate_file_questions(self, path, short_request=False):
        file_name = os.path.basename(path)

        file_id = self._get_id(path)
        file_json = self.storage.get_json(file_id)

        if file_json is None or file_json[KIND_FIELD] != FILE_KIND:
            return

        #if QUESTIONS_FIELD in file_json:
        #    print("Found questions for the file " + path)
        #    return

        folder_desc = self._get_parent_json(path)[DESCRIPTION_FIELD]

        if short_request:
            folder_desc = folder_desc[:SHORT_FOLDER_DESCRIPTION_SIZE]

        prompt = FILES_QUESTIONS_PROMPT.replace("[FILE_NAME]", file_name).\
            replace("[PROJECT_DESCRIPTION]", PROJECT_DESCRIPTION).\
            replace("[PARENT_FOLDER_DESCRIPTION]", folder_desc).replace("[SOURCES]", self._get_file_content(path))
        try:
            response = self.ai_core.get_generated_text(prompt, 250)

            file_json[QUESTIONS_FIELD] = self._parse_questions(response)
            self.storage.insert_json(file_id, file_json)
        except RuntimeError as e:
            if not short_request:
                print("GPU memory issue - try a short version of the request.")
                torch.cuda.empty_cache()
                self._generate_file_questions(path, True)
            else:
                print("ERROR! GPU memory issue during the short request.")
                raise e

