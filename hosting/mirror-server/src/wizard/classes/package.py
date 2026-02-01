from .section import WizardSection
from .strndx import WizardStrndx

class PackageWizard(object):
    FLAGS_DEFAULT = 0x00
    FLAGS_ALIGNMENT_8 = (1 << 0)
    FLAGS_ALIGNMENT_16 = (1 << 1)

    PADDING_BYTES = b"\xCD"

    def __init__(self):
        self._magic = b"\x42\xa4\x09\x67"
        self._sections = []
        self._strndx = []

        self._byte_order = "big"
        self._version = 0x0100
        self._flags = PackageWizard.FLAGS_DEFAULT | PackageWizard.FLAGS_ALIGNMENT_8

        self._version_size = 2
        self._endianness_size = 1
        self._flags_size = 4
        self._str_len_size = 4
        self._addresses_size = 8
        self._strndx_ref_size = 4
        self._section_header_size_size = 8
        self._section_number_size = 8
        self._section_size_size = 8
        self._section_type_size = 4
        self._section_flags_size = 4

    def _get_alignment(self):
        if ((self._flags & self.FLAGS_ALIGNMENT_8) > 0 and (self._flags & self.FLAGS_ALIGNMENT_16) > 0):
            raise ValueError("Only one alignment flag may be set.")

        if ((self._flags & self.FLAGS_ALIGNMENT_16) > 0):
            return (16)
        if ((self._flags & self.FLAGS_ALIGNMENT_8) > 0):
            return (8)
        return (1)

    @staticmethod
    def _align(value, alignment=8):
        return (value + alignment - 1) & ~(alignment - 1)

    @staticmethod
    def _align_fp(alignment, fp):
        if (alignment <= 1):
            return
        
        cur_addr = fp.tell()
        aligned_addr = PackageWizard._align(cur_addr, alignment)

        if (aligned_addr <= cur_addr):
            return
        fp.write(PackageWizard.PADDING_BYTES * (aligned_addr - cur_addr))

    def _compute_sections_size(self, offset):
        size: int = offset
        alignment: int = self._get_alignment()

        for item in self._sections:
            if (alignment > 1):
                size = self._align(size, alignment)
            size += item.get_size()
        return (size - offset)
    
    def get_endianness(self):
        return (self._byte_order)

    def get_str_offset_size(self):
        return (self._str_len_size)

    def add_section(self, section: WizardSection):
        self._sections.append(section)

    def add_strndx(self, string: str):
        already_registered = self.get_strndx(string)

        if (already_registered is not None):
            return (already_registered.addr)
        if (self._strndx):
            last_str = self._strndx[-1]
            addr = last_str.addr + len(last_str.content) + self._str_len_size
        else:
            addr = 0
        self._strndx.append(WizardStrndx(string, addr))

        return (addr)

    def get_strndx(self, string: str, /, encoding = "utf8"):
        encoded_str = string.encode(encoding)

        for item in self._strndx:
            if (item.content == encoded_str):
                return (item)
        return (None)

    def get_strndex_at(self, addr: int):
        for item in self._strndx:
            if (item.addr == addr):
                return (item)
            if (item.addr > addr):
                break
        return (None)

    def write(self, file_path) -> None:
        file_header = bytearray()
        section_header = bytearray()

        int_to_bytes = lambda number, size: int.to_bytes(number, size, byteorder=self._byte_order)

        alignment = self._get_alignment()

        _file_header_size = (
            len(self._magic) + 
            self._endianness_size +
            self._version_size +
            self._flags_size +
            self._addresses_size +
            self._addresses_size
        )

        _section_header_off = (
            _file_header_size
        )

        _section_header_entry = (
            self._strndx_ref_size +
            self._section_type_size +
            self._section_flags_size +
            self._section_size_size +
            self._addresses_size
        )

        _section_header_size = (
            self._section_header_size_size +
            self._section_number_size +
            (_section_header_entry * len(self._sections))
        )

        section_header.extend(int_to_bytes(number=_section_header_size, size=self._section_header_size_size))
        section_header.extend(int_to_bytes(number=len(self._sections), size=self._section_number_size))

        addr = _section_header_off + _section_header_size

        for item in self._sections:
            if (alignment > 1):
                addr = self._align(addr, alignment)
            section_header.extend(int_to_bytes(number=self.add_strndx(item.name), size=self._strndx_ref_size))
            section_header.extend(int_to_bytes(number=item.type, size=self._section_type_size))
            section_header.extend(int_to_bytes(number=item.flags, size=self._section_flags_size))
            section_header.extend(int_to_bytes(number=item.get_size(), size=self._section_size_size))
            section_header.extend(int_to_bytes(number=addr, size=self._addresses_size))

            addr += item.get_size()

        _sections_off = _file_header_size + len(section_header)

        _strndx_off = (
            _sections_off +
            self._compute_sections_size(_sections_off)
        )

        file_header.extend(self._magic)
        file_header.extend(int_to_bytes(number=(0 if self._byte_order == "big" else 1), size=self._endianness_size))
        file_header.extend(int_to_bytes(number=self._version, size=self._version_size))
        file_header.extend(int_to_bytes(number=self._flags, size=self._flags_size))
        file_header.extend(int_to_bytes(number=_section_header_off, size=self._addresses_size))
        file_header.extend(int_to_bytes(number=_strndx_off, size=self._addresses_size))

        with open(file_path, 'wb+') as fp:
            fp.write(file_header)
            fp.write(section_header)
            for item in self._sections:
                self._align_fp(alignment, fp)
                fp.write(item.to_bytes(parent=self))
            self._align_fp(alignment, fp)
            for item in self._strndx:
                fp.write(int_to_bytes(number=len(item.content), size=self._str_len_size))
                fp.write(item.content)