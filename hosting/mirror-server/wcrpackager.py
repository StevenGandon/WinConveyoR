#!/usr/bin/python3

from sys import exit, argv
from genericpath import isdir

from src import *

def init_typedef():
    instruction_loader: InstructionLoader = InstructionLoader()

    try:
        instruction_loader.load("../../assets/wizard/typedef_x64.xml")
    except Exception as e:
        print(e)

    for typedef in instruction_loader.data_types:
        size, is_str = instruction_loader.data_types[typedef]
        WizardArgument._ARGS[typedef] = size

        if (is_str):
            WizardArgument._STR_ARGS.append(typedef)

def init_ops(version):
    instruction_loader: InstructionLoader = InstructionLoader()

    try:
        instruction_loader.load("../../assets/wizard/instructions_v0.xml")
    except Exception as e:
        print(e)

    if (version not in instruction_loader.instructions_set):
        print(f"Version '{version}' not found in xml instructions configurations.")

    for item in instruction_loader.instructions_set[version]:
        YAMLConfigReader._OP_KEYS[item] = instruction_loader.instructions_set[version][item]

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
    
    init_ops(config.version)

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
    except Exception as e:
        print(f"{argv[0]}: Failed to generate package from config '{argv[2]}'. ({e})")
        return (1)

if (__name__ == "__main__"):
    exit(main())
