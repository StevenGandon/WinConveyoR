from uuid import uuid4
from socket import socket, AF_INET, SOCK_STREAM
from select import select
from signal import signal, SIGINT, SIGTERM

from .message import Message
from .handler import Handler

class ClientSocket(object):
    def __init__(self, host = "127.0.0.1", port = 1674, *, socket_builder = lambda: socket(AF_INET, SOCK_STREAM)):
        self.running = False
        self.sessions = {}
        self.handler = Handler()

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

class Client(object):
    MAX_PAYLOAD_SIZE = 1024 * 1024 * 1024 * 2

    def __init__(self, _socket, address = ("??.??.??.??", -1), /, public_key = None):
        self._id = uuid4().int
        self._socket: socket = _socket

        self.address = address
        self._public_key = public_key

    def set_public_key(self, public_key):
        self._public_key = public_key

    def get_id(self):
        return (self._id)
    
    def get_fd(self):
        return (self._socket.fileno())

    def isopen(self):
        return (self._socket and self._socket.fileno() >= 0)
    
    def read(self, encoding = "utf8") -> Message:
        if (not self.isopen()):
            raise ConnectionError("client disconnected.")

        payload_magic = self._socket.recv(4)
        
        if (len(payload_magic) == 0):
            raise ConnectionError("client disconnected.")
        
        payload_magic = int.from_bytes(payload_magic, "big")
        payload_flags = int.from_bytes(self._socket.recv(2), "big") # for later
        payload_size = int.from_bytes(self._socket.recv(8), "big")

        if (payload_size > Client.MAX_PAYLOAD_SIZE):
            raise BufferError("payload size received exceed max size.")

        payload_content = b""
        while len(payload_content) < payload_size:
            chunk = self._socket.recv(min(payload_size - len(payload_content), 65536))
            if (not chunk):
                raise ConnectionError("client disconnected.")
            payload_content += chunk

        return Message.from_recv(payload_magic, payload_flags, payload_content, encoding=encoding)

    def read_raw_to_file(self, size, filepath):
        remaining = size
        with open(filepath, "wb") as fp:
            while remaining > 0:
                chunk = self._socket.recv(min(remaining, 65536))
                if (not chunk):
                    raise ConnectionError("client disconnected.")
                fp.write(chunk)
                remaining -= len(chunk)

    def write(self, message: Message):
        if (not self.isopen()):
            return
        
        has_key = hasattr(self, "_public_key") and self._public_key

        if (has_key):
            old = getattr(message, "PUBLIC_KEY", None)
            setattr(message, "PUBLIC_KEY", self._public_key)

        self._socket.send(message.to_bytes())

        if (has_key):
            setattr(message, "PUBLIC_KEY", old)

    def close(self):
        if (not self.isopen()):
            return
        
        self._socket.close()
        self._socket = None

    def __del__(self):
        self.close()