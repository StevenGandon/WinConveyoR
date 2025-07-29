from sys import exit, argv
from genericpath import isfile
from os import access, O_RDONLY, environ
from os.path import join
from re import compile as compile_regex
from re import RegexFlag, sub
from enum import Enum
from pathlib import Path
from xml.etree import ElementTree
import sys

environ["TOOL_ROOT"] = str(Path(__file__).parent)

EXPORT_CONST_PYTHON = """"""

EXPORT_ENUM_CONST_PYTHON = """"""

EXPORT_STRUCT_CONST_PYTHON = """"""

EXPORT_SYMBOL_CONST_PYTHON = """"""

class Token(object):
    regex = None

class Symbol(Token):
    regex = compile_regex(r"(((struct *[a-z0-9_]+)|(const |)(unsigned |signed |)[a-z0-9_]+) *[*a-z0-9_]+\((void|((((struct *[a-z0-9_]+)|(const |)(unsigned |signed |)[a-z0-9_]+) *[*a-z0-9_]+)(, ((struct *[a-z0-9_]+)|(const |)(|unsigned |signed |)[a-z0-9_]+) *[*a-z0-9_]+)*))\))")

    def __init__(self, name: str, return_value: str, arguments: list):
        self.name = name
        self.return_value = return_value
        self.arguments = arguments

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return f"<Symbol name={self.name} ret={self.return_value} args={self.arguments}>"

class TypeValue(Token):
    def __init__(self, type_name, name = None, ptr_count = 0, signed = None):
        self.name = name
        self.type_name = type_name
        self.ptr_count = ptr_count
        self.signed = signed

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return f"<TypeValue name={self.name} type={self.type_name} ptr={self.ptr_count} signed={'Auto' if self.signed is None else self.signed}>"

class Struct(Token):
    regex = compile_regex(r"(typedef\s+)?(?P<PACKED>\w+)?\s*(?P<TYPE>struct)\s+(?P<NAME2>\w+)\s*\{(?P<FIELDS>.*?(?:\{.*?\}.*?)*?)\}(?P<NAME>[^;]*);", RegexFlag.S)

    def __init__(self, name: str, fields: list):
        self.name = name
        self.fields = fields

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return f"<Struct name={self.name} fields={self.fields}>"

class Enum(Token):
    regex = compile_regex(r"(typedef\s+)?(?P<PACKED>\w+)?\s*(?P<TYPE>enum)\s+(?P<NAME2>\w+)\s*\{(?P<FIELDS>.*?(?:\{.*?\}.*?)*?)\}(?P<NAME>[^;]*);", RegexFlag.S)

    def __init__(self, name: str, values: list):
        self.name = name
        self.values = values

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return f"<Enum name={self.name} values={self.values}>"

class HeaderFile(object):
    def __init__(self, fp):
        self.symbols = []
        self.structs = []
        self.enums = []

        if (fp.closed):
            raise RuntimeError("cannot execute operation on a closed file.")
        if (not fp.readable()):
            raise RuntimeError("cannot execute operation on a non-readable file.")
        self._parse(fp)

    def _var_to_type_value(self, raw_str):
        raw_str = raw_str.strip()

        arg_name = raw_str.split('*')[-1].split(' ')[-1].strip() if  raw_str.split('*')[-1].split(' ')[-1].strip() != raw_str else ""
        if (not arg_name):
            arg_name = None

        if (arg_name):
            arg_type = raw_str.replace('*', '').strip().split(arg_name)[0].replace("unsigned", '').replace('signed', '').strip()
        else:
            arg_type = raw_str.replace('*', '').strip().replace("unsigned", '').replace('signed', '').strip()
        arg_signed = None

        if raw_str.startswith("signed"):
            arg_signed = True

        if raw_str.startswith("unsigned"):
            arg_signed = False
        return (TypeValue(arg_type, arg_name, raw_str.count('*'), arg_signed))

    def _parse(self, fp):
        data = fp.read()

        for item in Symbol.regex.finditer(data):
            raw = item.group(1)
            name = raw.split("(")[0].split(' ')[-1].split('*')[-1].strip()
            return_type = raw.split(name)[0].strip()
            arguments = list(map(lambda x: x.strip(), ')'.join('('.join(raw.split('(')[1:]).split(')')[:-1]).split(',')))

            for i, it in enumerate(arguments):
                arguments[i] = self._var_to_type_value(it)

            self.symbols.append(Symbol(name, self._var_to_type_value(return_type), arguments))

        for item in Struct.regex.finditer(data):
            raw = (item.string[item.start():item.end()])
            raw = ''.join(map(lambda x: x.split('//')[0].strip(), sub(r'\/\*(.+)\*\/', '', raw, flags=RegexFlag.S).split("\n")))
            name = raw.split("struct")[1].split('{')[0].strip()
            arguments = list(filter(lambda x: x.strip(), '}'.join('{'.join(raw.split('{')[1:]).split('}')[:-1]).split(';')))

            for i, it in enumerate(arguments):
                arguments[i] = self._var_to_type_value(it)

            self.structs.append(Struct(name, arguments))

        for item in Enum.regex.finditer(data):
            raw = (item.string[item.start():item.end()])
            raw = ''.join(map(lambda x: x.split('//')[0].strip(), sub(r'\/\*(.+)\*\/', '', raw, flags=RegexFlag.S).split("\n")))
            name = raw.split("enum")[1].split('{')[0].strip()
            arguments = list(map(lambda n: tuple(map(lambda v: v.strip(), n.split('='))), list(filter(lambda x: x.strip(), '}'.join('{'.join(raw.split('{')[1:]).split('}')[:-1]).split(',')))))

            self.enums.append(Enum(name, arguments))

