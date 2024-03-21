import email.message

from ChatBot.AI.GenextGpt35AiCore import GenextGpt35AiCore
from ChatBot.KnowledgeBase import KnowledgeBase
from ChatBot.UI.Dialog import Dialog
from ChatBot.Common.Configuration import *
from ChatBot.Common.NotificationDispatcher import NotificationDispatcher
from ChatBot.JSONDataStorage import JSONDataStorage

from flask import Flask, request, render_template_string, session
import os, sys


def usage():
    print(
        """Usage: python chat_web_service.py PATH_TO_PROJECT""")
    exit(-1)


if len(sys.argv) != 2:
    usage()

path = sys.argv[1]

if not os.path.exists(path) or not os.path.isdir(path):
    print("Cannot find project directory: " + path)
    usage()

db_path = os.path.join(path, DB_FILE_NAME)

if not os.path.exists(db_path):
    print("Cannot find project index file: " + db_path + ". Please execute index.py script to create the index.")
    exit(-1)

temp_storage = JSONDataStorage(db_path)
project_description = temp_storage.get_json(PROJECT_DESCRIPTION_ID)
folders_to_exclude = temp_storage.get_json(FOLDERS_TO_EXCLUDE_ID)
assert project_description, "Index is broken"
set_app_config(Configuration(path, project_description, folders_to_exclude))
del temp_storage

ai_core = GenextGpt35AiCore()

# Web app
app = Flask(__name__)
app.secret_key = 'ZIYmUTf3WmQBfXbUnLZsOA'

HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Simple Chatbot</title>
    <script>
        async function fetchDocumentsAndSendBack() {
            const question = document.getElementById('question').value;
            // Send the question to the server to get URLs
            const answerResponse = await fetch('/process-question', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({question: question}),
            });
            const answer = await answerResponse.text();

            // Display the answer
            document.getElementById('answer').innerText = answer;
        }
    </script>
</head>
<body>
    <h2>Ask a Question</h2>
    <input type="text" id="question" name="question" size="50"><br><br>
    <button onclick="fetchDocumentsAndSendBack()">Send</button>
    <h3>Answer:</h3>
    <p id="answer"></p>
</body>
</html>
"""

KNOWLEDGE_BASE_KEY = 'knowledge_base'


def get_answer(question):
    #if KNOWLEDGE_BASE_KEY not in session:
    #   session[KNOWLEDGE_BASE_KEY] = base
    with  JSONDataStorage(db_path) as temp_storage:
        dispatcher = NotificationDispatcher()
        base = KnowledgeBase(ai_core, dispatcher, temp_storage)
        base.open_project()
        res, path = base.get_answer(question, [])
        return res, path


@app.route('/', methods=['GET'])
def chat():
    return render_template_string(HTML_TEMPLATE)


@app.route('/process-question', methods=['GET', 'POST'])
def process_documents():
    data = request.json
    question = data['question']
    try:
        answer = get_answer(question)

        if answer is None or answer[0] is None:
            answer = "An unknown error occurred while processing your request."
    except Exception as e:
        answer = "An error occurred while processing your request: " + str(e)

    return answer


if __name__ == '__main__':
    app.run(debug=True)
