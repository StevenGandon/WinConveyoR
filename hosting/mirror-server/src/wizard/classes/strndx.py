class WizardStrndx(object):
    def __init__(self, content: str, addr: int, /, encoding = "utf8"):
        self.content = content.encode(encoding)

        self.addr = addr