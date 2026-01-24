#!/usr/bin/python3

from sys import exit, argv
from json import dump, load
from os.path import join, basename
from os import mkdir, walk
from glob import glob
from tarfile import open as open_tar
from src.common import hash_file
from hashlib import md5, sha256
from yaml import safe_load

from genericpath import isdir

from src import InstructionLoader

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

    _STR_ARGS = [
        ARG_STR
    ]

    _ARGS = {
        ARG_STR: 0x4,
        ARG_U8: 0x1,
        ARG_U16: 0x2,
        ARG_U32: 0x4,
        ARG_U64: 0x8
    }

    def __init__(self, arg_type, value = None):
        self.type = arg_type
        self.value = value

    def get_size(self):
        assert self.type in self._ARGS

        return (self._ARGS[self.type])
    
    def to_bytes(self, parent):
        assert self.type in self._ARGS
        assert self.value is not None

        content = bytearray()

        if (self.type in self._STR_ARGS):
            content.extend(int.to_bytes(parent.add_strndx(self.value), parent.get_str_offset_size(), byteorder=parent.get_endianess()))
        else:
            content.extend(int.to_bytes(self.value, self._ARGS[self.type], byteorder=parent.get_endianess()))

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

        content.extend(int.to_bytes(len(self.values), self._ARR_COUNT_SIZE, byteorder=parent.get_endianess()))

        for item in self.values:
            self.value = item
            content.extend(super().to_bytes(parent))

        self.value = None

        return (bytes(content))

class WizardInstruction(object):
    _OP_CODES = []
    OP_NOOP = 0x00
    OP_MKDIR = 0x01
    OP_COPY = 0x02
    OP_RUN = 0x03
    OP_CHMOD = 0x04
    OP_REMOVE = 0x05
    OP_REMOVE_TREE = 0x06
    OP_COPY_TREE = 0x07
    OP_RMDIR = 0x08
    OP_MOVE = 0x09
    OP_MOVE_TREE = 0x0A

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
        self.section_flags = section_flags
    
    def get_size(self):
        return (0)

    def to_bytes(self, parent) -> bytes:
        return b""

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
    
    def get_endianess(self):
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

class ConfigSchema(object):
    def __init__(self, content):
        self.content = content

class ConfigSchemaBank(object):
    def __init__(self, **kwargs):
        self.schemas = {**kwargs}

class ConfigMessage(object):
    def __init__(self, message: str):
        self.message: str = message

    def __str__(self):
        return (f"{self.message}")

    def __repr__(self):
        return (self.__str__())

class ConfigWarning(ConfigMessage):
    pass

class ConfigError(ConfigMessage):
    pass

class ConfigKey(object):
    def __init__(self, content_type = str, /, required = True, default = None):
        self.required = required
        self.default = default

        self.type = content_type

