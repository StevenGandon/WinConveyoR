#!/usr/bin/python3

from sys import exit
from sys import argv
from os import environ
from src import *
from dotenv import load_dotenv, find_dotenv

load_dotenv(find_dotenv())

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
        self.error(client, server, f"action not found: '{message.content['action']}'.")

    def error(self, client, server, e, *, action = "unknown"):
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

def protected_route(route):
    def wrapper(client: Client, server: Server, message: JSONMessage):
        if ("session_id" not in message.content["data"]):
            client.write(JSONMessage({
                "action": message.content["action"],
                "data": {
                    "msg": "ko"
                },
                "code": 1
            }))

            return
        
        session_id = message.content["data"]["session_id"]

        if (session_id not in server.sessions):
            client.write(JSONMessage({
                "action": message.content["action"],
                "data": {
                    "msg": "invalid_session_id"
                },
                "code": 1
            }))

            return

        if (server.sessions[session_id].client != client):
            client.write(JSONMessage({
                "action": message.content["action"],
                "data": {
                    "msg": "unauthorized_session"
                },
                "code": 1
            }))

            return

        return route(client, server, message, session=server.sessions[session_id])
    return (wrapper)

def route_init_rsa(client: Client, server: Server, message: JSONMessage):
    if ("key" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    

    client.set_public_key(PublicSecurityKey.from_string(message.content["data"]["key"]))

    client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ok"
            },
            "code": 0
        }))

def route_hello(client: Client, server: Server, message: JSONMessage):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "******************************\n*                            *\n*      wcr source server     *\n*                            *\n******************************"
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

@protected_route
def route_list_packages(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "packages": list(session.session_instance.packages.keys())
        },
        "code": 0
    }))

def route_connect(client: Client, server: Server, message: JSONMessage):
    if ("password" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    if ("WCR_PASSWORD" in environ and message.content["data"]["password"] != environ["WCR_PASSWORD"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "invalid_password"
            },
            "code": 1
        }))

        return

    S = Session(client, session_instance=MirrorServer("." if "WCR_DIR" not in environ else environ["WCR_DIR"], load=True))

    server.sessions[S.get_id()] = S

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok",
            "session_id": S.get_id()
        },
        "code": 0
    }))

@protected_route
def route_write(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    session.session_instance.write()

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok",
            "edited_package_count": len(tuple(filter(lambda x: x.loaded, session.session_instance.packages.values()))),
            "edited_instance_count": len(tuple(filter(lambda x: x.loaded, sum([list(item.listing.values()) for item in session.session_instance.packages.values() if item.loaded], []))))
        },
        "code": 0
    }))

@protected_route
def route_disconnect(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    session.close()
    del server.sessions[session.get_id()]

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

def main():
    if ("WCR_PASSWORD" not in environ):
        print("warning: no password set anyone can edit.")
    # if ("WCR_ACCESS" not in environ):
    #     print("warning: no access key set anyone can download packages.")
    if ("WCR_RSA" not in environ):
        print("warning: no rsa encryption, requests are plain text.")
    if ("WCR_RSA_PASS" not in environ):
        print("warning: no rsa key password, using 'None' as password.")
    if ("WCR_DIR" not in environ):
        print("warning: no directory path given for source server data using '.'.")

    try:
        if ("WCR_RSA" in environ):
            Message.PRIVATE_KEY = PrivateSecurityKey.from_file(environ["WCR_RSA"], environ.get("WCR_RSA_PASS"))
    except Exception as e:
        print(f"failed to load rsa key. ({e})")
        return (1)

    try:
        S = Server()
    except ConnectionError as e:
        print(f"failed to create server. ({e})")
        return (1)
    R = Router()

    R.add_route("hello", route_hello)
    R.add_route("connect", route_connect)
    R.add_route("init_rsa", route_init_rsa)
    R.add_route("disconnect", route_disconnect)
    R.add_route("list_packages", route_list_packages)
    R.add_route("write", route_write)
    R.add_route("goodbye", route_goodbye)

    S.set_handler(WCRHandler(R))
    S.run()

    S.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())
