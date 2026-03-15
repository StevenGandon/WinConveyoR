from .security import PrivateSecurityKey

class Message(object):
    MAGIC = 0xffc407ec
    PRIVATE_KEY = None
    PUBLIC_KEY = None

    def __init__(self, magic: int, flags: int, content: str):
        if (Message.MAGIC != magic):
            raise ValueError("magic does not match message magic.")

        self.magic: int = magic
        self.flags: int = flags
        self.content: str = content

    @staticmethod
    def from_recv(magic: int, flags: int, content: bytes, /, encoding = "utf8"):
        if (Message.PRIVATE_KEY is None):
            return (Message(magic, flags, content.decode(encoding)))
        return (Message(magic, flags, Message.PRIVATE_KEY.decrypt(content).decode(encoding)))

    def to_bytes(self, encoding = "utf8"):
        buffer = bytearray()

        if (self.PUBLIC_KEY is None):
            encoded_content = self.content.encode(encoding)
        else:
            encoded_content = self.PUBLIC_KEY.encrypt(self.content.encode())

        buffer.extend(self.magic.to_bytes(4, "big"))
        buffer.extend(self.flags.to_bytes(2, "big"))
        buffer.extend(len(encoded_content).to_bytes(8, "big"))
        buffer.extend(encoded_content)


        return bytes(buffer)
        
