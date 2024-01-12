DOC_RELATIONSHIPS_PROMPT = (
    "You are a network graph maker who extracts terms and their relations from a documentation file "
    "of a software project. You are provided with a file and your task is to extract the ontology "
    "of terms mentioned in the given context. These terms should represent the key concepts as per the context. \n"
    "Thought 1: While traversing through each sentence, Think about the key terms mentioned in it.\n"
        "\tTerms may include object, software service, class, interface, organization, team, person, \n"
        "\tcondition, protocol, acronym, documents, concept etc.\n"
        "\tTerms should be as atomistic as possible\n\n"
    "Thought 2: Think about how these terms can have one on one relation with other terms.\n"
        "\tTerms that are mentioned near each other are typically related to each other.\n"
        "\tTerms can be related to many other terms\n\n"
    "Thought 3: Find out the relation between each such related pair of terms. \n\n"
    "Format your output as a list of json. Each element of the list contains a pair of terms and the "
    "relation between them. Try to put at the first place the main term in the relation - some object which uses or "
    "contains another object for example if it is possible. Format them like the following: \n"
    "# JSON"
    "[\n"
    "   {\n"
    '       "node_1": "A concept from extracted ontology",\n'
    '       "node_2": "A related concept from extracted ontology",\n'
    '       "edge": "relationship between the two concepts, node_1 and node_2 in one or two sentences"\n'
    "   }, {...}\n"
    "]\n"
)

FILE_SUMMARY_PROMPT = \
    """"Based on the following content, provide a concise summary highlighting the main purpose, 
    key functionalities, and any notable features or methods. Here is the content: 
    """

DIRECTORY_SUMMARY_PROMPT = \
""""Generate a summary that captures the collective functionality and key themes of the directory '[DIRECTORY_NAME]', 
focusing on the overall role and contribution of the directory in the project context. 
The directory contains several files, each contributing to the broader functionality. 
Listed below are the files with minimal details:

[FILES_DESCRIPTION]

Integrate these themes or functionalities to provide a holistic understanding of the directory's purpose and role,
rather than elaborating on individual file contents.
Don't mention each file - we need only the functionality of the whole directory."""

FILES_QUESTIONS_GENERATOR_PROMPT = \
"""I am working on a project which can be described as '[PROJECT_DESCRIPTION]'. 
In that project there is a folder which can be described as '[PARENT_FOLDER_DESCRIPTION]'.
Here is the full code of one of the file of that folder "[FILE_NAME]":

[SOURCES]

I was forced to analyze the code of that file because it is important for the understanding of the following essential topics of the project functionality (I am providing only a short description here):
1. """

FILES_QUESTIONS_CONTEXT = """You are creating a big FAQ for a software project which can be described as '[PROJECT_DESCRIPTION]. 
FAQ will be used by software developers and managers.
In that FAQ only essential topics of the project functionality must be included.
Now you are working with the folder of that project which is described as: '[PARENT_FOLDER_DESCRIPTION]'."""

FILES_QUESTIONS_PROMPT = \
"""Which topics would you add into FAQ looking at the following sources of the file '[FILE_NAME]':

[SOURCES]



Take into account that a FAQ's user probably don't know about that file and the classes/functions inside of it.
He probably has a related bug or he investigates the project asking questions. You should provide for each topic only one short sentence. Also add the number at the beginning."""


GROUP_QUESTIONS_PROMPT = """I am creating FAQ for a software project which can be described as '[PROJECT_DESCRIPTION]'
The project source code contains functionality related to the following essential topics:
[QUESTIONS_WITH_NUMBERS]
Now I need to unite those topics into bigger items which will be the main items of FAQ - I plan to have 7 main items.
I see the following main items:
"""

