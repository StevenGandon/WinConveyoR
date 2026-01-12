#!/usr/bin/python3

from sys import exit, argv
from json import dump, load
from os.path import join, basename
from os import mkdir, walk
from glob import glob
from tarfile import open as open_tar
from src.common import hash_file
from hashlib import md5, sha256

from genericpath import isdir

class PackageInfo(object):
    def __init__(self, name, version="1.0.0", description = "", depends = [], machine = "any", architecture = "x64"):
        self.name = name
        self.version = version
        self.description = description
        self.depends = depends
        self.machine = machine
        self.architecture = architecture

    def generate_json(self, file_path: str):
        with open(file_path, 'w+') as fp:
            dump({
                "package": self.name,
                "version": self.version,
                "description": self.description,
                "architecture": self.architecture,
                "depends": self.depends,
                "machine": self.machine
            }, fp, indent=4)

class WizardArgument(object):
    ARG_UKN = 0x00
    ARG_STR = 0x01
    ARG_U8 = 0x02
    ARG_U16 = 0x03
    ARG_U32 = 0x04
    ARG_U64 = 0x05

    def __init__(self, arg_type, value = None):
        self.type = arg_type
        self.value = value

class WizardInstruction(object):
    OP_NOOP = 0x00
    OP_MKDIR = 0x01
    OP_COPY = 0x02
    OP_RUN = 0x03
    OP_CHMOD = 0x04
    OP_REMOVE = 0x05
    OP_REMOVE_TREE = 0x06
    OP_COPY_TREE = 0x07
    OP_RMDIR = 0x08
    OP_CHMOD = 0x09
    OP_MOVE = 0x0A
    OP_MOVE_TREE = 0x0B

    def __init__(self, opcode: int, args: list):
        self.code = opcode
        self.args = args

class WizardSection(object):
    TYPE_GENERIC_SECTION = 0x00
    TYPE_SECTION_INSTALL = 0x01
    TYPE_SECTION_UNINSTALL = 0x02
    TYPE_SECTION_PURGE = 0x03
    TYPE_SECTION_BUILD = 0x04
    TYPE_SECTION_METADATA = 0x05

    FLAGS_DEFAULT = 0x00

    def __init__(self, name = "new_section", section_type = 0x00, section_flags = 0x00):
        self.name = name
        self.type = section_type
        self.section_flags = section_flags
    
    def get_size(self):
        return (0)

    def to_bytes(self, parent = None) -> bytes:
        return b""

class WizardStrndx(object):
    def __init__(self, content: str, addr: int, /, encoding = "utf8"):
        self.content = content.encode(encoding)

        self.addr = addr

