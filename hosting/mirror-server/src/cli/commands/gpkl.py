from ..network_command_wrapper import network_command, default_json_payload_filler
from ..middlewares.check_response import check_response
from ..middlewares.check_status import check_status
from ...network.message_json import JSONMessage
from ...network.session import Session

@network_command(
    JSONMessage({
        "action": "get_package_listing",
        "data": {"session_id": "$session_id", "package_name": "$package_name"}
    }), default_json_payload_filler
)
def gpkl_command(cli, /, session_id, package_name, response):
    print(response.content)
