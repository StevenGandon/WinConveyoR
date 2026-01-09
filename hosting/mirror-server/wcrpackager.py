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

class PackageBuilder(object):
    def __init__(self, asset_path, pkg_info, temp_dir = "./temp"):
        self.asset_path = asset_path
        self.pkg_info: PackageInfo = pkg_info
        self.temp_dir = temp_dir
        self.hashs = {}
        self.hash = ""

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

        for file_name in glob(join(self.asset_path, "*")):
            tar.add(file_name, basename(file_name))

        tar.close()

    def generate_package(self):
        self.pkg_info.generate_json(join(
            self.temp_dir,
            f"infos_{self.pkg_info.name}_{self.pkg_info.architecture}_{self.pkg_info.machine}_{self.pkg_info.version.replace('.', '-')}"
        ))

        self.pack_assets(join(self.temp_dir, self.hash , "package.tar.gz"))

        with open(join(self.temp_dir, self.hash, ".PKG_INFO"), "wb+") as fp:
            pass

        with open(join(self.temp_dir, self.hash, ".INSTALL"), "wb+") as fp:
            pass

        with open(join(self.temp_dir, self.hash, ".BUILD"), "wb+") as fp:
            pass

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

    print("-===========[ Installation ]===========-")

    package_type = Dialog.ask_enum("package installation type (nodefault) (static, compile, custom): ", ["static", "compile", "custom"])

    print("-===========[ Content ]===========-")

    P = PackageBuilder(argv[1], pkg_info)
    P.generate_package()

    return (0)

def config_file_mode():
    with open(argv[2], "r") as fp:
        config = load(fp)

    P = PackageBuilder(argv[1], PackageInfo(
        config["name"],
        config["version"],
        config["description"],
        config["deps"],
        config["machine"],
        config["architecture"]
    ))

    P.generate_package()

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
