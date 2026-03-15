from ..network_command_wrapper import network_command, default_json_payload_filler
from ..middlewares.check_response import check_response
from ..middlewares.check_status import check_status
from ...network.message_json import JSONMessage
from ...network.session import Session

@network_command(
    JSONMessage({
        "action": "user",
        "data": {"password": "$password"}
    }), default_json_payload_filler,
    [check_response, check_status]
)
def user_command(cli, /, password, response):
    if ("session_id" not in response.content["data"]):
        print("missing session_id in response.")
        return

    session = response.content["data"]["session_id"]
    cli.client.sessions[session] = Session(cli.client, session_id=session)
    cli.client.sessions[session].set_flags((1 << 1))
    cli.active_session = session
    cli.prompt = cli.prompt_session.replace("$session_id", hex(cli.active_session).split('0x')[-1])
