from ...network.message_json import JSONMessage

def check_response(cli, args, response):
    try:
        response = JSONMessage.from_message(response)
    except Exception as e:
        print(f"invalid data from server, failed to parse as json. ({e})")
        return (False, response)
    if ("code" not in response.content or "data" not in response.content or "action" not in response.content):
        print("invalid json content from server, json should always contain 'data', 'code' and 'action'.")
        return (False, response)
    return (True, response)
