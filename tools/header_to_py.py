from sys import exit, argv
from genericpath import isfile
from os import access, O_RDONLY
from re import compile as compile_regex
from re import RegexFlag, sub

import sys

class Token(object):
    regex = None

class Symbol(Token):
    regex = compile_regex(r"((struct *[a-z0-9_]+)|[a-z0-9_]+) *[*a-z0-9_]+\((void|((((struct *[a-z0-9_]+)|[a-z0-9_]+) *[*a-z0-9_]+)(, ((struct *[a-z0-9_]+)|[a-z0-9_]+) *[*a-z0-9_]+)*))\)")

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

    def _parse(self, fp):
        data = fp.read()

        for item in Symbol.regex.finditer(data):
            raw = (item.string[item.start():item.end()])
            name = raw.split("(")[0].split(' ')[-1].split('*')[-1].strip()
            return_type = raw.split(name)[0].strip()
            arguments = list(map(lambda x: x.strip(), ')'.join('('.join(raw.split('(')[1:]).split(')')[:-1]).split(',')))

            # return_typename = return_type.split('*')[-1].split(' ')[-1]
            return_type_type = return_type.replace('*', '').strip().replace("unsigned", '').replace('signed', '').strip()
            return_type_signed = None

            if return_type.startswith("signed"):
                return_type_signed = True

            if return_type.startswith("unsigned"):
                return_type_signed = False

            for i, item in enumerate(arguments):
                arg_name = item.split('*')[-1].split(' ')[-1].strip()
                if (not arg_name):
                    arg_name = None

                arg_type = item.replace('*', '').strip().split(arg_name if arg_name else None)[0].replace("unsigned", '').replace('signed', '').strip()
                arg_signed = None

                if item.startswith("signed"):
                    arg_signed = True

                if item.startswith("unsigned"):
                    arg_signed = False
                arguments[i] = TypeValue(arg_type, arg_name, item.count('*'), arg_signed)

            self.symbols.append(Symbol(name, TypeValue(return_type_type, None, return_type.count('*'), return_type_signed) , arguments))

        for item in Struct.regex.finditer(data):
            raw = (item.string[item.start():item.end()])
            raw = ''.join(map(lambda x: x.split('//')[0].strip(), sub(r'\/\*(.+)\*\/', '', raw, flags=RegexFlag.S).split("\n")))
            name = raw.split("struct")[1].split('{')[0].strip()
            arguments = list(filter(lambda x: x.strip(), '}'.join('{'.join(raw.split('{')[1:]).split('}')[:-1]).split(';')))

            for i, item in enumerate(arguments):
                arg_name = item.split('*')[-1].split(' ')[-1].strip()
                if (not arg_name):
                    arg_name = None

                arg_type = item.replace('*', '').strip().split(arg_name if arg_name else None)[0].replace("unsigned", '').replace('signed', '').strip()
                arg_signed = None

                if item.startswith("signed"):
                    arg_signed = True

                if item.startswith("unsigned"):
                    arg_signed = False
                arguments[i] = TypeValue(arg_type, arg_name, item.count('*'), arg_signed)

            self.structs.append(Struct(name, arguments))

        for item in Enum.regex.finditer(data):
            raw = (item.string[item.start():item.end()])
            raw = ''.join(map(lambda x: x.split('//')[0].strip(), sub(r'\/\*(.+)\*\/', '', raw, flags=RegexFlag.S).split("\n")))
            name = raw.split("enum")[1].split('{')[0].strip()
            arguments = list(map(lambda n: tuple(map(lambda v: v.strip(), n.split('='))), list(filter(lambda x: x.strip(), '}'.join('{'.join(raw.split('{')[1:]).split('}')[:-1]).split(',')))))

            self.enums.append(Enum(name, arguments))

        print(self.symbols, self.structs, self.enums, sep="\n")

def main() -> int:
    if (len(argv) < 2):
        sys.stderr.write(argv[0] + ": missing header file path.\n")
        return (1)

    if (len(argv) > 2):
        sys.stderr.write(argv[0] + ": too many arguments.\n")
        return (1)

    if (not isfile(argv[1])):
        sys.stderr.write(argv[0] + f": {argv[1]}: file not found.\n")
        return (1)

    if (not access(argv[1], O_RDONLY)):
        sys.stderr.write(argv[0] + f": {argv[1]}: read permission missing.\n")
        return (1)

    with open(argv[1], 'r') as fp:
        hf: HeaderFile = HeaderFile(fp)

    return (0)

if (__name__ == "__main__"):
    exit(main())