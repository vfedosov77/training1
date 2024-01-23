from ChatBot.Common.Configuration import Configuration, get_config

from langchain.embeddings import CacheBackedEmbeddings, HuggingFaceEmbeddings
from langchain.vectorstores import FAISS
from langchain.text_splitter import RecursiveCharacterTextSplitter, Document
from langchain.storage import LocalFileStore

from typing import List
import os

MODEL_ID = 'BAAI/bge-small-en-v1.5'
KEYWORD_INDEX_FILE_NAME = "keywords2.bin"
KEYWORD_CACHE_FILE_NAME = "keywords2.bin.store"


class EmbeddingChecker:
    def __init__(self):
        core_embeddings_model = HuggingFaceEmbeddings(model_name=MODEL_ID)
        store = LocalFileStore(os.path.join(get_config().get_project_path(), KEYWORD_CACHE_FILE_NAME))
        self.embedder = CacheBackedEmbeddings.from_bytes_store(core_embeddings_model,
                                                          store,
                                                          namespace=MODEL_ID)

        if os.path.exists(os.path.join(get_config().get_project_path(), KEYWORD_INDEX_FILE_NAME + ".faiss")):
            self.vector_store = FAISS.load_local(folder_path=get_config().get_project_path(),
                                                 index_name = KEYWORD_INDEX_FILE_NAME,
                                                 embeddings=self.embedder)
        else:
            self.vector_store = None

    def find_keyword_matches(self, keyword: str, count: int = 3):
        embeddings = self.embed_text(keyword)
        matches = self.vector_store.similarity_search_by_vector(embeddings, k=count, distance_threshold=0.0)
        return [m.page_content for m in matches]

    def embed_text(self, text: str):
        embedding_vector = self.embedder.embed_query(text)
        return embedding_vector

    def create_storage(self, keywords: List[str]):
        #assert self.vector_store is None, "Already created"
        documents = [Document(page_content=txt) for txt in keywords]
        text_splitter = RecursiveCharacterTextSplitter(chunk_size=500, chunk_overlap=200)
        esops_documents = text_splitter.transform_documents(documents)
        self.vector_store = FAISS.from_documents(esops_documents, self.embedder)
        self.vector_store.save_local(folder_path=get_config().get_project_path(), index_name=KEYWORD_INDEX_FILE_NAME)

