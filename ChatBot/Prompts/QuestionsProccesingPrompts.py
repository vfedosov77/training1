FIND_DUPLICATIONS_CONTEXT = \
"""You are a software developer of a software project which can be described as: '[PROJECT_DESCRIPTION]'
You are creating FAQ of that project and now you are working on the part of questions which are inside of the main topic '[TOPIC_NAME]'.
In that topic there are too many questions and you want to remove one of them. 
Therefore you are looking for duplications between FAQ questions."""

FIND_DUPLICATIONS_PROMPT = \
"""Duplications must mean the same for a user of FAQ.
Check the following questions if two of them mean completelly the same:

[QUESTIONS_WITH_NUMBERS] 

PROVIDE ONLY THE NUMBERS OF TWO DUPLICATED ITEMS if you see ones. Format the output as: (1, 2)"""

CHECK_DUPLICATION_CONTEXT = """You are a software developer of a software project which can be described as: '[PROJECT_DESCRIPTION]'
You are creating FAQ of that project and now you are working on the part of questions which are inside of the main topic '[TOPIC_NAME]'.
In that topic there are too many questions and you should remove one of them. 
Therefore you are looking for duplications between FAQ questions. We must avoid overlapped questions in our FAQ because it is not convenient for the user."""

CHECK_DUPLICATION_PROMPT = """Check if questions are related to the same aspect and one of them can be removed from our FAQ:
1. [Question1]
2. [Question2]
Are these questions similar? Write Yes or No."""

GET_ITEM_TO_REMOVE_PROMPT = """Which of the following questions is better to leave in FAQ because it is more comprehensive?
1. [Question1]
2. [Question2]
Please write only the number."""