from uuid import uuid4
from socket import socket

from .message import Message

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

        payload_content = self._socket.recv(payload_size).decode(encoding)

        return Message(payload_magic, payload_flags, payload_content)

    def write(self, message: Message):
        if (not self.isopen()):
            return

        self._socket.send(message.to_bytes())

    def close(self):
        if (not self.isopen()):
            return
        
        self._socket.close()
        self._socket = None

    def __del__(self):
        self.close()