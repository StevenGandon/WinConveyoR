from yaml import safe_load
from re import fullmatch

from .schema import ConfigSchemaBank, ConfigSchema, ConfigKey
from .message import ConfigError, ConfigMessage, ConfigWarning

from ...wizard import WizardInstruction, WizardArgument, WizardSection, WizardCodeSection, WizardArgumentArray, PackageWizard
from ..package_builder import PackageBuilder, PackageInfo

class YAMLConfigReader(object):
    _ROOT = "wizard"
    _VERSION = "$root.version"
    _LATEST = 1
    _MIN_VER = 1
    _OP_KEYS = {}
    _MACHINES = [
        "posix",
        "windows",
        "linux",
        "macos"
    ]
    _OPTION_TYPES = [
        "path",
        "user",
        "mode",
        "string"
    ]
    _OPTION_NAME_REGEX = r"[A-Za-z_][A-Za-z0-9_]*"
    _OPTIONS = "$root.options"
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
                }),
                "rmdir": ConfigSchema({
                    "path": ConfigKey(str, required=True)
                }),
                "move": ConfigSchema({
                    "from": ConfigKey(str, required=True),
                    "to": ConfigKey(str, required=True)
                }),
                "move_tree": ConfigSchema({
                    "from": ConfigKey(str, required=True),
                    "to": ConfigKey(str, required=True)
                }),
                "chown": ConfigSchema({
                    "path": ConfigKey(str, required=True),
                    "user": ConfigKey(str, required=True)
                }),
                "chmod_str": ConfigSchema({
                    "path": ConfigKey(str, required=True),
                    "mode": ConfigKey(str, required=True)
                })
            }),
            option=ConfigSchema({
                "name": ConfigKey(str, required=True),
                "type": ConfigKey(str, required=False, default="string"),
                "prompt": ConfigKey(str, required=False, default=""),
                "default": ConfigKey(str, required=False, default="")
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
            "machine": "",
            "options": []
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

    def _mode_to_str(self, mode):
        # YAML reads 0755 as the integer 493, keep it as an octal string
        if (isinstance(mode, int) and not isinstance(mode, bool)):
            return (f"0{mode:o}")
        return (mode)

    def _fill_option(self, schema, option, index, seen):
        parsed = {}

        if (not isinstance(option, dict)):
            self._add_error(f"options.{index} should be a dictionnary.")
            return (None)

        if (option.get("type", None) == "mode"):
            option["default"] = self._mode_to_str(option.get("default", None))

        for key in schema.schemas["option"].content:
            try:
                parsed[key] = self._validate_data(option.get(key, None), schema.schemas["option"].content[key], src=f"options.{index}.{key}")
            except ValueError:
                return (None)

        if (not fullmatch(self._OPTION_NAME_REGEX, parsed["name"])):
            self._add_error(f"options.{index}.name '{parsed['name']}' is not a valid variable name.")
            return (None)
        if (parsed["type"] not in self._OPTION_TYPES):
            self._add_error(f"options.{index}.type '{parsed['type']}' should be one of {self._OPTION_TYPES}.")
            return (None)
        if (parsed["name"] in seen):
            self._add_error(f"options.{index}.name '{parsed['name']}' is declared twice.")
            return (None)

        if (not parsed["prompt"]):
            parsed["prompt"] = parsed["name"]
        return (parsed)

    def _fill_options(self, schema):
        options = self.object_from_dot_ref(self._OPTIONS)
        seen = []

        if (options is None):
            return
        if (not isinstance(options, list)):
            self._add_error(f"{self._OPTIONS} should be a list.")
            return

        for i, option in enumerate(options):
            parsed = self._fill_option(schema, option, i, seen)

            if (parsed is None):
                continue
            seen.append(parsed["name"])
            self._parsed["options"].append(parsed)

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

                if (k == "chmod_str" and isinstance(step[k], dict) and "mode" in step[k]):
                    step[k]["mode"] = self._mode_to_str(step[k]["mode"])

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
        if (name not in self._OP_KEYS):
            raise (ValueError(f"Instruction '{name}' not found in instruction set for version '{self.version}'."))

        code, args = self._OP_KEYS[name]

        return (code, [
            WizardArgument(
                item.name,
                obj.get(item.label, None)
            ) if not item.is_list else WizardArgumentArray(
                item.name,
                obj.get(item.label, [None] * item.list_size)
            ) for item in args
        ])

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

        if ("option" in schema.schemas):
            self._fill_options(schema)

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

        builder.wizard = self.to_wizard()
        return (builder)

    # the compiled procedure alone (.WIZARD), also used to override a package procedure (F30)
    def to_wizard(self) -> PackageWizard:
        wizard = PackageWizard()

        wizard.add_section(WizardSection("metadata", WizardSection.TYPE_SECTION_METADATA, WizardSection.FLAGS_DEFAULT))

        if (self._parsed["options"]):
            options_section = WizardCodeSection("options", WizardSection.TYPE_SECTION_OPTIONS, WizardSection.FLAGS_DEFAULT)

            for option in self._parsed["options"]:
                options_section.instructions.append(WizardInstruction(*self.op_from_string("option", option)))
            wizard.add_section(options_section)
        
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
                    if ("any" not in code_sections):
                        code_sections["any"] = WizardCodeSection(f"{job}_any", self._section_type_from_job(job), WizardCodeSection.FLAGS_POSIX & WizardCodeSection.FLAGS_WINDOWS)
                    for section in code_sections.values():
                        section.instructions.append(WizardInstruction(*self.op_from_string(k, v)))
                else:
                    section = code_sections[os]

                    section.instructions.append(WizardInstruction(*self.op_from_string(k, v)))

            for item in code_sections.values():
                wizard.add_section(item)

        return (wizard)
