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

CHECK_THE_FILE_PROMPT = """###Instruction###
Your task is to find a place in the code which is related to the question: '[QUESTION]'
If there is such a place then write the name of the most related item (class, method, function, variable etc.). If the file contains no code which is closely related to the question - write '__NOTHING__' in your answer.
This is the sources of the file '[FILE_NAME]':

###Sources###
[SOURCES]


###Instruction###
So the question is: [QUESTION]

Write in your message __NOTHING__ keyword if you think that the code contains no any closely related to the question code. If the code is related - don't write this keyword.
"""

ONLY_RELATED_ITEM_NAME_PROMPT = "WRITE ONLY THE NAME OF THE ITEM"

##################################Documents################################

CHECK_DOCUMENT_CONTEXT = """You are a software developer of a software project which can be described as '[PROJECT_DESCRIPTION]'.
This project is big and you are not familiar with all the parts of the project.
Now you are working documentation of that project to find an answer for a question."""

CHECK_DOCUMENT_PROMPT = """###Instruction###
Your task is to find a place in the document which is related to the question: '[QUESTION]'
If there is such a place then write shortly the answer. If the file contains no code which is closely related to the question - write '__NOTHING__' in your answer.
This is the content of the document: '[FILE_NAME]':

###Document content###
[SOURCES]


###Instruction###
So the question is: [QUESTION]

Write in your message __NOTHING__ keyword if you think that the document contains no any closely related to the question info. If some info is related - don't write this keyword.
"""