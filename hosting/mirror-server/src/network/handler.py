from .message import Message

class Handler(object):
    def __init__(self):
        pass

    def error(self, client, server, e):
        print(f"error with client {client.get_id()}: {e}")
        client.write(Message(Message.MAGIC, 0, e))

    def message(self, client, server, message):
        print(f"message from client {client.get_id()}: {message.content}")
        client.write(Message(Message.MAGIC, 0, message.content))