class YAMLConfigReader(object):
    _ROOT = "wizard"
    _VERSION = "$root.version"
    _LATEST = 1
    _MIN_VER = 1
    _OP_KEYS = {}
    _SCHEMA_BANK = {
        1: ConfigSchemaBank(
            metadata=ConfigSchema({
                "$root.metadata.name": ConfigKey(str, required=False, default="new_package"),
                "$root.metadata.description": ConfigKey(str, required=False, default="new package."),
                "$root.metadata.version": ConfigKey(str, required=False, default="1.0.0"),
                "$root.metadata.deps": ConfigKey(list, required=False, default=[]),
                "$root.metadata.machine": ConfigKey(str, required=False, default="any"),
                "$root.metadata.architecture": ConfigKey(str, required=False, default="any")
            }),
            step=ConfigSchema({
                "$step.when": ConfigKey(dict, required=False, default={}),
                "$step.when.os": ConfigKey(str, required=False, default="any")
            }),
            steps=ConfigSchema({
                "mkdir":  ConfigSchema({
                    "path": ConfigKey(str, required=True)
                }),
                "run": ConfigSchema({
                    "tool": ConfigKey(str, required=True),
                    "args": ConfigKey(list, required=False, default=[])
                }),
                "chmod": ConfigSchema({
                    "path": ConfigKey(str, required=True),
                    "mode": ConfigKey(int, required=True)
                }),
                "remove": ConfigSchema({
                    "path": ConfigKey(str, required=True)
                }),
                "remove_tree": ConfigSchema({
                    "path": ConfigKey(str, required=True)
                }),
                "copy": ConfigSchema({
                    "from": ConfigKey(str, required=True),
                    "to": ConfigKey(str, required=True)
                })
            }),
            jobs=ConfigSchema({
                "$root.build": ConfigKey(dict, required=False, default={}),
                "$root.install": ConfigKey(dict, required=False, default={}),
                "$root.uninstall": ConfigKey(dict, required=False, default={}),
                "$root.purge": ConfigKey(dict, required=False, default={})
            }),
            job=ConfigSchema({
                "$job.requires": ConfigKey(dict, required=False, default={}),
                "$job.requires.tools": ConfigKey(list, required=False, default=[]),
                "$job.steps": ConfigKey(list, required=False, default=[])
            })
        )
    }

    def __init__(self, file: str):
        self.file: str = file
        self.version: int = 0

        with open(self.file, 'r') as fp:
            self._raw_config: object = safe_load(fp)

        self._parsed = {
            "jobs": {},
            "name": "",
            "deps": [],
            "description": "",
            "version": "",
            "architecture": "",
            "machine": ""
        }
        self._encountered_machines = {}
        self._output_message_stack = []

    def _add_error(self, message):
        self._output_message_stack.append(ConfigError(message))

    def _add_warning(self, message):
        self._output_message_stack.append(ConfigWarning(message))

    def _replace_env(self, string: str, env: dict):
        for item in env:
            string = string.replace(item, str(env[item]))
        return (string)

    def _fill_metadata(self, schema):
        for item in schema.schemas["metadata"].content:
            obj = self.object_from_dot_ref(item)
            obj_data = schema.schemas["metadata"].content[item]

            try:
                obj = self._validate_data(obj, obj_data, src=item)
            except ValueError:
                continue

            self._parsed[item.split('.')[-1]] = obj

    def _walk_data(self, data, ref: str, /, base = None, env = {}):
        keys = tuple(map(lambda x: int(x) if x.isnumeric() else x, self._replace_env(ref, env).replace('$root', self._ROOT).split('.')))
        node = self._raw_config if base is None else base
 
        for item in keys[:-1]:
            if (isinstance(item, int) and not isinstance(node, list)):
                raise RuntimeError(f"Expected list at {item} for {ref}.")
            if (isinstance(item, str) and not isinstance(node, dict)):
                raise RuntimeError(f"Expected a dict {item} for {ref}.")
            if (isinstance(item, int) and len(node) <= item):
                raise RuntimeError(f"Expected a list at {item} for {ref}.")
            if (isinstance(item, str) and item not in node):
                raise RuntimeError(f"Expected a dict at {item} for {ref}.")
            node = node[item]
        
        if (isinstance(keys[-1], int)):
            node.insert(data, keys[-1])
        else:
            node[keys[-1]] = data

    def _validate_data(self, data, config_key, /, src = "<object>"):
        if (data is not None and not isinstance(data, config_key.type)):
            try:
                data = config_key.type(data)

            except Exception:
                self._add_error(f"{src} should be an instance of {config_key.type}.")
                raise ValueError("Invalid object.")

        if (config_key.required and data is None):
            self._add_error(f"{src} is required.")
            raise ValueError("Invalid object.")

        if (data is None):
            data = config_key.default

        return (data)

    def _fill_steps(self, schema):
        for item in self._parsed["jobs"]:
            for i, step in enumerate(self._parsed["jobs"][item]["steps"]):
                if (len(step.keys()) > 1):
                    self._add_error(f"Wrongly indented step: {step}, steps should be a key referencing a dictionnary of values.")
                    continue

                k = tuple(step.keys())[0]
    
                for content in schema.schemas["step"].content:
                    try:
                        obj = self._validate_data(self.object_from_dot_ref(content, base=step, env={"$step": k}), schema.schemas["step"].content[content], src=content)
                    except ValueError:
                        continue
                    self._walk_data(obj, content, base=step, env={"$step": k})

                if (k not in schema.schemas["steps"].content):
                    self._add_error(f"Invalid step {k}.")
                    continue

                for content in schema.schemas["steps"].content[k].content:
                    try:
                        obj = self._validate_data(self.object_from_dot_ref(content, base=step[k]), schema.schemas["steps"].content[k].content[content], src=content)
                    except ValueError:
                        continue
                    self._walk_data(obj, content, base=step[k])

    def _fill_jobs(self, schema):
        self._parsed["jobs"] = {}

        for item in schema.schemas["jobs"].content:
            try:
                obj = self._validate_data(self.object_from_dot_ref(item), schema.schemas["jobs"].content[item], src=item)
            except ValueError:
                continue
            self._parsed["jobs"][item.split('.')[-1]] = obj

    def _fill_individual_job(self, schema):
        for item in self._parsed["jobs"]:
            for content in schema.schemas["job"].content:
                try:
                    obj = self._validate_data(self.object_from_dot_ref(content, base=self._parsed["jobs"], env={"$job": item}), schema.schemas["job"].content[content], src=content)
                except ValueError:
                    continue
                self._walk_data(obj, content, base=self._parsed["jobs"], env={"$job": item})

    def _machine_str_to_enum(self, machine):
        machine = machine.lower()
        if (machine == "posix" or machine == "linux"):
            return (WizardCodeSection.FLAGS_POSIX)
        if (machine == "windows"):
            return (WizardCodeSection.FLAGS_WINDOWS)

    def _section_type_from_job(self, job):
        if (job == "install"):
            return (WizardCodeSection.TYPE_SECTION_INSTALL)
        if (job == "build"):
            return (WizardCodeSection.TYPE_SECTION_BUILD)
        if (job == "purge"):
            return (WizardCodeSection.TYPE_SECTION_PURGE)
        if (job == "uninstall"):
            return (WizardCodeSection.TYPE_SECTION_UNINSTALL)
        return (WizardCodeSection.TYPE_GENERIC_SECTION)

    def op_from_string(self, name: str, obj: dict):
        if (name == "mkdir"):
            return (WizardInstruction.OP_MKDIR, [WizardArgument(WizardArgument.ARG_STR, obj["path"])])

        if (name == "run"):
            return (WizardInstruction.OP_RUN, [WizardArgument(WizardArgument.ARG_STR, obj["tool"]), WizardArgumentArray(WizardArgument.ARG_STR, obj["args"])])

        if (name == "chmod"):
            return (WizardInstruction.OP_CHMOD, [WizardArgument(WizardArgument.ARG_STR, obj["path"]), WizardArgument(WizardArgument.ARG_U16, obj["mode"])])

        if (name == "remove"):
            return (WizardInstruction.OP_REMOVE, [WizardArgument(WizardArgument.ARG_STR, obj["path"])])

        if (name == "remove_tree"):
            return (WizardInstruction.OP_REMOVE_TREE, [WizardArgument(WizardArgument.ARG_STR, obj["path"])])

        if (name == "copy"):
            return (WizardInstruction.OP_COPY, [WizardArgument(WizardArgument.ARG_STR, obj["from"]), WizardArgument(WizardArgument.ARG_STR, obj["to"])])
        
        return (WizardInstruction.OP_NOOP, [])

    def pop_message(self):
        if (not self._output_message_stack):
            return (None)
        return self._output_message_stack.pop()

    def has_message(self):
        return (not (not self._output_message_stack))

    def object_from_dot_ref(self, ref: str, /, base = None, env = {}):
        keys = map(lambda x: int(x) if x.isnumeric() else x, self._replace_env(ref, env).replace('$root', self._ROOT).split('.'))
        node = self._raw_config if base is None else base
 
        for item in keys:
            if (isinstance(item, int) and not isinstance(node, list)):
                self._add_error(f"Expected list at {item} for {ref}.")
            if (isinstance(item, str) and not isinstance(node, dict)):
                self._add_error(f"Expected a dict {item} for {ref}.")

            if (isinstance(item, int) and len(node) <= item):
                return (None)
            if (isinstance(item, str) and item not in node):
                return (None)
            node = node[item]
        return (node)

    def parse(self):
        if (self._ROOT not in self._raw_config):
            self._add_error(f"Missing configuration root '{self._ROOT}' in config file.")
            return
        if (self.object_from_dot_ref(self._VERSION) is None):
            self._add_warning(f"Missing version key at {self._VERSION} using version {self._LATEST}.")
            self.version = self._LATEST
        else:
            self.version = self.object_from_dot_ref(self._VERSION)
        if (not isinstance(self.version, int)):
            self._add_error(f"Version should be an integer.")
            return
        if (self.version > self._LATEST):
            self._add_error(f"Version not supported VERSION > LATEST ({self.version} > {self._LATEST}).")
            return
        if (self.version < self._MIN_VER):
            self._add_error(f"Version not supported VERSION < MIN_VER ({self.version} < {self._MIN_VER}).")
            return
        if (self.version not in self._SCHEMA_BANK):
            self._add_error(f"Version not supported.")
            return
        schema = self._SCHEMA_BANK[self.version]
        
        if ("metadata" in schema.schemas):
            self._fill_metadata(schema)

        if ("jobs" in schema.schemas):
            self._fill_jobs(schema)

        if ("job" in schema.schemas):
            self._fill_individual_job(schema)

        if ("steps" in schema.schemas):
            self._fill_steps(schema)

        for job in self._parsed["jobs"]:
            self._encountered_machines[job] = []
            for step in self._parsed["jobs"][job]["steps"]:
                machine = step[tuple(step.keys())[0]]["when"]["os"]
                if (machine == "any" or machine in self._encountered_machines):
                    continue
                self._encountered_machines[job].append(machine)

    def to_package_builder(self, asset_path, /, temp_dir = "./temp") -> PackageBuilder:
        builder = PackageBuilder(asset_path, PackageInfo(
            self._parsed["name"],
            self._parsed["version"],
            self._parsed["description"],
            self._parsed["deps"],
            self._parsed["machine"],
            self._parsed["architecture"]
        ), temp_dir)

        builder.wizard.add_section(WizardSection("metadata", WizardSection.TYPE_SECTION_METADATA, WizardSection.FLAGS_DEFAULT))
        
        for job in self._parsed["jobs"]:
            any_only = len(self._encountered_machines[job]) == 0
            one_arch = len(self._encountered_machines[job]) == 1
            code_sections = {}

            if (any_only):
                code_sections["any"] = WizardCodeSection(f"{job}_any", self._section_type_from_job(job), WizardCodeSection.FLAGS_POSIX & WizardCodeSection.FLAGS_WINDOWS)
            elif (one_arch):
                arch = self._encountered_machines[job][0]
                code_sections[arch] = WizardCodeSection(f"{job}_{arch}", self._section_type_from_job(job), self._machine_str_to_enum(arch))
            else:
                for item in self._encountered_machines[job]:
                    code_sections[item] = WizardCodeSection(f"{job}_{item}", self._section_type_from_job(job), self._machine_str_to_enum(item))

            for item in self._parsed["jobs"][job]["steps"]:
                k, v = tuple(item.keys())[0], item[tuple(item.keys())[0]]
                os = v["when"]["os"]

                if (os == "any"):
                    for section in code_sections.values():
                        section.instructions.append(WizardInstruction(*self.op_from_string(k, v)))
                else:
                    section = code_sections[os]

                    section.instructions.append(WizardInstruction(*self.op_from_string(k, v)))

            for item in code_sections.values():
                builder.wizard.add_section(item)

        return (builder)

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
    print("not implemented.")

    return (1)

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

