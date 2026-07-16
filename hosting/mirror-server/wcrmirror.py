#!/usr/bin/python3

from sys import exit
from sys import argv
from os import environ, mkdir
from os.path import join, isdir
from src import *
from src.arghandler import *
from dotenv import load_dotenv, find_dotenv
from json import dumps
from hashlib import sha256
from base64 import b64decode

import sys

load_dotenv(find_dotenv(usecwd=True))

FLAG_ADMIN = (1 << 0)
FLAG_USER = (1 << 1)

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
        
        try:
            self._router.route(message.content["action"], client, server, message)
        except Exception as e:
            print(f"error: {e}")
            return self.error(client, server, f"server error: {e}.")

def protected_route(flags = FLAG_ADMIN):
    def protected_route_wrapper(route):
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
            
            if (not server.sessions[session_id].has_flags(flags)):
                client.write(JSONMessage({
                    "action": message.content["action"],
                    "data": {
                        "msg": "permission_not_match"
                    },
                    "code": 1
                }))

                return

            return route(client, server, message, session=server.sessions[session_id])
        return (wrapper)
    return (protected_route_wrapper)

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

@protected_route(FLAG_ADMIN)
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
    S.set_flags(FLAG_ADMIN | FLAG_USER)

    server.sessions[S.get_id()] = S

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok",
            "session_id": S.get_id()
        },
        "code": 0
    }))

def route_user(client: Client, server: Server, message: JSONMessage):
    if ("password" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    if ("WCR_ACCESS" in environ and message.content["data"]["password"] != environ["WCR_ACCESS"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "invalid_password"
            },
            "code": 1
        }))

        return

    S = Session(client, session_instance=MirrorServer("." if "WCR_DIR" not in environ else environ["WCR_DIR"], load=True))
    S.set_flags(FLAG_USER)

    server.sessions[S.get_id()] = S

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok",
            "session_id": S.get_id()
        },
        "code": 0
    }))

@protected_route(FLAG_USER)
def route_get_hash(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    session.session_instance.load()

    client.write(Message(Message.MAGIC, 0x00, session.session_instance.checksum))

@protected_route(FLAG_USER)
def route_get_listing(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    session.session_instance.load()

    client.write(Message(Message.MAGIC, 0x00, '\r\n'.join(f"{item.name} {item.latest} {item.hash}" for item in session.session_instance.packages.values())))

@protected_route(FLAG_USER)
def route_get_package_listing(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    
    session.session_instance.load()

    if (message.content["data"]["package_name"] not in session.session_instance.packages):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "not_found"
            },
            "code": 1
        }))

        return

    package_name = message.content["data"]["package_name"]
    package = session.session_instance.packages[package_name]

    package.load()

    client.write(Message(Message.MAGIC, 0x00, '\r\n'.join(f"{item.version} {item.architecture} {item.machine} {sha256(str(item.location).encode(errors='replace')).hexdigest()}" for item in package.listing.values())))

@protected_route(FLAG_USER)
def route_get_package_metadata(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    
    if ("location_hash" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    
    session.session_instance.load()

    if (message.content["data"]["package_name"] not in session.session_instance.packages):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "not_found"
            },
            "code": 1
        }))

        return

    package_name = message.content["data"]["package_name"]
    location_hash = int(message.content["data"]["location_hash"], base=16)
    package = session.session_instance.packages[package_name]
    package_listing = None

    package.load()

    for item in package.listing.values():
        if (int.from_bytes(sha256(str(item.location).encode(errors='replace')).digest(), "big") != location_hash):
            continue
        package_listing = item
        break

    if (not package_listing):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "not_found"
            },
            "code": 1
        }))

        return
    
    package_listing.load()

    client.write(Message(Message.MAGIC, 0x00, dumps(package_listing.package_data)))