TOPICS_QUESTIONS_PROMPT = """I need to organize a set of questions related to a software development project into major 
topics. The project can be described as '[PROJECT_DESCRIPTION]'.
I already found the topics and now I need to find all the related questions for each topic. 
Here is one of the topics:

'[TOPIC]'
 
These are the questions to distribute by topics:

[QUESTIONS_WITH_NUMBERS]

I am completely sure that the questions with the following numbers are clearly related to the topic:
"""

TOPIC2SUBTOPICS_PROMPT = """I am creating a FAQ and I need to slit one big topic '[TOPIC]' into a few (2-5) subtopics. 
These topic contains the following questions:

[QUESTIONS_WITH_NUMBERS]

Provide the list of the subtopics."""

TOPIC_FOR_QUESTION_PROMPT = """We have the following main topics in our FAQ for the software project "[PROJECT_DESCRIPTION]":
[TOPICS_WITH_NUMBERS]
Your subtopic "[QUESTION]" I would put into the main topic number """

###############################################################################################

WHICH_TOPIC_IS_CLOSEST_CONTEXT = """You are a software developer of a software project which can be described as:
###Project description###
'[PROJECT_DESCRIPTION]'

###Instruction###
This project is big and you are not familiar with all the parts of the project. 
But there is FAQ for that project which can help you to find quickly all the required info about the project.
This FAQ is structured and all the questions there are split into topics."""

WHICH_TOPIC_IS_CLOSEST_PROMT = """You need an answer for the question:
'[QUESTION]'


Which of the following FAQ topics can contain the answer:
[TOPICS_WITH_NUMBERS]


PROVIDE ONLY THE NUMBER of the topic."""

CHECK_THE_FILE_CONTEXT = """You are a software developer of a software project which can be described as '[PROJECT_DESCRIPTION]'.
This project is big and you are not familiar with all the parts of the project.
Now you are working with one of the source files of a folder of that project which is described as: '[PARENT_FOLDER_DESCRIPTION]'."""

CHECK_THE_FILE_PROMPT = """Your task is to find a place in the code which is related to the question:
[QUESTION]
 

###Instruction###
If there is such a place then write the name of the most related item (class, method, function, variable etc.). If the file contains no code which is closely related to the question - write '__NOTHING__' in your answer.
This is the sources of the file '[FILE_NAME]':


[SOURCES]


So the question is: [QUESTION]


Write in your message __NOTHING__ keyword if you think that the code contains no any closely related to the question code. If the code is related - don't write this keyword.
"""

ONLY_RELATED_ITEM_NAME_PROMPT = "WRITE ONLY THE NAME OF THE ITEM"

ONLY_NUMBER_PROMPT = "WRITE ONLY THE NUMBER"

# Split topic: I am creating a FAQ for a software project (Augmented Reality Engine) and the topic "Native Code Integration" in that FAQ has too many questions. I would like to split it into 3-5 parts. Which parts would you suggest if I have the following questions in that topic:

#############################################################################
ROOT_CONTEXT = """You are a software developer of a software project which can be described as '[PROJECT_DESCRIPTION]'.
You know not too good the code of the project but there is a big FAQ with almost all the info about the code and a separated FAQ for the documentation (now it is not too good).
There is also a project keywords index where you can find a file which uses some keyword. But it contains only project-specific names (classes, methods etc)
Your colleague asked you a few questions about the project. This is the history of the communication:
[CHAT_LOG]
"""

ROOT_PROMPT = """Which one of the following actions would you prefer to answer the following question of your colleague '[QUESTION]':
1. See source code of the project.
2. See the project keywords index.
3. Can provide the answer immediately - no further investigation is needed.
WRITE ONLY THE NUMBER OF ACTION"""
#4. See the documentation
#Only one word can be checked - so you must be sure.

KEYWORD_PROMPT = """Which keyword in the keywords index would you find to answer the following question of your colleague '[QUESTION]':
WRITE ONLY THE KEYWORD"""

IMMEDIATELY_PROMPT = """Answer the following question of the colleague: '[QUESTION]'"""
