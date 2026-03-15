from xml.etree import ElementTree

class InstructionArgumentDef(object):
    def __init__(self, name, /, is_list = False, list_size = -1, label =  None):
        self.name = name
        self.is_list = is_list
        self.list_size = list_size
        self.label = label

    @staticmethod
    def from_string(string: str):
        name = string
        is_list = False
        list_size = -1

        if ("[" in string and "]" in string):
            name = string.split("[")[0]
            is_list = True
            list_size = string.split("[")[1].split("]")[0]

            if (not list_size):
                list_size = -1

            else:
                if (not list_size.isnumeric()):
                    raise ValueError(f"Invalid list size '{list_size}', expected a numerical value or nothing.")
                list_size = int(list_size)

        return InstructionArgumentDef(name, is_list=is_list, list_size=list_size)

class InstructionLoader(object):
    def __init__(self):
        self.instructions_set = {}
        self.data_types = {}

    def verify(self):
        for instruction_set in self.instructions_set.values():
            for item in instruction_set:
                for arg in self.instructions_set[item][1]:
                    if (arg.name in self.data_types):
                        continue

                raise ValueError(f"Data type '{arg.name}' not found, for '{item}'.")

    def load(self, file):
        try:
            tree: ElementTree = ElementTree.parse(file)
        except Exception as e:
            raise ValueError(f"Failed to parse {file}, {e}")

        root = tree.getroot()

        if (root.tag != "wizard"):
            raise ValueError(f"Root tag should be 'wizard'.")

        for item in root:
            if (item.tag == "typedef"):
                for typedef in item:
                    if (typedef.tag != "type"):
                        raise ValueError(f"Invalid tag in typedef '{typedef.tag}'.")
                    name = typedef.attrib.get("name", "")
                    size = typedef.attrib.get("size", "0")
                    is_str = typedef.attrib.get("is_str", "no").lower()

                    if (is_str != "yes" and is_str != "no"):
                        raise ValueError(f"Invalid attribute, is_str should only be 'yes' or 'no', for typedef '{name}'.")

                    if ("[" in name or "]" in name):
                        raise ValueError(f"Invalid characters in typename '{name}', '[' and ']' can't be used in type name.")

                    if (not size.isnumeric()):
                        raise ValueError(f"Invalid size in type, got {size}, but expected a numeric value.")

                    if (int(size) == 0):
                        print(f"warning: '{name}' has no size set.")

                    if (name in self.data_types):
                        print(f"warning: '{name}' already registered as a type, overwritting.")
                    self.data_types[name] = (int(size), is_str == "yes")
                continue

            if (item.tag == "instructions"):
                set_name = item.attrib.get("version", "0")

                if (not set_name.isnumeric()):
                    raise ValueError(f"Version should be a numeric value for instruction set, got: '{set_name}'.")

                set_name = int(set_name)

                if (set_name in self.instructions_set):
                    print(f"warning: '{set_name}' already registered as an instruction set, overwritting.")

                self.instructions_set[set_name] = {}

                for instruction in item:
                    if (instruction.tag != "op"):
                        raise ValueError(f"Invalid tag in instructions: '{instruction.tag}'.")
                    name = instruction.attrib.get("name", "")
                    code = instruction.attrib.get("code", "-1")

                    if (not code.lstrip('-').isnumeric()):
                        raise ValueError(f"Invalid code in type, got '{code}', but expected a numeric value.")

                    if (int(code) == -1):
                        print(f"warning: '{name}' op has no code set.")

                    if (name in self.instructions_set[set_name]):
                        print(f"warning: '{name}' already registered as an instruction, overwritting.")

                    self.instructions_set[set_name][name] = (int(code), [])

                    for arg in instruction:
                        if (arg.tag != "arg"):
                            raise ValueError(f"Invalid tag in instruction {name} '{arg.tag}'.")
                        
                        type_name = arg.attrib.get("type", "")
                        label = arg.attrib.get("label", None)

                        if (label is None):
                            print(f"warning: argument '{type_name}' of '{name}' has no label, it will cause an assertion if used to generate wizard.")

                        temp = InstructionArgumentDef.from_string(type_name)
                        temp.label = label

                        self.instructions_set[set_name][name][1].append(temp)

                continue

            raise ValueError(f"Invalid tag in root '{item.tag}'.")