class Converter(object):
    def __init__(self, header_file: HeaderFile):
        self.header_file = header_file

    def _type_to_ctype(self, type_value):
        return ("")

    def convert(self) -> str:
        return ("")

class PythonConverter(Converter):
    def __init__(self, header_file: HeaderFile):
        super().__init__(header_file)

    def _type_to_ctype(self, type_value: TypeValue):
        if (type_value.type_name == "void"):
            ctype = "None"
        elif (type_value.type_name.startswith("struct ")):
            ctype = type_value.type_name.split("struct ")[1]
        elif (("c_" + ('u' if type_value.signed == False else '') + type_value.type_name) in EXPORT_CONST_PYTHON):
            ctype = "c_" + ('u' if type_value.signed == False else '') + (type_value.type_name if type_value.type_name else "int")
        else:
            ctype = "None"

        for _ in range(type_value.ptr_count):
            ctype = 'POINTER(' + ctype + ')'

        return (ctype)

    def convert(self) -> str:
        export = EXPORT_CONST_PYTHON

        temp = map(lambda item: EXPORT_ENUM_CONST_PYTHON.replace("$name$", item.name).replace("$arguments$", '\n'.join(map(lambda x: f'    {x[0]} = {x[1]}', item.values))), self.header_file.enums)
        export = export.replace("$mapped_enums$", '\n'.join(temp))

        temp = map(lambda item: EXPORT_STRUCT_CONST_PYTHON.replace("$name$", item.name).replace("$fields$", ',\n'.join(map(lambda x: f'    ("{x.name}", {self._type_to_ctype(x)})', item.fields))), self.header_file.structs)
        export = export.replace("$mapped_structs$", '\n'.join(temp))

        temp = map(lambda item: EXPORT_SYMBOL_CONST_PYTHON.replace("$name$", item.name).replace("$return_type$", self._type_to_ctype(item.return_value)).replace("$arguments$", (', ' if tuple(filter(lambda x: x.type_name != "void" or x.ptr_count > 0, item.arguments)) else '') + ', '.join(map(self._type_to_ctype, filter(lambda x: x.type_name != "void" or x.ptr_count > 0, item.arguments)))), self.header_file.symbols)
        export = export.replace("$mapped_functions$", '\n'.join(temp))

        return (export)

