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
