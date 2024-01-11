from ChatBot.Promts import *
from ChatBot.JSONDataStorage import JSONDataStorage
from ChatBot.QuestionsTree import QuestionsTree
from ChatBot.KeywordsIndex import KeywordsIndex
from ChatBot.Utils import *
from ChatBot.Constants import *
import torch
import os
import pathlib as pl
import json
from typing import List, Tuple

PROJECT_DESCRIPTION = "Augmented reality engine for Android devices."

SHORT_FOLDER_DESCRIPTION_SIZE = 3000
SHORT_FILES_DESCRIPTION_SIZE = 5000


class KnowlegeBase:
    def __init__(self, ai_core, callback=None):
        self.storage: JSONDataStorage = None
        self.files_info = dict()
        self.dirs_info = dict()
        self.ai_core = ai_core
        self.code_suffices = {"cpp", "c", "h", "hpp", "java", "py"}
        self.doc_suffices = {"txt", "md"}
        self.tree = None
        self.keywords = None
        self.callback = callback
        # self.paths_to_fix = {"/content/drive/MyDrive/Sources/ParallelWorld/jni"}

    def get_answer(self, question, chat_history: List[Tuple[str, str]]):
        context = ROOT_CONTEXT.\
            replace('[PROJECT_DESCRIPTION]', PROJECT_DESCRIPTION).\
            replace('[CHAT_LOG]', self._format_chat_history(chat_history))

        prompt = ROOT_PROMPT.replace('[QUESTION]', question)
        self._on_step("The way to answer will be selected...", "Context: " + context + "\nPrompt: " + prompt)
        result = self.ai_core.get_short_conversation_result(prompt, 2, context)
        result = get_idx_from_response(result)

        if result == 1:
            self._on_step("The sources investigations is required to answer.", None)
            return self.tree.get_answer(question)
        elif result == 4:
            self._on_step("The documentation investigations is required to answer.", None)
            self._on_step("The documentation investigations is not implemented yet.", None, SELECTED_TEXT)
            return None
        elif result == 2:
            self._on_step("The keywords index investigations is required to answer.", None)
            prompt = KEYWORD_PROMPT.replace('[QUESTION]', question)
            result = self.ai_core.get_short_conversation_result(prompt, 5, context)
            self._on_step("Looking for the keyword: " + result, prompt)
            results = []

            if context.find(result) != -1 or question.find(result) != -1:
                results = [item for item in self.keywords.find_keyword(result)]

            if len(results) > 20:
                self._on_step("Too many findings - the sources investigations is required to answer.", None)
                return self.tree.get_answer(question)

            for path in results:
                result = self.tree.check_file(path, question)

                if result:
                    return result
            self._on_step("No findings - the sources investigations is required to answer.", None)
            return self.tree.get_answer(question)
        elif result == 3:
            self._on_step("No sources/documentation investigation is required to answer.", None)
            prompt = IMMEDIATELY_PROMPT.replace('[QUESTION]', question)
            result = self.ai_core.get_short_conversation_result(prompt, 200, context)
            self._on_step(result, prompt, SELECTED_TEXT)
            return result, None

        self._on_step("Too many findings - the sources investigations is required to answer.", None, SELECTED_TEXT)
        return None, None

    def get_tree(self):
        items = self.storage.get_all()
        questions = dict()

        for item in items:
            if QUESTIONS_FIELD in item:
                questions.update({question: item[PATH_FIELD] for question in item[QUESTIONS_FIELD] if len(question) > 1})

        self.tree = QuestionsTree(questions, self.ai_core, PROJECT_DESCRIPTION, self.storage, self.callback)
        return self.tree

    def discover_project(self, project_path: str):
        self.storage = JSONDataStorage(os.path.join(project_path, "knowledge.db"))
        self.keywords = KeywordsIndex(project_path, self.code_suffices, self.callback)
        # self._clear_brocken()

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

                    self.storage.insert_json(get_file_id(child_path),
                                             self._create_json_for_path(child_path, BROKEN_KIND, ""))
            try:
                return self._process_dir(path, children, False)
            except RuntimeError as e:
                torch.cuda.empty_cache()
                print(f"Cannot process {child_path} because of the lack of the GPU memory. Will try to create a "
                      f"shorter version of the request.")
                return self._process_dir(path, children, True)

        dfs(project_path)
        self._create_questions(project_path)

    def _format_chat_history(self, history):
        return "".join(role + ": " + message + "\n" for role, message in history)

    def _on_step(self, short_name, description, kind=NORMAL_TEXT):
        if self.callback:
            self.callback(short_name, description, kind)

    def _clear_brocken(self):
        print("Broken will be cleaned")
        count = 0

        for id, item in self.storage.get_all_with_ids():
            if (KIND_FIELD in item and (item[KIND_FIELD] == BROKEN_KIND or item[KIND_FIELD] == "brocken")) or \
                    KIND_FIELD not in item and PATH_FIELD in item:
                self.storage.remove(id)
                count += 1

        print("Cleaned " + str(count) + " items")
        exit(0)

    def _create_questions(self, project_path):
        def dfs(path):
            for child in os.listdir(path):
                child_path = os.path.join(path, child)

                if "opencv-2.4.6.1" in child_path:
                    continue

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
    def _create_json_for_path(path, kind, description):
        return {PATH_FIELD: path, KIND_FIELD: kind, DESCRIPTION_FIELD: description}

    def _process_dir(self, path, children, make_short_request):
        dir_id = get_file_id(path)

        if not children:
            raise ValueError()

        print("Dir: " + path)

        dir_json = self.storage.get_json(dir_id)
        if dir_json:
            if dir_json[KIND_FIELD] == BROKEN_KIND:
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
            obj_json = self.storage.get_json(get_file_id(child))

            if json is None:
                raise RuntimeError("Not found ID which must be presented!")

            files_descriptions.append(obj_json[KIND_FIELD] + " '" + os.path.basename(child) + "': " +
                                      obj_json[DESCRIPTION_FIELD].replace("\n", "") + "\n\n")
        descriptions = "".join(files_descriptions)
        return descriptions

    def _create_short_children_descriptions(self, children, files_descriptions):
        max_simbols_per_file = SHORT_FILES_DESCRIPTION_SIZE // len(children)

        for child in children:
            obj_json = self.storage.get_json(get_file_id(child))

            if json is None:
                raise RuntimeError("Not found ID which must be presented!")

            desc = obj_json[DESCRIPTION_FIELD][:max_simbols_per_file].replace("\n", "")
            files_descriptions.append(obj_json[KIND_FIELD] + " '" + os.path.basename(child) + "': " +
                                      desc + "...\n\n")
        descriptions = "".join(files_descriptions)
        return descriptions

    def _process_file(self, path):
        suffix = pl.Path(path).suffix.lower()[1:]
        is_code = suffix in self.code_suffices
        is_doc = suffix in self.doc_suffices

        if not is_code:# and not is_doc:
            raise ValueError()

        print("File: " + path)

        file_id = get_file_id(path)
        file_json = self.storage.get_json(file_id)

        if file_json:
            if file_json[KIND_FIELD] == BROKEN_KIND:
                print("File was not parsed because of memory error previously: " + path)
                raise ValueError()

            print("Info was found for the file: " + path)
            return file_id

        text = get_file_content(path)
        response = self.ai_core.get_short_conversation_result(FILE_SUMMARY_PROMPT + text, 1500)
        self.storage.insert_json(file_id, self._create_json_for_path(path, FILE_KIND, response))

        return file_id

    def _get_children(self, path):
        children = []

        for child in os.listdir(path):
            child_path = os.path.join(path, child)
            item_json = self.storage.get_json(get_file_id(child_path))

            if item_json:
                children.append(item_json)

        return children

    def _get_parent_json(self, path):
        parent = os.path.dirname(path)
        folder_json = self.storage.get_json(get_file_id(parent))

        if folder_json is None or folder_json[KIND_FIELD] != DIRECTORY_KIND:
            raise ValueError("Cannot find the folder json!!!!!!!!!! " + parent)

        if len(self._get_children(parent)) < 3:
            try:
                folder_json = self._get_parent_json(parent)
            except ValueError:
                pass

        return folder_json

    def _get_tokens_count(self, file_content):
        file_len = len(file_content)

        if file_len < 500:
            return 100

        if file_len < 2000:
            return 200

        if file_len < 3000:
            return 300

        return 350

    def _get_generated_questions(self, file_name, file_content, folder_desc):
        tokens_count = self._get_tokens_count(file_content)

        if self.ai_core.is_generation_preferred():
            prompt = "1. " + FILES_QUESTIONS_GENERATOR_PROMPT.replace("[FILE_NAME]", file_name). \
                replace("[PROJECT_DESCRIPTION]", PROJECT_DESCRIPTION). \
                replace("[PARENT_FOLDER_DESCRIPTION]", folder_desc).replace("[SOURCES]", file_content)

            response = self.ai_core.get_generated_text(prompt, tokens_count)
        else:
            context = FILES_QUESTIONS_CONTEXT.replace("[PROJECT_DESCRIPTION]", PROJECT_DESCRIPTION).\
                replace("[PARENT_FOLDER_DESCRIPTION]", folder_desc)

            prompt = "1. " + FILES_QUESTIONS_PROMPT.replace("[FILE_NAME]", file_name). \
                replace("[PROJECT_DESCRIPTION]", PROJECT_DESCRIPTION). \
                replace("[PARENT_FOLDER_DESCRIPTION]", folder_desc).replace("[SOURCES]", file_content)

            response = self.ai_core.get_short_conversation_result(prompt, tokens_count, context)

        return response

    def _generate_file_questions(self, path, short_request=False):
        file_name = os.path.basename(path)

        file_id = get_file_id(path)
        file_json = self.storage.get_json(file_id)

        if file_json is None or file_json[KIND_FIELD] != FILE_KIND:
            return

        if QUESTIONS_FIELD in file_json and isinstance(file_json[QUESTIONS_FIELD], list):
            print("Found questions for the file " + path)
            return

        folder_desc = self._get_parent_json(path)[DESCRIPTION_FIELD]

        if short_request:
            folder_desc = folder_desc[:SHORT_FOLDER_DESCRIPTION_SIZE]

        try:
            response = self._get_generated_questions(file_name, get_file_content(path), folder_desc)
            questions = parse_numbered_items(response)
            print("Parsed: " + str(questions))
            file_json[QUESTIONS_FIELD] = questions
            self.storage.insert_json(file_id, file_json)
        except RuntimeError as e:
            if not short_request:
                print("GPU memory issue - try a short version of the request.")
                torch.cuda.empty_cache()
                self._generate_file_questions(path, True)
            else:
                print("ERROR! GPU memory issue during the short request.")
                raise e

