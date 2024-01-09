import os

from Gpt35AICore import Gpt35AICore
from KnowlegeGraph import KnowlegeGraph


ai_core = Gpt35AICore()
graph = KnowlegeGraph(ai_core)

graph.discover_project("/home/q548040/Downloads/ParallelWorld/ParallelWorld/")
graph.get_graph()