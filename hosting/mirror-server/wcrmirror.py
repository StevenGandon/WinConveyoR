from sys import exit
from os import read, write
from socket import socket, AF_INET, SOCK_STREAM
from select import select
from uuid import uuid4
from signal import signal, SIGINT, SIGTERM
from json import dumps

class Message(object):
    MAGIC = 0xffc407ec

    def __init__(self, magic: int, flags: int, content: str):
        if (Message.MAGIC != magic):
            raise ValueError("magic does not match message magic.")

        self.magic: int = magic
        self.flags: int = flags
        self.content: str = content

    def to_bytes(self, encoding = "utf8"):
        buffer = bytearray()
        encoded_content = self.content.encode(encoding)

        buffer.extend(self.magic.to_bytes(4, "big"))
        buffer.extend(self.flags.to_bytes(2, "big"))
        buffer.extend(len(encoded_content).to_bytes(8, "big"))
        buffer.extend(encoded_content)

        return bytes(buffer)

class Client(object):
    MAX_PAYLOAD_SIZE = 1024 * 1024 * 1024 * 2

    def __init__(self, _socket, address = ("??.??.??.??", -1)):
        self._id = uuid4().int
        self._socket: socket = _socket

        self.address = address

    def get_id(self):
        return (self._id)
    
    def get_fd(self):
        return (self._socket.fileno())

    def isopen(self):
        return (self._socket and self._socket.fileno() >= 0)
    
    def read(self, encoding = "utf8") -> Message:
        payload_magic = self._socket.recv(4)
        
        if (len(payload_magic) == 0):
            raise ConnectionError("client disconnected.")
        
        payload_magic = int.from_bytes(payload_magic, "big")
        payload_flags = int.from_bytes(self._socket.recv(2), "big") # for later
        payload_size = int.from_bytes(self._socket.recv(8), "big")

        if (payload_size > Client.MAX_PAYLOAD_SIZE):
            raise BufferError("payload size received exceed max size.")

        payload_content = self._socket.recv(payload_size).decode(encoding)

        return Message(payload_magic, payload_flags, payload_content)

    def write(self, message: Message):
        pass

    def close(self):
        if (not self.isopen()):
            return
        
        self._socket.close()
        self._socket = None

    def __del__(self):
        self.close()

class Session(object):
    def __init__(self, client = Client(None)):
        self._id = uuid4().int

        self.opened = True
        self.client = client

    def get_id(self):
        return (self._id)

    def isopen(self):
        return (self.opened and self.client.isopen())

    def close(self):
        self.opened = False

    def __del__(self):
        self.close()

class Server(object):
    def __init__(self, host = "0.0.0.0", port = 1674, *, socket_builder = lambda: socket(AF_INET, SOCK_STREAM)):
        self.running = False
        self.clients = {}
        self.sessions = {}

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

    def isopen(self):
        return (hasattr(self, "_socket") and self._socket and self._socket.fileno() >= 0)

    def _serve(self, client: Client):
        try:
            msg = client.read()

        except BufferError:
            print(f"exceeding size msg from: {client.get_id()}.")
            client.write(Message(Message.MAGIC, 0, dumps({"msg": "message size exceed.", "code": 1})))
            return

        except ValueError:
            print(f"invalid msg from: {client.get_id()}.")
            client.write(Message(Message.MAGIC, 0, dumps({"msg": "message does not stard with a valid magic.", "code": 1})))
            return

        except (ConnectionResetError, ConnectionError):
            client.close()
            return

        print(f"msg from {client.get_id()}: {msg.content}")

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

def main():
    S = Server()
    S.run()
    S.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())
