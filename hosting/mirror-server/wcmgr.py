from sys import exit
from socket import socket, AF_INET, SOCK_STREAM
from select import select
from signal import signal, SIGINT, SIGTERM

from json import dumps, loads

from src import *

import sys

if (sys.platform.startswith("win")):
    from msvcrt import getch, kbhit

    def init_terminal(fileno = None):
        return (None)

    def uninit_terminal(old_settings, fileno = None):
        pass

    def non_blocking_read():
        if (kbhit()):
            return getch()
        return (None)
else:
    from os import read
    from tty import setcbreak, setraw
    from termios import tcsetattr, tcgetattr, TCSADRAIN, TCSAFLUSH, BRKINT, ICRNL, INPCK, ISTRIP, IXON, OPOST, CSIZE, PARENB, CS8, ECHO, ICANON, IEXTEN, VMIN, VTIME

    IFLAG = 0
    OFLAG = 1
    CFLAG = 2
    LFLAG = 3
    ISPEED = 4
    OSPEED = 5
    CC = 6

    def _setraw(fd, when=TCSAFLUSH):
        mode = tcgetattr(fd)
        mode[IFLAG] = mode[IFLAG] & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON)
        mode[OFLAG] = mode[OFLAG] & ~(OPOST)
        mode[CFLAG] = mode[CFLAG] & ~(CSIZE | PARENB)
        mode[CFLAG] = mode[CFLAG] | CS8
        mode[LFLAG] = mode[LFLAG] & ~(ECHO | ICANON | IEXTEN)
        mode[CC][VMIN] = 1
        mode[CC][VTIME] = 0
        tcsetattr(fd, when, mode)

    def init_terminal(fileno = None):
        if (fileno is None):
            fileno = sys.stdin.fileno()

        attrs = tcgetattr(fileno)

        _setraw(fileno)
        setcbreak(fileno, TCSAFLUSH)

        return (attrs)

    def uninit_terminal(old_settings, fileno = None):
        if (fileno is None):
            fileno = sys.stdin.fileno()
        tcsetattr(fileno, TCSADRAIN, old_settings)

    def non_blocking_read():
        rlist, _, _ = select([sys.stdin.fileno()], [], [], 0)

        for item in rlist:
            if (item == sys.stdin.fileno()):
                return read(sys.stdin.fileno(), 1)
        
        return (None)

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
    def __init__(self, cli):
        self.cli = cli

    def error(self, client: Client, server, e):
        self.cli.display(f"error with server: {e}")

    def message(self, client: Client, server, message: Message):
        self.cli.display(f"message from server: {message.content}")

class ClientHandler(object):
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

class NetworkCLI(object):
    def __init__(self):
        self.client: ClientHandler = ClientHandler()
        self.client.set_handler(HandlerClient(self))

        self.running = False
        self.buffer = bytearray()

        self.prompt = ">>> "
        self.cursor = 0

        self._old_attrs = init_terminal()

    def handle_input(self, inputs):
        print(f"command not found '{inputs}'.")

    def user_input(self):
        val = non_blocking_read()

        if (val is None):
            return
        
        if (val == b"\n" or val == b"\r"):
            print("\r\n", end="", flush=True)
            self.cursor = 0
            value = self.buffer.decode(errors="replace")
            self.buffer.clear()
            self.handle_input(value)
            return
        
        if (val == b"\x1b"):
            seq = non_blocking_read()

            if (seq != b"["):
                return
            
            action = non_blocking_read()

            if (action == b"D" and self.cursor > 0):
                self.cursor -= 1

            if (action == b"C" and self.cursor < len(self.buffer)):
                self.cursor += 1
            
        if (val == b"\xe0"):
            direction = non_blocking_read()

            if (direction == b"K" and self.cursor > 0):
                self.cursor -= 1

            if (direction == b"M" and self.cursor < len(self.buffer)):
                self.cursor += 1
        
        try:
            decoded = val.decode()
        except Exception as e:
            return
        
        if (decoded.isprintable()):
            if (self.cursor == len(self.buffer)):
                self.buffer.extend(val)
            else:
                self.buffer.insert(self.cursor, val[0])
            self.cursor += 1

    def display(self, message):
        print("\r" + ' ' * (len(self.buffer.decode()) + len(self.prompt)) + '\r' + message)

    def draw(self):
        print("\r" + " " * (len(self.buffer.decode()) + len(self.prompt) + 1) + "\r" + self.prompt + self.buffer.decode() + "\b" * (len(self.buffer) - self.cursor), end="", flush=True)

    def update(self):
        self.client.update()

    def events(self):
        self.user_input()
        self.client.events()

        if (not self.client.isopen()):
            self.display("Lost connection to the server.")

            self.running = False

        return self.running

    def run(self):
        signal(SIGINT, lambda *args: self.close())
        signal(SIGTERM, lambda *args: self.close())

        self.running = True

        self.client.write(JSONMessage({"action": "hello", "data": {}}))

        while (self.running):
            if (not self.events()):
                break
            self.update()
            self.draw()

    def close(self):
        if (hasattr(self, "client") and self.client):
            self.client.close()
        uninit_terminal(self._old_attrs)
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