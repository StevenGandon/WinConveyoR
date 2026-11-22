from .message import Message

from json import loads, dumps

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
