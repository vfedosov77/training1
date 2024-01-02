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
"""We are creating a structured knowledge base for a software project. 
User will use it to get answers for question about the project. 
Now we are creating the set of possible questions of users to have something like a big FAQ in our base. 
Those questions we must imagine on our own.
We are making a list of possible questions for the details in each source file of project.
I am providing the full code of the file [FILE_NAME], along with brief descriptions of its parent folder as generated 
in the previous step. Here is the full code of [FILE_NAME]:

[SOURCES]


The whole project description: '[PROJECT_DESCRIPTION]'

And here are the summaries for the parent folder: [PARENT_FOLDER_DESCRIPTION]

Based on this information, generate a list of potential questions of the project user which can touch the details 
mentioned in the provided '[FILE_NAME]'. 
That can be any project-related question which answers require to see that file. 
Please take into account that user probably don't know about that file and the classes/functions inside of it. 
He probably has a related bug or he investigates the project asking questions. 
He can ask also questions related to extending or integrating the code, or any specific implementation details etc."""

FILE_QUESTIONS_ADDITIONAL_PROMPT = """as I told user is not familiar with the file name and the classes/functions names 
of that file - he knows the project and he only starts to investigate the implementation. But please generate only 
questions regarding the info which is included in '[FILE_NAME]' - not mentioning the name of the file. """

GROUP_QUESTIONS_PROMPT = """I need to organize a set of questions related to a software development project into major 
topics. The whole project description: '[PROJECT_DESCRIPTION]'

Please analyze these questions and identify a few major topics (2-7) that they broadly fall under. 
This will help in categorizing and referencing the questions efficiently. Only topics names are required.
Topic names should be as atomistic as possible.
Below is the list of questions:

[QUESTIONS_WITH_NUMBERS]

Please provide only a few atomic items - up to 7. 
Don't provide any info except the topic names which must be as short as possible.
Please format the result as:
 1. Topic1 
 2. Topic2 
 ..."""

TOPICS_QUSTIONS_PROMPT = """I need to organize a set of questions related to a software development project into major 
topics. I already found the topics and now I need to find all the related questions for each topic. 
Here is one of the topics:

'[TOPIC]'

Please provide the list the numbers of the questions that are relevant to that topic. Don't use ranges in the result.
These are the questions:

[QUESTIONS_WITH_NUMBERS]

Please mention only the questions which are clearly related to the topic.
Don't use ranges.
The result must be formatted as the comma separated numbers.
If there are any doubts that a question and the topic are related - don't include that question's number."""

ONLY_COMMA_SEPARATED_PROMPT = """Please write only comma separated numbers of questions in your response."""

ONLY_A_FEW_ITEMS_PROMPT = """Please write only up to 7 main topics in your response."""

TOPIC2SUBTOPICS_PROMPT = """I am creating a FAQ and I need to slit one big topic '[TOPIC]' into a few (2-5) subtopics. 
These topic contains the following questions:

[QUESTIONS_WITH_NUMBERS]

Please provide the list of the subtopics."""

TOPIC_FOR_QUESTION_PROMPT = """We are trying to find a related question in a big FAQ for a software project which is described as '[PROJECT_DESCRIPTION]'.
There are the following main topics in that FAQ:

[TOPICS_WITH_NUMBERS]

Which topic as of your understanding is the most relevant for the following question:

[QUESTION]

The result format as the number of the topic - it will be used by an automatic parser. Please write no any additional info."""

ONLY_NUMBER_PROMPT = "WRITE ONLY ONE NUMBER!!!"

