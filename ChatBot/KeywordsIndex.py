import os, sys

class KeywordsIndex:
    def __init__(self, path, code_files_suffices, callback):
        self.path = path
        self.code_files_suffices: set = code_files_suffices
        self.callback = callback

    def find_keyword(self, keyword: str):
        # TODO: temporary approach - the index is required here
        for root, dirs, files in os.walk(self.path, topdown=False):
            for name in files:
                if name.split(".")[-1] in self.code_files_suffices:
                    path = os.path.join(root, name)
                    try:
                        with open(path)  as f:
                            content = f.read(30000)

                            if content.find(keyword) != -1:
                                yield path
                    except Exception:
                        pass