class GenericConverter(Converter):
    def __init__(self, header_file, langpack_file):
        super().__init__(header_file)

        self.schemas = {}
        self.build_rules = {"env": None, "builds": {}}
        self.type_conversion = None

        try:
            ET = ElementTree.parse(langpack_file)
        except Exception as e:
            raise RuntimeError(f"Failed to parse langpack {langpack_file}: {e}.")

        root = ET.getroot()

        if (root.tag != "converter"):
            raise (RuntimeError(f"Invalid root tag name '{root.tag}', expected: 'converter'."))

        schema = None
        build = None
        type_conversion = None

        for item in root:
            if (item.tag == "schemas"):
                schema = item
            if (item.tag == "build"):
                build = item
            if (item.tag == "type-conversion"):
                type_conversion = item

        for scheme in schema:
            if (scheme.attrib.get("type", "ukn") == "file"):
                if ("for" not in scheme.attrib):
                    continue
                text = scheme.text.replace('\r', '').replace('\n', '').strip()

                for item in environ:
                    text = text.replace("${" + str(item) + "}", environ[item])

                with open(text, 'r') as fp:
                    self.schemas[scheme.attrib.get("for")] = fp.read()

        if (type_conversion.attrib.get("strategy", "ukn") == "code"):
            self.type_conversion = compile(f"""def _type_conversion_wrapper(type_value):\n{'\n'.join(map(lambda x: '  ' + x, type_conversion.text.split('\n')))}\nreturn_value = _type_conversion_wrapper(type_value)\n""", "type_conversion", "exec", 0)

        def build_builder(item):
            if (item.attrib.get("strategy", "ukn") == "code"):
                return f"""def build():\n{'\n'.join(map(lambda x: '  ' + x, item[0].text.split('\n')))}\nreturn_value = build()"""
            if (item.attrib.get("strategy", "ukn") == "replace"):
                return f"""def replacer(schema):\n{'\n'.join(f"  schema = schema.replace('{replace.attrib.get('name', '')}', {replace.text})" for replace in item)}\n  return (schema)\nreturn_value = replacer(schema)"""

        for item in build:
            if (item.attrib.get("type", "ukn") == "build"):
                self.build_rules["builds"][item.tag] = build_builder(item)
            if (item.attrib.get("type", "ukn") == "env"):
                self.build_rules["env"] = (item.tag, build_builder(item))

    def _type_to_ctype(self, type_value: TypeValue):
        _globals = globals()
        _locals = locals()
        _locals["return_value"] = None

        if (self.type_conversion is None):
            return (_locals["return_value"])

        exec(self.type_conversion, globals=_globals, locals=_locals)

        return (_locals["return_value"])

    def convert(self) -> str:
        _globals = globals()
        _locals = locals()
        _globals["enums"] = self.header_file.enums
        _globals["structs"] = self.header_file.structs
        _globals["symbols"] = self.header_file.symbols
        _globals["_type_to_ctype"] = self._type_to_ctype

        for item in self.build_rules["builds"]:
            _globals["schema"] = None

            if (item in self.schemas):
                _globals["schema"] = self.schemas[item]

            _locals["return_value"] = None

            exec(self.build_rules["builds"][item], globals=_globals, locals=_locals)

            _globals[item] = _locals["return_value"]

        if (not self.build_rules["env"]):
            return ('')

        _globals["schema"] = None
        _locals["return_value"] = None

        if (self.build_rules["env"][0] in self.schemas):
            _globals["schema"] = self.schemas[self.build_rules["env"][0]]

        exec(self.build_rules["env"][1], globals=_globals, locals=_locals)
        return (_locals["return_value"])

def main() -> int:
    if (len(argv) < 2):
        sys.stderr.write(argv[0] + ": missing header file path.\n")
        return (1)

    if (len(argv) < 3):
        sys.stderr.write(argv[0] + ": missing langpack name.\n")
        return (1)

    if (len(argv) > 3):
        sys.stderr.write(argv[0] + ": too many arguments.\n")
        return (1)

    if (not isfile(argv[1])):
        sys.stderr.write(argv[0] + f": {argv[1]}: file not found.\n")
        return (1)

    if (not isfile(join(environ["TOOL_ROOT"], "langpacks", argv[2], "converter.xml"))):
        sys.stderr.write(argv[0] + f": {argv[2]}: langpack not found.\n")
        return (1)

    if (not access(argv[1], O_RDONLY)):
        sys.stderr.write(argv[0] + f": {argv[1]}: read permission missing.\n")
        return (1)

    with open(argv[1], 'r') as fp:
        hf: HeaderFile = HeaderFile(fp)

    print(GenericConverter(hf, join(environ["TOOL_ROOT"], "langpacks", argv[2], "converter.xml")).convert())

    return (0)

if (__name__ == "__main__"):
    exit(main())