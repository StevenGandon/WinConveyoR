class WizardInstruction(object):
    _OP_SIZE = 0x2

    def __init__(self, opcode: int, args: list):
        self.code = opcode
        self.args = args

    def get_size(self):
        return (self._OP_SIZE + sum(map(lambda x: x.get_size(), self.args)))
    
    def to_bytes(self, parent):
        content = bytearray()

        content.extend(int.to_bytes(self.code, self._OP_SIZE, byteorder=parent.get_endianess()))
        for item in self.args:
            content.extend(item.to_bytes(parent))

        return (bytes(content))