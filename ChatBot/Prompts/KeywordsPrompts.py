GET_KEYWORDS_CONTEXT = """We are making keywords index for a software project '[PROJECT_DESCRIPTION]'.
I have '[FILE_NAME]' file from that project. """

GET_KEYWORDS_PROMPT = """Here's a snippet of the code: 
[SOURCES]


Can you identify and list the keywords from this code? Focus on unique functions, class names, and any domain-specific terms. Ignore standard library functions and programming language keywords. Only project specific functionlity aspects are interesting. 
KEYWORDS MUST BE CLEAR FOR ALL SOFTWARE DEVELOPERS. 
Don't use variables and method's/function's names but their functionality instead. 
Don't use variables and method's/function's names but their functionality instead. 
Keywords must written in natural language and must be clear for all the developers - even if they did not work with that project.
For example instead of variable "nCount", use "Object count" keyword instead. And instead of "getServiceInterface()" use "Service interface processing" keywords.
Format the result as:
1. Keyword1
2. Keyword2
..."""

KEYPOINTS_FROM_QUESTION_CONTEXT = """You are new developer on a software project which is '[PROJECT_DESCRIPTION]'. 
And you are must find any info regarding the a topic and find the corresponding code part. 
There is a keywords index on that project which allows to find corresponding source file by a keyword."""

KEYPOINTS_FROM_QUESTION_PROMPT = """Which keywords would you use for the topic:
[QUESTION]
Before to write a keyword take into account if it allows to reduce the amount of corresponding source files which. 
If some keyword is not able to make the search narrow - don't write it.
Format the result as:
1. Keyword1
2. Keyword2
..."""

################################Documents###########################

GET_DOCUMENT_KEYWORDS_PROMPT = """Here's the content of that document: 
###Document content###
[SOURCES]


###Instruction###
Can you identify and list the keywords from this document? Focus on unique information of that document ignoring well-known facts. Only project project specific aspects are interesting. 
KEYWORDS MUST BE CLEAR FOR ALL SOFTWARE DEVELOPERS. 
Keywords must written in natural language and must be clear for all the developers - even if they did not work with that project.
Format the result as:
1. Keyword1
2. Keyword2
..."""