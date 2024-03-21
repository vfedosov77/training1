import requests
import json
import time
from http import HTTPStatus
from requests.exceptions import HTTPError

import logging

logging.getLogger().setLevel(logging.INFO)
logging.captureWarnings(True)

POLLING_INTERVAL = 1

CONFIG = {
    "PROD": {
        "HOSTNAME": "api.gcp.cloud.bmw",
        "CLIENT_ID": "c5da1774-3f9d-4b0f-bbc6-9f2b9c904657",
        "CLIENT_SECRET": "Tts5VQeqBZYOKC5184i8aWCNPNNVSTfATwdzfgED",
        "API_KEY": "FW2f1PERkGGo6trjYesWthXon6SFXK9bI9b3wru1cIewws8V" #iLdAMZLRZ0mGvgJUsHm3QJ8uIFfzaX8TeznYnQwztb1xFwGY0TGw8d49cd1Upc5A"
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
    while count < 50:
        response = get_generate_chat_request(requests_session, request_id)
        logging.info(f"Received polling response:\n{json.dumps(response, indent=2)}")

        if response["status"] != "PENDING":
            duration = time.time() - start_time
            logging.info(f"Finished polling after {duration:.2f} seconds.")
            return response

        time.sleep(POLLING_INTERVAL)

    raise Exception("Polling took too long, aborting")


def main():
    # Define the JSON payload
    payload = {
        "model_name": "gpt-4-turbo-8k",
        "model_parameters": {"temperature": 0.8, "max_completion_token_count": 400},
        "history": [
            {
                "role": "system",
                "content": "You are a friendly AI assistant, helping humans with their questions.",
            },
            {"role": "assistant", "content": "Hello, how can I help you?"},
            {"role": "user", "content": "Tell me something about BMW in 2 sentences."},
        ],
    }

    try:
        # wen_token = get_webeam_access_token(requests_session)

        with requests.Session() as requests_session:
            # requests_session.verify = False  # ensure to install CA on
            requests_session.proxies = {"http": "", "https": ""}
            requests_session.hooks = {"response": lambda r, *args, **kwargs: r.raise_for_status()}

            wen_token = get_webeam_access_token(requests_session)

            requests_session.headers.update(
                {
                    "Accept": "application/json",
                    "x-apikey": CONFIG[ENVIRONMENT]["API_KEY"],
                    "Authorization": f"Bearer {wen_token}",
                }
            )

            request_id = post_generate_chat_request(requests_session, payload)
            answer = poll_get_generate_chat_request(requests_session, request_id)
            logging.info(f"Received answer:\n{json.dumps(answer, indent=2)}")
    except HTTPError as e:
        logging.exception(f"error with request:\n{e.response.json()}")


if __name__ == "__main__":
    main()