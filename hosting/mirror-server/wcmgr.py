from sys import exit
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

    def handle_input(self, inputs):
        command, *args = inputs.split(" ")

        if (command == "connect"):
            self.client.write(JSONMessage({"action": "connect", "data": {"password": args[0] if args else ""}}))
            message = JSONMessage.from_message(self.client.read())

            if (not "code" in message.content or message.content["code"] != 0):
                print(f"connection failed: {message.content['data'].get('msg')}")
                return

            if ("session_id" not in message.content["data"]):
                print("missing session_id in response.")
                return

            session = message.content["data"]["session_id"]
            self.client.sessions[session] = Session(self.client, session_id=session)
            self.active_session = session
            self.prompt = self.prompt_session.replace("$session_id", hex(self.active_session).split('0x')[-1])
            return
        
        if (command == "disconnect"):
            if (len(args) < 1):
                if (self.active_session is None):
                    print("missing session_id.")
                    return
                else:
                    args.append(hex(self.active_session).split("0x")[-1])
            session_as_int = int(args[0], 16)
            if (session_as_int not in self.client.sessions):
                print("invalid session.")
                return

            self.client.write(JSONMessage({"action": "disconnect", "data": {"session_id": session_as_int}}))
            message = JSONMessage.from_message(self.client.read())

            if (not "code" in message.content or message.content["code"] != 0):
                print(f"disconnection failed: {message.content['data'].get('msg')}")
                return

            del self.client.sessions[session_as_int]

            if (self.active_session == session_as_int):
                self.active_session = None
                self.prompt = self.prompt_base
            return
        
        if (command == "list_sessions"):
            print('\n'.join(map(lambda x: hex(x).split('0x')[-1], self.client.sessions.keys())))
            return
        
        if (command == "switch_session"):
            if (len(args) < 1):
                self.active_session = None
                self.prompt = self.prompt_base
                return
            session_as_int = int(args[0], 16)
            if (session_as_int not in self.client.sessions):
                print("invalid session.")
                return
            self.active_session = session_as_int
            self.prompt = self.prompt_session.replace("$session_id", hex(self.active_session).split('0x')[-1])
            return
        
        if (command == "list_packages"):
            if (self.active_session is None):
                print("not in a session.")
                return
            self.client.write(JSONMessage({"action": "list_packages", "data": {"session_id": self.active_session}}))
            message = JSONMessage.from_message(self.client.read())

            if (not "code" in message.content or message.content["code"] != 0):
                print(f"list_packages failed: {message.content['data'].get('msg')}")
                return
            
            print(message.content["data"]["packages"])
            return

        if (command == "quit"):
            self.client.write(JSONMessage({"action": "goodbye", "data": {}}))
            self.running = False
            return

        print(f"command not found '{command}'.")

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