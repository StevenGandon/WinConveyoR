from ..common.user_input import init_terminal, uninit_terminal, non_blocking_read
from ..network.message_json import JSONMessage
from ..network.message import Message
from ..network.client import ClientSocket
from .commands.connect import connect_command
from .commands.quit import quit_command
from .commands.list_sessions import list_sessions_command
from .commands.switch_session import switch_session_command
from .commands.disconnect import disconnect_command
from .commands.list_packages import list_packages_command
from .command import CLICommand, CLICommandArg

from string import hexdigits

from signal import signal, SIGINT, SIGTERM

class HandlerClient(object):
    def __init__(self, cli):
        self.cli = cli

    def error(self, client, server, e):
        self.cli.display(f"error with server: {e}")

    def message(self, client, server, message: Message):
        self.cli.display(f"message from server: {message.content}")

class NetworkCLICommand(object):
    def __init__(self, command: CLICommand, /, need_login: bool = False):
        self.command: CLICommand = command
        self.need_login: bool = need_login

class NetworkCLI(object):
    def __init__(self):
        self._old_attrs = init_terminal()
        
        self.client: ClientSocket = ClientSocket()
        self.client.set_handler(HandlerClient(self))

        self.active_session = None

        self.running = False
        self.buffer = bytearray()

        self.history = [""]

        self.prompt_base = ">>> "
        self.prompt_session = "[$session_id]>>> "
        self.prompt = self.prompt_base
        self.cursor = 0
        self.history_cursor = 0

        self.commands = {}

    def add_command(self, command: CLICommand, need_login: bool = False):
        self.commands[command.name] = NetworkCLICommand(command, need_login=need_login)

    def handle_input(self, inputs):
        if (not inputs):
            return

        command, *args = inputs.split(" ")

        if (command not in self.commands):
            print(f"command not found '{command}'.")
            return
        
        if (self.commands[command].need_login):
            if (not self.active_session):
                print(f"command need to be executed in a session.")
                return
            args = [str(self.active_session)] + args

        return (self.commands[command].command.run_command(self, args))

    def user_input(self):
        val = non_blocking_read()

        if (val is None):
            return
        
        if (val == b"\n" or val == b"\r"):
            print("\r\n", end="", flush=True)
            self.cursor = 0
            value = self.buffer.decode(errors="replace")
            self.buffer.clear()
            self.history_cursor = 0
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

        try:
            message = JSONMessage.from_message(self.client.read())
        except ConnectionError as e:
            print(f"lost connection to server during handcheck. ({e})")
            return
        except Exception as e:
            print(f"failed to get and parse server handcheck response. ({e})")
            return

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