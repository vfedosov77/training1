SHORT_DESCRIPTION_PROMPT = "Could you make this project [KIND] overview shorter -  a few sentences would be enough: "

FILE_SUMMARY_CONTEXT = \
""""You are a software developer of a software project which can be described as '[PROJECT_DESCRIPTION]'.
Your task is to write a short description for manager of each file in that project. 
Now you are working on the file [FILE_NAME] which is located in the directory [DIR_NAME]."""

FILE_SUMMARY_PROMPT = """Create short overview of the functionality for the following source code:

[SOURCES]
"""

DIRECTORY_SUMMARY_CONTEXT = \
""""You are a software developer of a software project which can be described as '[PROJECT_DESCRIPTION]'.
Your task is to write a short description for manager of each directory in that project. 
Now you are working on the directory [DIR_NAME].
The directory contains several items, each contributing to the broader functionality. 
Listed below is the directory content with minimal details:

[FILES_DESCRIPTION]
"""

DIRECTORY_SUMMARY_PROMPT = \
"""Integrate this content description to provide a holistic understanding of the directory's purpose and role,
rather than elaborating on individual file contents.
Don't mention each file - we need only the functionality of the whole directory. """


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
Take into account that a FAQ's user probably don't know about files/classes/functions names inside of that project. Therefore create topics for a user who is not aware of those names.
Now you are working with the folder of that project which is described as: '[PARENT_FOLDER_DESCRIPTION]'."""

FILES_QUESTIONS_PROMPT = \
"""###Instruction###
Which topics would you add into FAQ looking at the following sources of the file '[FILE_NAME]':


###Sources###
[SOURCES]


###Instruction###
Remember: In that FAQ only essential topics of the project functionality must be included.
Remember: a FAQ's user probably don't know about files/classes/functions names inside of that project. Therefore create topics for a user who is not aware of those names.
He probably has a related bug or he investigates the project asking questions. 
You should provide for each topic only one short sentence. 
Topics must have numbers."""

NO_NAMES_PROMPT = """Could you rewrite the following topics in such a way that they contain no class/files/directories and methods names and they must be not overlapped:
[TOPICS_WITH_NUMBERS]
"""

ONLY_NUMBER_PROMPT = "WRITE ONLY THE NUMBER"

ONLY_YES_NO_PROMPT = "Write only 'Yes' or 'No'"

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

# You will be penalized
