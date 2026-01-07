from sys import exit
from src import *

from json import dumps, loads

class JSONMessage(Message):
    def __init__(self, content):
        super().__init__(Message.MAGIC, 0, content)

    def to_bytes(self, encoding="utf8"):
        old_content = self.content
        self.content = dumps(self.content)

        value = super().to_bytes(encoding)

        self.content = old_content
        return (value)

    @staticmethod
    def from_message(message: Message):
        try:
            content = loads(message.content)
        except Exception:
            raise ValueError("can't parse client message as json.")

        return (JSONMessage(content))

class Router(object):
    def __init__(self):
        self._routes = {
            "default": lambda *args, **kwargs: print(*args, **kwargs)
        }

    def route(self, name, *args, **kwargs):
        if (name not in self._routes):
            return self._routes["default"](*args, **kwargs)
        return self._routes[name](*args, **kwargs)

    def add_route(self, name, callback):
        self._routes[name] = callback

class WCRHandler(Handler):
    def __init__(self, router: Router, *, set_default_route = True):
        super().__init__()

        self._router = router

        if (set_default_route):
            self._router.add_route("default", self.default_router_not_found)

    def default_router_not_found(self, client: Client, server: Server, message: Message):
        self.error(client, server, f"action not found: '{message.content["action"]}'.")

    def error(self, client, server, e, *, action = "uknown"):
        client.write(JSONMessage({
            "action": action,
            "data": {
                "msg": str(e)
            },
            "code": 1
        }))

    def message(self, client, server, message):
        try:
            message = JSONMessage.from_message(message)
        except Exception as e:
            return self.error(client, server, "failed to parse json message.")

        if ("action" not in message.content or "data" not in message.content):
            return self.error(client, server, "missing fields in json.")
        
        self._router.route(message.content["action"], client, server, message)

def route_hello(client: Client, server: Server, message: JSONMessage):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "hello"
        },
        "code": 0
    }))

def route_goodbye(client: Client, server: Server, message: JSONMessage):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "goodbye"
        },
        "code": 0
    }))

    server.clients[client.get_id()].close()

def route_connect(client: Client, server: Server, message: JSONMessage):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

    S = Session(client)

    server.sessions[S.get_id()] = S

def route_disconnect(client: Client, server: Server, message: JSONMessage):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

    if ("session_id" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

def main():
    try:
        S = Server()
    except ConnectionError as e:
        print(f"failed to create server. ({e})")
        return (1)
    R = Router()

    R.add_route("hello", route_hello)
    R.add_route("connect", route_connect)
    R.add_route("disconnect", route_disconnect)
    R.add_route("goodbye", route_goodbye)

    S.set_handler(WCRHandler(R))
    S.run()

    S.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())
