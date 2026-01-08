from sys import exit
from socket import socket, AF_INET, SOCK_STREAM
from select import select
from signal import signal, SIGINT, SIGTERM

from src import *

class HandlerClient(object):
    def __init__(self, cli):
        self.cli = cli

    def error(self, client, server, e):
        self.cli.display(f"error with server: {e}")

    def message(self, client, server, message: Message):
        self.cli.display(f"message from server: {message.content}")

class NetworkCLI(object):
    def __init__(self):
        self.client: ClientSocket = ClientSocket()
        self.client.set_handler(HandlerClient(self))

        self.running = False
        self.buffer = bytearray()

        self.history = [""]

        self.prompt = ">>> "
        self.cursor = 0
        self.history_cursor = 0

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
            self.history[-1] = value
            self.history.append("")
            self.handle_input(value.strip())
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

            if (action == b"A" and self.history_cursor < len(self.history) - 1):
                if (self.history_cursor == 0):
                    self.history[-1] = self.buffer.decode()
                self.history_cursor += 1
                print('\r' + " " * (len(self.prompt) + len(self.buffer.decode())) + "\r" + self.prompt, end="")
                self.buffer = bytearray(self.history[len(self.history) - 1 - self.history_cursor].encode())
                self.cursor = len(self.buffer)

            if (action == b"B" and self.history_cursor > 0):
                if (self.history_cursor == 0):
                    self.history[-1] = self.buffer.decode()
                self.history_cursor -= 1
                print('\r' + " " * (len(self.prompt) + len(self.buffer.decode())) + "\r" + self.prompt, end="")
                self.buffer = bytearray(self.history[len(self.history) - 1 - self.history_cursor].encode())
                self.cursor = len(self.buffer)

            if (action == b"3" and non_blocking_read() == b"~" and self.cursor < len(self.buffer)):
                self.buffer.pop(self.cursor)
            
        if (val == b"\xe0"):
            direction = non_blocking_read()

            if (direction == b"K" and self.cursor > 0):
                self.cursor -= 1

            if (direction == b"M" and self.cursor < len(self.buffer)):
                self.cursor += 1

            if (direction == b"S" and self.cursor < len(self.buffer)):
                self.buffer.pop(self.cursor)

            if (direction == b"H" and self.history_cursor < len(self.history) - 1):
                if (self.history_cursor == 0):
                    self.history[-1] = self.buffer.decode()
                self.history_cursor += 1
                print('\r' + " " * (len(self.prompt) + len(self.buffer.decode())) + "\r" + self.prompt, end="")
                self.buffer = bytearray(self.history[len(self.history) - 1 - self.history_cursor].encode())
                self.cursor = len(self.buffer)

            if (direction == b"P" and self.history_cursor > 0):
                if (self.history_cursor == 0):
                    self.history[-1] = self.buffer.decode()
                self.history_cursor -= 1
                print('\r' + " " * (len(self.prompt) + len(self.buffer.decode())) + "\r" + self.prompt, end="")
                self.buffer = bytearray(self.history[len(self.history) - 1 - self.history_cursor].encode())
                self.cursor = len(self.buffer)
        
        if ((val == b"\x08" or val == b"\x7f") and self.cursor > 0):
            self.buffer.pop(self.cursor - 1)
            self.cursor -= 1

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
        message = JSONMessage.from_message(self.client.read())

        if ("code" not in message.content or "action" not in message.content or "data" not in message.content or message.content["code"] != 0 or message.content["action"] != "hello"):
            raise ConnectionError("handcheck with server failed")

        if ("msg" in message.content["data"]):
            print(message.content["data"]["msg"])

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