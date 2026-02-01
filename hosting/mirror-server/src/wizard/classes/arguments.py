class WizardArgument(object):
    _STR_ARGS = [
    ]

    _ARGS = {
    }

    def __init__(self, arg_type, value = None):
        self.type = arg_type
        self.value = value

    def get_size(self):
        assert self.type in self._ARGS, f"Type '{self.type}' is not registered in {self.__class__.__qualname__}."

        return (self._ARGS[self.type])
    
    def to_bytes(self, parent):
        assert self.type in self._ARGS, f"Type '{self.type}' is not registered in {self.__class__.__qualname__}."
        assert self.value is not None, f"Value for '{self.type}' can not be None to convert to byte."

        content = bytearray()

        if (self.type in self._STR_ARGS):
            content.extend(int.to_bytes(parent.add_strndx(self.value), parent.get_str_offset_size(), byteorder=parent.get_endianness()))
        else:
            content.extend(int.to_bytes(self.value, self._ARGS[self.type], byteorder=parent.get_endianness()))

        return (bytes(content))

class WizardArgumentArray(WizardArgument):
    _ARR_COUNT_SIZE = 0x4
    
    def __init__(self, arg_type, values=None):
        super().__init__(arg_type, None)
        self.values = values if values is not None else list()

    def get_size(self):
        return (super().get_size() * len(self.values))
    
    def to_bytes(self, parent):
        assert self.value is None

        content = bytearray()

        content.extend(int.to_bytes(len(self.values), self._ARR_COUNT_SIZE, byteorder=parent.get_endianness()))

        for item in self.values:
            self.value = item
            content.extend(super().to_bytes(parent))

        self.value = None

        return (bytes(content))
