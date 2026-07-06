class WizardSection(object):
    TYPE_GENERIC_SECTION = 0x00
    TYPE_SECTION_INSTALL = 0x01
    TYPE_SECTION_UNINSTALL = 0x02
    TYPE_SECTION_PURGE = 0x03
    TYPE_SECTION_BUILD = 0x04
    TYPE_SECTION_METADATA = 0x05

    FLAGS_DEFAULT = 0x00
    FLAGS_WINDOWS = (1 << 0)
    FLAGS_POSIX = (1 << 1)

    def __init__(self, name = "new_section", section_type = 0x00, section_flags = 0x00):
        self.name = name
        self.flags = section_flags
        self.type = section_type
    
    def get_size(self):
        return (1)

    def to_bytes(self, parent) -> bytes:
        return b"\00"

class WizardCodeSection(WizardSection):
    def __init__(self, name="new_section", section_type=0, section_flags=0):
        super().__init__(name, section_type, section_flags)

        self.instructions = []

    def get_size(self):
        return sum(map(lambda x: x.get_size(), self.instructions))
    
    def to_bytes(self, parent):
        content = bytearray()

        for item in self.instructions:
            content.extend(item.to_bytes(parent))
        return bytes(content)