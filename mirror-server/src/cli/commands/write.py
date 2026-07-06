from ..network_command_wrapper import network_command, default_json_payload_filler
from ..middlewares.check_response import check_response
from ..middlewares.check_status import check_status
from ...network.message_json import JSONMessage

@network_command(
    JSONMessage({
        "action": "write",
        "data": {"session_id": "$session_id"}
    }), default_json_payload_filler,
    [check_response, check_status]
)
def write_command(cli, /, session_id, response):
    print(f'{response.content["data"]["edited_instance_count"]} PACKAGE INSTANCE IN {response.content["data"]["edited_package_count"]} PACKAGE WRITTEN')
