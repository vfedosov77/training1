from ChatBot.Promts import *

from dataclasses import dataclass
import networkx as nx
import os
import pathlib as pl
import json


USER_PROMPT = f"context: ```{input}``` \n\n output: "

AI_REQUEST = ""
MAX_SYMBOLS_TO_READ = 50000
NODE_1 = "node_1"
NODE_2 = "node_2"
EDGE = "edge"

@dataclass
class EdgeInfo:
    node1: str
    node2: str
    relations: set
    count: int


class KnowlegeGraph:
    def __init__(self, ai_core):
        self.graph = nx.Graph()
        self.files_info = dict()
        self.edges_info = dict()
        self.ai_core = ai_core
        self.code_suffices = {"cpp", "h", "hpp", "java", "py"}
        self.doc_suffices = {"txt", "md"}

    def discover_project(self, path: str):
        for root, dirs, files in os.walk(path):
            for file in files:
                file_path = os.path.join(root, file)
                print(file_path)
                self._process_file(file_path)

        self._create_graph()

    def get_graph(self):
        return self.graph

    @staticmethod
    def _get_edge_key(self, node1, node2):
        #if node1 >= node2:
        #    return node1 + "###" + node2

        return node1 + "->" + node2

    def _add_ontology_items(self, items):
        for item in items:
            if len(item) != 3:
                print("Wrong fields count: " + str(item))

            if NODE_1 in item and NODE_2 in item and EDGE in item:
                node1 = item[NODE_1]
                node2 = item[NODE_2]
                relation = item[EDGE]
                key = self._get_edge_key(node1, node2)

                if key in self.edges_info:
                    info = self.edges_info[key]
                    info.count += 1
                    info.relations.add(relation)
                else:
                    info = EdgeInfo(node1, node2, {relation}, 1)
                    self.edges_info[key] = info

    @staticmethod
    def _get_max_answer_tokens_count(self, file_size):
        return file_size // 3

    def _process_ontology(self, text):
        response = self.ai_core.get_response(CODE_SYS_PROMPT, text, self._get_max_answer_tokens_count(len(text)))
        try:
            items = json.loads(response)
            self._add_ontology_items(items)
        except:
            print("Wrong response: " + response)

    def _process_file(self, path):
        suffix = pl.Path(path).suffix.lower()[1:]
        is_code = suffix in self.code_suffices
        is_doc = suffix in self.doc_suffices
        print(suffix)
        if not is_code and not is_doc:
            return

        with open(path) as f:
            text = f.read(MAX_SYMBOLS_TO_READ)
            self._process_ontology(text, )

    def _create_graph(self):
        for _, edge in self.edges_info:
            self.graph.add_edge(
                edge.node1,
                edge.node2,
                title=str(edge.relations),
                weight=edge.code
            )
