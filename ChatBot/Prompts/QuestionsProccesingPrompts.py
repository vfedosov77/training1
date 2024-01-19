FIND_DUPLICATIONS_CONTEXT = \
"""You are a software developer of a software project which can be described as: '[PROJECT_DESCRIPTION]'
You are creating FAQ of that project and now you are working on the part of questions which are inside of the main topic '[TOPIC_NAME]'.
In that topic there are too many questions and you want to remove one of them. 
Therefore you are looking for duplications between FAQ questions.
If two questions are related to the same functionality aspects but different files/classes - it is OK. We can stll unite them and provoide links for the files/classes."""

FIND_DUPLICATIONS_PROMPT = \
"""Duplications must mean the same for a user of FAQ.
Check the following questions if two of them mean completely the same:

[QUESTIONS_WITH_NUMBERS] 

PROVIDE ONLY THE NUMBERS OF TWO DUPLICATED ITEMS if you see ones. Format the output as: (1, 2)"""

CHECK_DUPLICATION_CONTEXT = """You are a software developer of a software project which can be described as: '[PROJECT_DESCRIPTION]'
You are creating FAQ of that project and now you are working on the part of questions which are inside of the main topic '[TOPIC_NAME]'.
In that topic there are too many questions and you should remove one of them. 
Therefore you are looking for duplications between FAQ questions. We must avoid overlapped questions in our FAQ because it is not convenient for the user."""

CHECK_DUPLICATION_PROMPT = """Check if questions are for sure related to the same aspect and we can unite them in our FAQ:
1. [Question1]
2. [Question2]
If these questions are related to the same functionality aspects but different files/classes - it is OK. We can unite them and mention both classes or files.
Are these questions similar? Write Yes or No."""

GET_ITEM_TO_REMOVE_PROMPT = """Unite these 2 topics into 1 common topic which covers both of them:
1. [Question1]
2. [Question2]
The result must be one sentence. Format the result as:
RESULT: The short topic description
"""

