from os.path import join, abspath, isfile, isdir, basename
from datetime import datetime
from os import mkdir, remove
from shutil import copyfile
from hashlib import sha256, md5

from ..common import hash_file

from .package import Package

class MirrorServer(object):
    def __init__(self, location: str = None, backup_path: str = None, /, load: bool = False, recursive_load: bool = False, register_path = "register", packages_path = "pkgs", metadata_path = "pkgs"):
        if (location):
            location = abspath(location)

        self.checksum = "CD" * 32
        self.packages: dict = {}

        self.register_path = register_path.replace('\\', '/')
        self.package_path = packages_path.replace('\\', '/')
        self.metadata_path = metadata_path.replace('\\', '/')

        self.location = location.replace('\\', '/')
        self.loaded = False
        self.recursive_load = recursive_load

        self.backup_path = (backup_path if backup_path else join(self.location, "backups"))

        if (load):
            self.load()

    def __str__(self):
        return (f"{'~' if not self.loaded else ''}Mirror<{self.checksum}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.packages[x]}', self.packages))}]")

    def __repr__(self):
        return self.__str__()

    def write(self):
        hsh = sha256()
        _linebreak: str = '\n'

        if (not self.location or not self.loaded):
            return

        if (not isdir(self.backup_path)):
            mkdir(self.backup_path)
        
        backups_location = join(self.backup_path, datetime.now().strftime("%Y%m%d-%H%M%S"))

        if (not isdir(join(self.location, backups_location))):
            mkdir(join(self.location, backups_location))

        if (isfile(join(self.location, "pkgs.list"))):
            copyfile(join(self.location, "pkgs.list"), join(self.location, backups_location, "pkgs.list"))
        if (isfile(join(self.location, "checksum.hsh"))):
            copyfile(join(self.location, "pkgs.list"), join(self.location, backups_location, "pkgs.list"))

        for item in self.packages.values():
            if (item.loaded):
                item.write(backups_location)

        with open(join(self.location, "pkgs.list"), 'wb+') as fp:
            last = len(self.packages)

            for i, item in enumerate(self.packages.values()):
                computed_string = f"{item.name},{item.latest if item.latest else 'null'},{item.location},{item.hash}{_linebreak if last - 1 != i else ''}".encode()
                fp.write(computed_string)
                hsh.update(computed_string)

        self.checksum = hsh.hexdigest()

        with open(join(self.location, "checksum.hsh"), 'wb+') as fp:
            fp.write(str(self.checksum).encode())

    def load(self):
        if (not self.location):
            return

        if (isfile(join(self.location, "pkgs.list"))):
            self.checksum: str = hash_file(join(self.location, "pkgs.list"))

            with open(join(self.location, "pkgs.list"), 'r') as fp:
                line = fp.readline()
                while line:
                    line = line.replace('\r\n', '\n').strip()
                    name, version, location, sha256 = tuple(filter(lambda x: len(x), map(lambda x: x.strip(), line.split(','))))
                    self.packages[name] = Package(name, location, version, sha256, load=self.recursive_load, recursive_load=self.recursive_load, base_path=self.location)

                    line = fp.readline()
        else:
            self.checksum = "0"
        self.loaded = True

    def add_package_register(self, name) -> Package:
        if (not self.loaded):
            self.load()

        if (name not in self.packages):
            self.packages[name] = Package(name, join(f"/{self.register_path}", name + ".list"), base_path=self.location)
            self.packages[name].loaded = True

        return (self.packages[name])

    def get_package_register(self, name) -> Package:
        if (not self.loaded):
            self.load()

        return (self.packages[name])

    def remove_package_register(self, name, *, hard_delete = False) -> None:
        if (not self.loaded):
            self.load()

        if (not self.location):
            return

        pkg_location = join(self.location, self.packages[name].location.lstrip('/'))

        if (hard_delete and isfile(pkg_location)):
            if (not isdir(self.backup_path)):
                mkdir(self.backup_path)
            if (not isdir(join(self.backup_path, "deleted"))):
                mkdir(join(self.backup_path, "deleted"))
            filename, *extension = basename(pkg_location).split('.')
            copyfile(pkg_location, join(self.backup_path, "deleted", filename + '-' + hash_file(pkg_location, md5) + '.' + '.'.join(extension)))
            remove(pkg_location)

        del self.packages[name]