@protected_route(FLAG_USER)
def route_get_file(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    from os.path import isfile, join

    if ("package_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {"msg": "ko"},
            "code": 1
        }))
        return

    if ("location_hash" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {"msg": "ko"},
            "code": 1
        }))
        return

    session.session_instance.load()

    package_name = message.content["data"]["package_name"]
    location_hash = int(message.content["data"]["location_hash"], base=16)

    if (package_name not in session.session_instance.packages):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {"msg": "not_found"},
            "code": 1
        }))
        return

    package = session.session_instance.packages[package_name]
    package.load()

    package_listing = None
    for item in package.listing.values():
        if (int.from_bytes(sha256(str(item.location).encode(errors="replace")).digest(), "big") != location_hash):
            continue
        package_listing = item
        break

    if (not package_listing):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {"msg": "not_found"},
            "code": 1
        }))
        return

    package_listing.load()

    address = package_listing.package_data.get("address")
    if (not address):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {"msg": "not_found"},
            "code": 1
        }))
        return

    wcr_dir = "." if "WCR_DIR" not in environ else environ["WCR_DIR"]
    full_path = join(wcr_dir, address.lstrip('/'))

    if (not isfile(full_path)):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {"msg": "not_found"},
            "code": 1
        }))
        return

    with open(full_path, "rb") as f:
        content = f.read()

    client.write(Message(Message.MAGIC, 0x00, content))

@protected_route(FLAG_ADMIN)
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

@protected_route(FLAG_ADMIN)
def route_new_pkg_listing(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    session.session_instance.add_package_register(message.content["data"]["package_name"])

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

@protected_route(FLAG_ADMIN)
def route_undo(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if (session.session_instance.undo_stack.can_undo()):
        lastest = session.session_instance.undo_stack.get_lastest_undo()
        session.session_instance.undo()

        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": f"undone {lastest.name}."
            },
            "code": 0
        }))
    else:
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "nothing to undo."
            },
            "code": 0
        }))

        return

@protected_route(FLAG_ADMIN)
def route_redo(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if (session.session_instance.undo_stack.can_redo()):
        lastest = session.session_instance.undo_stack.get_lastest_redo()
        session.session_instance.redo()

        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": f"redone {lastest.name}."
            },
            "code": 0
        }))
    else:
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "nothing to redo."
            },
            "code": 0
        }))

        return

@protected_route(FLAG_ADMIN)
def route_list_backups(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": list(session.session_instance.list_backups())
        },
        "code": 0
    }))

@protected_route(FLAG_ADMIN)
def route_retrieve_backup(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("backup_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    try:
        session.session_instance.restore(message.content["data"]["backup_name"])
    except FileNotFoundError:
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "not_found"
            },
            "code": 1
        }))

        return

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))


@protected_route(FLAG_ADMIN)
def route_purge_pkg_listing(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    
    if (not session.session_instance.has_package_register(message.content["data"]["package_name"])):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "package_not_found"
            },
            "code": 1
        }))
    
    session.session_instance.remove_package_register(message.content["data"]["package_name"], hard_delete=False)

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

@protected_route(FLAG_ADMIN)
def route_remove_pkg_listing(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_name" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    
    if (not session.session_instance.has_package_register(message.content["data"]["package_name"])):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "package_not_found"
            },
            "code": 1
        }))

    package_register = session.session_instance.get_package_register(message.content["data"]["package_name"])

    if (not package_register.has_package_listing(message.content["data"]["package_hash"])):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "package_listing_not_found"
            },
            "code": 1
        }))

    package_register.remove_package_listing(message.content["data"]["package_hash"], hard_delete=False)

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

