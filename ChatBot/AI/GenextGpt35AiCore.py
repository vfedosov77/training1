from ChatBot.Common.Constants import *

from ChatBot.AI.AiCoreBase import AiCoreBase
import requests
import json
import time
from http import HTTPStatus
from requests.exceptions import HTTPError
import logging


logging.getLogger().setLevel(logging.INFO)
logging.captureWarnings(True)

POLLING_INTERVAL = 2

CONFIG = {
    "PROD": {
        "HOSTNAME": "api.gcp.cloud.bmw",
        "CLIENT_ID": "c5da1774-3f9d-4b0f-bbc6-9f2b9c904657", #a9935f04-14d5-4e25-839f-7aec4532b7e3",
        "CLIENT_SECRET": "Tts5VQeqBZYOKC5184i8aWCNPNNVSTfATwdzfgED",
        "API_KEY": "FW2f1PERkGGo6trjYesWthXon6SFXK9bI9b3wru1cIewws8V",
    },
}

ENVIRONMENT = "PROD"


# Get access token
def get_webeam_access_token(requests_session: requests.Session) -> str:
    """Get access token from auth server using OAuth2 client credentials flow.

    Returns:
        str: Access token if authentication succeeded, else False.

    """

    # Make request to get access token
    auth_endpoint = f"https://auth{'-i' if ENVIRONMENT != 'PROD' else ''}.bmwgroup.net/auth/oauth2/realms/root/realms/machine2machine/access_token"

    auth_response = requests_session.post(
        auth_endpoint,
        headers={"Content-Type": "application/x-www-form-urlencoded"},
        data={
            "grant_type": "client_credentials",
            "client_id": CONFIG[ENVIRONMENT]["CLIENT_ID"],
            "client_secret": CONFIG[ENVIRONMENT]["CLIENT_SECRET"],
            "scope": "machine2machine",
        },
    )

    # Check response status code
    if auth_response.status_code != HTTPStatus.OK:
        # raise if request failed
        raise Exception(
            "OAuth2 authentication failed. HTTP status code: {auth_response.status_code}."
        )
    else:
        # Get access token from response
        access_token = auth_response.json().get("access_token")
        logging.info("Successfully received WEN token.")
        return access_token


# Helpers
def get_generate_chat_request(requests_session: requests.Session, request_id: str):
    # Make the GET request
    response = requests_session.get(
        f"https://{CONFIG[ENVIRONMENT]['HOSTNAME']}/generaid/llm/v1/tenant_id/text-prediction/generate-chat-request/{request_id}",
    )
    return response.json()


def post_generate_chat_request(requests_session: requests.Session, payload):
    # Make the POST request
    response = requests_session.post(
        f"https://{CONFIG[ENVIRONMENT]['HOSTNAME']}/generaid/llm/v1/tenant_id/text-prediction/generate-chat-request",
        headers={"Content-Type": "application/json"},
        data=json.dumps(payload),
    )
    return response.json()["request_id"]


def poll_get_generate_chat_request(requests_session: requests.Session, request_id: str):
    logging.info(f"Start polling for request with ID {request_id}")
    start_time = time.time()
    count = 0
    while count < 100:
        response = get_generate_chat_request(requests_session, request_id)
        logging.info(f"Received polling response:\n{json.dumps(response, indent=2)}")

        if response["status"] != "PENDING":
            duration = time.time() - start_time
            logging.info(f"Finished polling after {duration:.2f} seconds.")
            return response

        count += 1
        time.sleep(POLLING_INTERVAL)

    raise Exception("Polling took too long, aborting")


class GenextGpt35AiCore(AiCoreBase):
    def __init__(self):
        self.session = requests.Session()
        self.proxies = {"http": "", "https": ""}
        self.hooks = {"response": lambda r, *args, **kwargs: r.raise_for_status()}

    def is_generation_preferred(self):
        return False

    def get_generated_text(self, prompt: str, max_answer_tokens) -> str:
        raise NotImplementedError()

    def get_conversation_result(self, callback, max_answer_tokens, context="") -> str:
        print("Context: " + context)
        prompt = callback(0, None)
        cur_len = len(context)

        messages = [
            {"role": "system", "content": context},
        ]

        with self._get_session() as requests_session:
            for i in range(1, MAX_CONVERSATION_STEPS):
                print("Prompt: " + prompt)
                cur_len += len(prompt)

                messages.append({"role": "user", "content": prompt})
                payload = self._get_payload(messages, max_answer_tokens, cur_len > 8000)

                request_id = post_generate_chat_request(requests_session, payload)
                response = poll_get_generate_chat_request(requests_session, request_id)
                answer = response["completion"]
                print("Response: " + str(answer))

                prompt = callback(i, answer)

                if prompt is None:
                    return answer

                cur_len += len(answer)
                received_message = response["completion"]
                messages.append({"role": "assistant", "content": received_message})

        raise BrokenPipeError()

    def _get_payload(self, history: list, max_tokens: int, big_context: bool) -> list:
        # payload = {
        #     "model_name": "gpt-4-turbo-8k",
        #     "model_parameters": {"temperature": 0.8, "max_completion_token_count": 400},
        #     "history": [
        #         {
        #             "role": "system",
        #             "content": "You are a friendly AI assistant, helping humans with their questions.",
        #         },
        #         {"role": "assistant", "content": "Hello, how can I help you?"},
        #         {"role": "user", "content": "Tell me something about BMW in 2 sentences."},
        #     ],
        # }
        #if big_context:

        payload = {
            "model_name": "gpt-4-turbo-8k",
            "model_parameters": {"temperature": 0.0, "max_completion_token_count": max_tokens},
        }

        # else:
        #     payload = {
        #         "model_name": "gpt-35-turbo-4k",
        #         "model_parameters": {"temperature": 0.0, "max_completion_token_count": max_tokens},
        #     }

        payload["history"] = history
        return payload

    def _get_session(self):
        requests_session = requests.Session()

        # requests_session.verify = False  # ensure to install CA on
        requests_session.proxies = self.proxies
        requests_session.hooks = self.hooks

        wen_token = get_webeam_access_token(requests_session)

        requests_session.headers.update(
            {
                "Accept": "application/json",
                "x-apikey": CONFIG[ENVIRONMENT]["API_KEY"],
                "Authorization": f"Bearer {wen_token}",
            }
        )

        return requests_session

