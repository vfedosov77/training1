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
    key functionalities, and any notable features or methods. Here is the content: """

DIRECTORY_SUMMARY_PROMPT = \
""""Generate a summary that captures the collective functionality and key themes of the directory '[DIRECTORY_NAME]', 
focusing on the overall role and contribution of the directory in the project context. 
The directory contains several files, each contributing to the broader functionality. 
Listed below are the files with minimal details:

[FILES_DESCRIPTION]

Please integrate these themes or functionalities to provide a holistic understanding of the directory's purpose and role, 
rather than elaborating on individual file contents. 
Please don't mention each file - we need only the functionality of the whole directory."""

FILES_QUESTIONS_PROMPT = \
"""I am working on a project which can be described as '[PROJECT_DESCRIPTION]'. 
In that project there is a folder which can be described as '[PARENT_FOLDER_DESCRIPTION]'.
Here is the full code of one of the file of that folder "[FILE_NAME]":

[SOURCES]

I was forced to analyze the code of that file because it is important for the understanding of the following essential topics of the project functionality (I am providing only short names here):
"""

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

Please provide the list of the subtopics."""

TOPIC_FOR_QUESTION_PROMPT = """We have the following main topics in our FAQ for the software project "[PROJECT_DESCRIPTION]":
[TOPICS_WITH_NUMBERS]
Your subtopic "[QUESTION]" I would put into the main topic number """


