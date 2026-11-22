from ...network.message_json import JSONMessage
from json import load
from base64 import b64encode

def add_pkg_command(cli, /, session_id, package_data, package_archive):
    with open(package_data, 'r') as fp:
        data = load(fp)
    with open(package_archive, 'rb') as fp:
        cli.client.write(JSONMessage({"action": "add_package", "data": {"session_id": session_id, "package_data": data, "package_archive": b64encode(fp.read()).decode("utf-8")}}))
    try:
        response = cli.client.read()
    except ConnectionError:
        print("server connection close without giving a response.")
        cli.close()
        return
    print(JSONMessage.from_message(response).content["data"]["msg"])
