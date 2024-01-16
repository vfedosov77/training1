FIND_DUPLICATIONS_CONTEXT = \
"""You are a software developer of a software project which can be described as: '[PROJECT_DESCRIPTION]'
You are creating FAQ of that project and now you are working on the part of questions which are inside of the main topic '[TOPIC_NAME]'.
In that topic there are too many questions and you want to remove one of them. 
Therefore you are looking for duplications between FAQ questions. 
If two questions are asking about the same thing from different points of view - then we need both of them: it can be useful for different users. 
Duplications must mean the same for a user of FAQ."""

FIND_DUPLICATIONS_PROMPT = \
"""Check the following questions if two of them mean completelly the same:

[QUESTIONS_WITH_NUMBERS] 

PROVIDE ONLY THE NUMBERS OF TWO DUPLICATED ITEMS if you see ones. Format the output as: (1, 2)"""