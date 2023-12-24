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
""""Generate a summary that captures the collective functionality and key themes of the directory [DIRECTORY_NAME], 
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


The whole project description: [PROJECT_DESCRIPTION]

And here are the summaries for the parent folder: [PARENT_FOLDER_DESCRIPTION]

Based on this information, generate a list of potential questions of the project user which can touch the details 
mentioned in the provided [FILE_NAME]. 
That can be any project-related question which answers require to see that file. 
Please take into account that user probably don't know about that file and the classes/functions inside of it. 
He probably has a related bug or he investigates the project asking questions. 
He can ask also questions related to extending or integrating the code, or any specific implementation details etc."""

FILE_QUESTIONS_ADDITIONAL_PROMPT = "as I told the user is not familiar with the file name and the classes/functions "
"names of that file - he knows the project and he only starts to investigate the implementation"
