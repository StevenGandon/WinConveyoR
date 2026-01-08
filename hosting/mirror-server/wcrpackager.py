from sys import exit
from sys import argv
from json import dump

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

def ask_until_given(text):
    content = ""

    while (not content):
        content = input(text).strip()

    return (content)

def ask_until_empty(text):
    content = "empty"
    items = []

    while (content):
        content = input(text).strip()
        if (content):
            items.append(content)

    return (items)

def ask_enum(text, possibilities):
    content = ""

    while (content not in possibilities):
        content = input(text).strip()

        if (content not in possibilities):
            print(f"invalid value, should be within these ones: ({', '.join(possibilities)})")

    return (content)

def ask_with_default(text, default):
    content = input(text).strip()

    if (not content):
        return (default)
    return (content)

def interactive_mode():
    print("-===========[ Informations ]===========-")
    
    package_name = ask_until_given("package name (nodefault): ")
    package_description = ask_with_default(f"package description (default: '{package_name} package.'): ", package_name)
    package_version = ask_with_default(f"package version (default: 1.0.0): ", "1.0.0")
    package_deps = ask_until_empty(f"package dependencies (leave empty to stop defining dependencies): ")
    package_machine = ask_with_default(f"package machine (default: windows): ", "windows")
    package_architecture = ask_with_default(f"package architecture (default: x64): ", "x64")

    pkg_info = PackageInfo(package_name, package_version, package_description, package_deps, package_machine, package_architecture)

    pkg_info.generate_json(f"./infos_{package_name}_{package_architecture}_{package_machine}_{package_version.replace('.', '-')}")

    print("-===========[ Content ]===========-")

    package_type = ask_enum("package installation type (nodefault) (static, compile): ", ["static", "compile"])



    return (0)

def config_file_mode():
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
