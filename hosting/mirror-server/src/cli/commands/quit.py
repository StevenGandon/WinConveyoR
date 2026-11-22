from ...network.message_json import JSONMessage

def quit_command(cli):
    cli.client.write(JSONMessage({"action": "goodbye", "data": {}}))
    cli.running = False