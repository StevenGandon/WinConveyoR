from sys import exit
from socket import socket, AF_INET, SOCK_STREAM
from select import select
from signal import signal, SIGINT, SIGTERM

from json import dumps, loads

from src import *

import sys

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

class HandlerClient(object):
    def __init__(self):
        pass

    def error(self, client: Client, server, e):
        print(f"error with client {client.get_id()}: {e}")

    def message(self, client: Client, server, message: Message):
        print(f"message from client {client.get_id()}: {message.content}")

class ClientHandler(object):
    def __init__(self, host = "127.0.0.1", port = 1674, *, socket_builder = lambda: socket(AF_INET, SOCK_STREAM)):
        self.running = False
        self.sessions = {}
        self.handler = HandlerClient()

        try:
            self._socket: socket = socket_builder()
        except Exception:
            raise OSError(f"failed to create socket. ({e})")


        if (not self._socket or self._socket.fileno() < 0):
            raise OSError(f"failed to create socket (invalid object/fileno after creation).")

        try:
            self._socket.connect((host, port))
        except Exception as e:
            raise ConnectionError(f"failed to connect socket to {host}:{port}. ({e})")

        self.read = lambda *args, **kwargs: Client.read(self, *args, **kwargs)
        self.write = lambda *args, **kwargs: Client.write(self, *args, **kwargs)

    def get_id(self):
        return (0)

    def set_handler(self, handler):
        self.handler = handler

    def isopen(self):
        return (hasattr(self, "_socket") and self._socket and self._socket.fileno() >= 0)

    def _get_response(self):
        try:
            message = self.read()

        except BufferError:
            self.handler.error(self, self, "message size exceed.")
            return

        except ValueError:
            self.handler.error(self, self, "message does not stard with a valid magic.")
            return

        except (ConnectionResetError, ConnectionError):
            self.close()
            return

        self.handler.message(self, self, message)

    def update(self):
        if (not self.isopen()):
            return
        
        for item in tuple(self.sessions.keys()):
            if (self.sessions[item].isopen()):
                continue
            self.sessions[item].close()
            print(f"destroying: sessions#{item}")
            del self.sessions[item]

    def events(self):
        if (not self.isopen()):
            return

        try:
            rlist, _, _ = select([self._socket.fileno()], [], [], 0)
        except Exception as e:
            print(f"failed to select sockets. ({e})")
            return

        for fd in rlist:
            if (fd == self._socket.fileno()):
                self._get_response()

    def run(self):
        if (not self.isopen()):
            return

        signal(SIGINT, lambda *args: self.close())
        signal(SIGTERM, lambda *args: self.close())

        self.running = True

        while (self.running):
            if (not self.isopen()):
                self.running = False
                break
            
            self.events()
            self.update()

    def close(self):
        for item in self.sessions.values():
            item.close()
        self.sessions.clear()

        if (self.isopen()):
            self._socket.close()
        self._socket = None

        self.running = False

    def __del__(self):
        self.close()

class NetworkCLI(object):
    def __init__(self):
        self.client: ClientHandler = ClientHandler()

        self.running = False

    def user_input(self):
        pass

    def update(self):
        self.client.update()

    def events(self):
        self.user_input()
        self.client.events()

        if (not self.client.isopen()):
            print("Lost connection to the server.")

            self.running = False

    def run(self):
        signal(SIGINT, lambda *args: self.close())
        signal(SIGTERM, lambda *args: self.close())

        self.running = True

        self.client.write(JSONMessage({"action": "hello", "data": {}}))
        print(">>> ", end="", flush=True)

        while (self.running):
            self.events()
            self.update()

    def close(self):
        if (hasattr(self, "client") and self.client):
            self.client.close()
        self.running = False

    def __del__(self):
        self.close()

def main():
    try:
        nc = NetworkCLI()
    except ConnectionError as e:
        print(f"failed to connect to server. ({e})")
        return (1)
    nc.run()
    nc.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())