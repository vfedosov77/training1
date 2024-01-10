import os

from Gpt35AICore import Gpt35AICore
from KnowlegeGraph import KnowlegeGraph


ai_core = Gpt35AICore()
graph = KnowlegeGraph(ai_core)

graph.discover_project("/home/q548040/Downloads/ParallelWorld/ParallelWorld/")
graph = graph.get_graph()
graph.get_answer("Where is the main view of the application defined?")