def init_typedef():
    instruction_loader: InstructionLoader = InstructionLoader()

    try:
        instruction_loader.load("../../assets/wizard/instructions.xml")
    except Exception as e:
        print(e)

    for typedef in instruction_loader.data_types:
        size, is_str = instruction_loader.data_types[typedef]
        WizardArgument._ARGS[typedef] = size

        if (is_str):
            WizardArgument._STR_ARGS.append(typedef)

def config_file_mode():
    config: YAMLConfigReader = YAMLConfigReader(argv[2])
    config.parse()

    has_error = False
    while (config.has_message()):
        msg = config.pop_message()

        if (not has_error and isinstance(msg, ConfigError)):
            has_error = True
        print(msg)

    if (has_error):
        return (1)

    pkg_builder = config.to_package_builder(argv[1])

    pkg_builder.generate_package()

    return (0)

def main() -> int:
    if (len(argv) < 2):
        print(f"{argv[0]}: not enough argument.")
        return (1)

    if (not isdir(argv[1])):
        print(f"{argv[0]}: {argv[1]}: not a directory.")
        return (1)
    
    init_typedef()
    
    if (len(argv) < 3):
        print(f"{argv[0]} No config file provided.")
        #print("no config file provided, starting interactive mode.")
        #return (interactive_mode())
        return (1)

    try:
        return (config_file_mode())
    except Exception:
        print(f"{argv[0]}: Failed to generate package from config '{argv[2]}'.")
        return (1)

if (__name__ == "__main__"):
    exit(main())