class PackageWizard(object):
    FLAGS_DEFAULT = 0x00
    FLAGS_ALIGNEMENT_8 = (1 << 0)
    FLAGS_ALIGNEMENT_16 = (1 << 1)

    def __init__(self):
        self._magic = b"\x42\xa4\x09\x67"
        self._sections = []
        self._strndx = []

        self._byte_order = "big"
        self._version = 0x0100
        self._flags = PackageWizard.FLAGS_DEFAULT

        self._version_size = 2
        self._endianess_size = 1
        self._flags_size = 4
        self._str_len_size = 4
        self._addresses_size = 8
        self._strndx_ref_size = 4
        self._section_header_size_size = 8
        self._section_number_size = 8
        self._section_size_size = 8
        self._section_type_size = 4
        self._section_flags_size = 4

    def _align(value, alignment=8):
        return (value + alignment - 1) & ~(alignment - 1)

    def _compute_sections_size(self):
        size: int = 0

        for item in self._sections:
            size += item.get_size()
        return (size)

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

        _file_header_size = (
            len(self._magic) + 
            self._endianess_size +
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
            section_header.extend(int_to_bytes(number=self.add_strndx(item.name), size=self._strndx_ref_size))
            section_header.extend(int_to_bytes(number=item.type, size=self._section_type_size))
            section_header.extend(int_to_bytes(number=item.flags, size=self._section_flags_size))
            section_header.extend(int_to_bytes(number=item.get_size(), size=self._section_size_size))
            section_header.extend(int_to_bytes(number=addr, size=self._addresses_size))

            addr += item.get_size()


        _strndx_off = (
            _file_header_size +
            len(section_header) +
            self._compute_sections_size()
        )

        file_header.extend(self._magic)
        file_header.extend(int_to_bytes(number=(0 if self._byte_order == "big" else 1), size=self._endianess_size))
        file_header.extend(int_to_bytes(number=self._version, size=self._version_size))
        file_header.extend(int_to_bytes(number=self._flags, size=self._flags_size))
        file_header.extend(int_to_bytes(number=_section_header_off, size=self._addresses_size))
        file_header.extend(int_to_bytes(number=_strndx_off, size=self._addresses_size))

        with open(file_path, 'wb+') as fp:
            fp.write(file_header)
            fp.write(section_header)
            for item in self._sections:
                fp.write(item.to_bytes(parent=self))
            for item in self._strndx:
                fp.write(int_to_bytes(number=len(item.content), size=self._str_len_size))
                fp.write(item.content)

class PackageBuilder(object):
    def __init__(self, asset_path, pkg_info, temp_dir = "./temp"):
        self.asset_path = asset_path
        self.pkg_info: PackageInfo = pkg_info
        self.temp_dir = temp_dir
        self.hashs = {}
        self.hash = ""

        self.wizard = PackageWizard()

        if (not isdir(self.temp_dir)):
            mkdir(self.temp_dir)

        self._generate_hashs()
        self._generate_global_hash()

        if (not isdir(join(self.temp_dir, self.hash))):
            mkdir(join(self.temp_dir, self.hash))

    def _generate_global_hash(self):
        computed = sha256()

        for item in sorted(self.hashs.keys()):
            computed.update(self.hashs[item]["sha256"].encode())
        self.hash = computed.hexdigest()

    def _generate_hashs(self):
        for parent, dirs, files in walk(self.asset_path):
            for item in files:
                full_path = join(parent, item)
                self.hashs[full_path.replace('\\', '/').replace(self.asset_path.replace('\\', '/'), '').lstrip('/')] = {"sha256": hash_file(full_path), "md5": hash_file(full_path, md5)}

    def pack_assets(self, output_path: str):
        tar = open_tar(output_path, "w:gz")

        for file_name in glob(join(self.asset_path, "*"), include_hidden=True):
            tar.add(file_name, basename(file_name))

        tar.close()

    def pack_package(self, output_path: str):
        tar = open_tar(output_path, "w:gz")

        for file in glob(join(self.temp_dir, self.hash, "*"), include_hidden=True):
            tar.add(file, basename(file))

        tar.close()

    def generate_package(self):
        self.pkg_info.generate_json(join(
            self.temp_dir,
            f"infos_{self.pkg_info.name}_{self.pkg_info.architecture}_{self.pkg_info.machine}_{self.pkg_info.version.replace('.', '-')}"
        ))

        self.pack_assets(join(self.temp_dir, self.hash , "package.tar.gz"))

        self.wizard.write(join(self.temp_dir, self.hash, ".WIZARD"))

        with open(join(self.temp_dir, self.hash, ".PACK"), "wb+") as fp:
            content = bytearray()
            byte_order = "big"

            content.extend(b"\xff\x45\x87\x90")
            content.extend(len(self.hashs).to_bytes(8, byte_order))

            for item in self.hashs:
                encoded = item.encode()

                content.extend(len(encoded).to_bytes(8, byte_order))
                content.extend(encoded)
                content.extend(int(self.hashs[item]["sha256"], 16).to_bytes(32, byte_order))
                content.extend(int(self.hashs[item]["md5"], 16).to_bytes(16, byte_order))

            fp.write(bytes(content))

        self.pack_package(join(
            self.temp_dir,
            f"{self.pkg_info.name}_{self.pkg_info.architecture}_{self.pkg_info.machine}_{self.pkg_info.version.replace('.', '-')}.tar.gz"
        ))

class Dialog(object):
    @staticmethod
    def ask_until_given(text):
        content = ""

        while (not content):
            content = input(text).strip()

        return (content)

    @staticmethod
    def ask_until_empty(text):
        content = "empty"
        items = []

        while (content):
            content = input(text).strip()
            if (content):
                items.append(content)

        return (items)

    @staticmethod
    def ask_enum(text, possibilities):
        content = ""

        while (content not in possibilities):
            content = input(text).strip()

            if (content not in possibilities):
                print(f"invalid value, should be within these ones: ({', '.join(possibilities)})")

        return (content)

    @staticmethod
    def ask_with_default(text, default):
        content = input(text).strip()

        if (not content):
            return (default)
        return (content)

def interactive_mode():
    print("-===========[ Informations ]===========-")
    
    package_name = Dialog.ask_until_given("package name (nodefault): ")
    package_description = Dialog.ask_with_default(f"package description (default: '{package_name} package.'): ", package_name)
    package_version = Dialog.ask_with_default(f"package version (default: 1.0.0): ", "1.0.0")
    package_deps = Dialog.ask_until_empty(f"package dependencies (leave empty to stop defining dependencies): ")
    package_machine = Dialog.ask_with_default(f"package machine (default: windows): ", "windows")
    package_architecture = Dialog.ask_with_default(f"package architecture (default: x64): ", "x64")

    pkg_info = PackageInfo(package_name, package_version, package_description, package_deps, package_machine, package_architecture)
    pkg_builder = PackageBuilder(argv[1], pkg_info)

    print("-===========[ Installation ]===========-")

    package_type = Dialog.ask_enum("package installation type (nodefault) (static, compile, custom): ", ["static", "compile", "custom"])


    pkg_builder.wizard.add_section(WizardSection())

    print("-===========[ Content ]===========-")

    pkg_builder.generate_package()

    return (0)

def config_file_mode():
    with open(argv[2], "r") as fp:
        config = load(fp)

    pkg_builder = PackageBuilder(argv[1], PackageInfo(
        config["name"],
        config["version"],
        config["description"],
        config["deps"],
        config["machine"],
        config["architecture"]
    ))

    pkg_builder.generate_package()

    return (0)

def main() -> int:
    if (len(argv) < 2):
        print(f"{argv[0]}: not enough argument.")
        return (1)

    if (not isdir(argv[1])):
        print(f"{argv[0]}: {argv[1]}: not a directory.")
        return (1)
    
    if (len(argv) < 3):
        print("no config file provided, starting interactive mode.")
        return (interactive_mode())

    return (config_file_mode())

if (__name__ == "__main__"):
    exit(main())