@protected_route(FLAG_ADMIN)
def route_add_pkg(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_data" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return
    
    if ("package_archive" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    new_package = session.session_instance.add_package_register(message.content["data"]["package_data"]["package"])
    name = f"{message.content['data']['package_data']['package']}-{message.content['data']['package_data']['version']}-{message.content['data']['package_data']['architecture']}-{message.content['data']['package_data']['machine']}.tar.gz"

    if (not isdir(join(session.session_instance.location, "temp"))):
        mkdir(join(session.session_instance.location, "temp"))
    with open(join(session.session_instance.location, "temp", name), 'wb+') as fp:
        fp.write(b64decode(message.content["data"]["package_archive"]))

    listing = new_package.add_package_listing(message.content["data"]["package_data"], join(session.session_instance.location, "temp", name))

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

@protected_route(FLAG_ADMIN)
def route_add_pkg_binary(client: Client, server: Server, message: JSONMessage, /, session: Session = None):
    if ("package_data" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    if ("archive_size" not in message.content["data"]):
        client.write(JSONMessage({
            "action": message.content["action"],
            "data": {
                "msg": "ko"
            },
            "code": 1
        }))

        return

    archive_size = message.content["data"]["archive_size"]
    new_package = session.session_instance.add_package_register(message.content["data"]["package_data"]["package"])
    name = f"{message.content['data']['package_data']['package']}-{message.content['data']['package_data']['version']}-{message.content['data']['package_data']['architecture']}-{message.content['data']['package_data']['machine']}.tar.gz"

    if (not isdir(join(session.session_instance.location, "temp"))):
        mkdir(join(session.session_instance.location, "temp"))

    client._busy = True

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ready"
        },
        "code": 0
    }))

    filepath = join(session.session_instance.location, "temp", name)
    client.read_raw_to_file(archive_size, filepath)

    client._busy = False

    listing = new_package.add_package_listing(message.content["data"]["package_data"], filepath)

    client.write(JSONMessage({
        "action": message.content["action"],
        "data": {
            "msg": "ok"
        },
        "code": 0
    }))

@protected_route(FLAG_ADMIN)
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
    argsettings = ArgumentParserSettings(1, 1)

    argsettings.define_argument(str, "program")

    argsettings.define_parameter("--port", int)
    argsettings.define_parameter("--host", str)

    argsettings.define_option("-h", is_help=True)
    argsettings.define_option("--help", is_help=True)
    argsettings.define_option("-?", is_help=True)

    argsettings.validate()

    try:
        argparser = ArgumentParser(argv, argsettings)
    except ArgumentHandlerException as e:
        sys.stderr.write(f"{argv[0]}: {e}\n")
        return (1)

    if ("-h" in argparser.options or "--help" in argparser.options or "-?" in argparser.options):
        help_message = f"Usage: $prgm_name [options] $args\nOptions:\n$options"
        args = ''.join(str(item.name) if i < argsettings.min_argv - 1 else f"({item.name})" for i, item in enumerate(argsettings.arguments[1:]))
        options = '  ' + '\n  '.join(item.name for item in (list(argsettings.parameters.values()) + list(argsettings.options.values())))

        print(help_message.replace("$prgm_name", str(argparser.arguments[0].value)).replace("$args", args).replace("$options", options))
        return (0)

    if ("WCR_PASSWORD" not in environ):
        print("warning: no password set anyone can edit.")
    if ("WCR_ACCESS" not in environ):
        print("warning: no access key set anyone can download packages.")
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
        S = Server("0.0.0.0" if "--host" not in argparser.parameters else argparser.parameters["--host"].value, 1674 if "--port" not in argparser.parameters else argparser.parameters["--port"].value)
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
    R.add_route("user", route_user)
    R.add_route("get_hash", route_get_hash)
    R.add_route("get_listing", route_get_listing)
    R.add_route("get_package_listing", route_get_package_listing)
    R.add_route("get_package_metadata", route_get_package_metadata)
    R.add_route("get_file", route_get_file)
    R.add_route("add_package", route_add_pkg)
    R.add_route("add_package_binary", route_add_pkg_binary)
    R.add_route("new_package", route_new_pkg_listing)
    R.add_route("purge_package", route_purge_pkg_listing)
    R.add_route("remove_package", route_remove_pkg_listing)
    R.add_route("restore_backup", route_retrieve_backup)
    R.add_route("list_backups", route_list_backups)
    R.add_route("undo", route_undo)
    R.add_route("redo", route_redo)
    R.add_route("goodbye", route_goodbye)

    S.set_handler(WCRHandler(R))
    S.run()

    S.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())
