from select import select
from socket import socket, AF_INET, SOCK_STREAM
from signal import signal, SIGINT, SIGTERM

from .message import Message
from .client import Client
from .handler import Handler

class Server(object):
    def __init__(self, host = "0.0.0.0", port = 1674, *, socket_builder = lambda: socket(AF_INET, SOCK_STREAM)):
        self.running = False
        self.clients = {}
        self.sessions = {}
        self.handler = Handler()

        try:
            self._socket: socket = socket_builder()
        except Exception:
            raise OSError(f"failed to create socket. ({e})")


        if (not self._socket or self._socket.fileno() < 0):
            raise OSError(f"failed to create socket (invalid object/fileno after creation).")

        try:
            self._socket.bind((host, port))
        except Exception as e:
            raise ConnectionError(f"failed to bind socket on {host}:{port}. ({e})")

    def set_handler(self, handler):
        self.handler = handler

    def isopen(self):
        return (hasattr(self, "_socket") and self._socket and self._socket.fileno() >= 0)

    def _serve(self, client: Client):
        try:
            message = client.read()

        except BufferError:
            self.handler.error(client, self, "message size exceed.")
            return

        except ValueError:
            self.handler.error(client, self, "message does not stard with a valid magic.")
            return

        except (ConnectionResetError, ConnectionError):
            client.close()
            return

        self.handler.message(client, self, message)

    def update(self):
        if (not self.isopen()):
            return

        for item in tuple(self.clients.keys()):
            if (self.clients[item].isopen()):
                continue
            self.clients[item].close()
            print(f"destroying: client#{item}")
            del self.clients[item]

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
            rlist, _, xlist = select([self._socket.fileno(), *[item.get_fd() for item in self.clients.values()]], [], [], 0)
        except Exception as e:
            print(f"failed to select sockets. ({e})")
            return

        for fd in rlist:
            if (fd == self._socket.fileno()):
                socket, address = self._socket.accept()
                client = Client(socket, address)

                self.clients[client.get_id()] = client 
                print(f"creating: client#{client.get_id()}")
            else:
                for client in self.clients.values():
                    if (fd != client.get_fd()):
                        continue
                    self._serve(client)

    def run(self, backlog = 5):
        if (not self.isopen()):
            return

        try:
            self._socket.listen(backlog)
        except Exception as e:
            raise ConnectionError(f"socket failed to listen. ({e})")

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
        for item in self.clients.values():
            item.close()

        for item in self.sessions.values():
            item.close()

        self.clients.clear()
        self.sessions.clear()

        if (self.isopen()):
            self._socket.close()
        self._socket = None

        self.running = False

    def __del__(self):
        self.close()