from sys import exit
from hashlib import sha256, md5
from json import load, dump
from shutil import copyfile
from os import mkdir, stat, remove
from os.path import join, abspath, isfile, isdir, basename, dirname
from datetime import datetime
from warnings import warn

def hash_file(file, algorithm = sha256, /, buffer_size = 65536):
    base_hash = algorithm()

    with open(file, 'rb') as fp:
        data = fp.read(buffer_size)

        while data:
            base_hash.update(data)
            data = fp.read(buffer_size)

    return base_hash.hexdigest()

class UndoAction(object):
    def __init__(self, name = "generic_action"):
        self.name = name

        self.do = []

class UndoStack(object):
    def __init__(self):
        self.undo_stack = []
        self.redo_stack = []

        self.registering_action = None

    def start_regisering_undo(self, name = "generic_action"):
        self.redo_stack.clear()

        self.registering_action = name

    def register_action(self):
        if (self.registering_action is None):
            raise RuntimeError("Can't register action when undo action has not started.")

    def end_regisering_undo(self):
        self.undo_stack.append(self.register_action)
        self.register_action = None

    def undo(self):
        if (self.registering_action is not None):
            warn("can't undo while registering an action.", UserWarning)
            return

        action = self.undo_stack.pop()

        self.redo_stack.append(action)

    def redo(self):
        if (self.registering_action is not None):
            warn("can't redo while registering an action.", UserWarning)
            return

        action = self.redo_stack.pop()

        self.undo_stack.append(action)

class PackageListing(object):
    def __init__(self, architecture, version, machine, location = None, /, load: bool = False, base_path = None):
        self.architecture = architecture
        self.version = version
        self.machine = machine
        self.package_data = {}

        if (not base_path):
            base_path = "."

        self.base_path = base_path

        self.location = location.replace('\\', '/')

        self.loaded = False

        if (load):
            self.load()

    def __str__(self):
        return f"{'~' if not self.loaded else ''}PkgItem<{self.version}>:{self.machine}#{self.architecture}@{self.location} -> ({self.package_data})"

    def __repr__(self):
        return (self.__str__())

    def load(self):
        if (not self.location):
            return

        with open(join(self.base_path, self.location.lstrip('/')), 'r') as fp:
            self.package_data = load(fp)

        self.loaded = True

    def write(self, backup_parent):
        if (not self.loaded or not self.location):
            return
        
        if ("address" in self.package_data):
            pkg_location = join(self.base_path, self.package_data["address"].lstrip('/'))
            if (isfile(pkg_location)):
                # the file is on the same server maybe add the fact that the file can come from somewhere else
                self.package_data["SHA256"] = hash_file(pkg_location, sha256)
                self.package_data["MD5sum"] = hash_file(pkg_location, md5)
                self.package_data["size"] = stat(pkg_location).st_size

        backup_dir = join(backup_parent, dirname(self.location.lstrip('/')))

        if (not isdir(backup_dir)):
            mkdir(backup_dir)

        if (isfile(join(self.base_path, self.location.lstrip('/')))):
            copyfile(join(self.base_path, self.location.lstrip('/')), join(backup_parent, self.location.lstrip('/')))

        with open(join(self.base_path, self.location.lstrip('/')), "w+") as fp:
            dump(self.package_data, fp, indent=4)

class Package(object):
    def __init__(self, name="", location=None, latest = None, hsh = 0, /, load: bool=False, base_path = None):
        self.name = name
        self.latest = latest
        self.location = location.replace('\\', '/')
        self.hash = hsh
        self.listing = {}

        if (not base_path):
            base_path = "."

        self.base_path = base_path

        self.loaded = False

        if (load):
            self.load()

    def __str__(self):
        return f"{'~' if not self.loaded else ''}Package[{self.name}]<{self.hash}~{self.latest}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.listing[x]}', self.listing))}]"

    def __repr__(self):
        return (self.__str__())

    def load(self):
        if (not self.location):
            return
        
        self.listing.clear()

        with open(join(self.base_path, self.location.lstrip('/')), 'r') as fp:
            data = {}

            for item in fp.readlines():
                if not item.strip():
                    temp = PackageListing(data.get("Architecture", "ukn"), data.get("Version", "0.0.0"), data.get("Machine", "ukn"), data.get("Location", "???"), load=True, base_path=self.base_path)
                    self.listing[temp.package_data.get("SHA256", f"???-{data.get('Version', '???')}")] = temp
                    data.clear()
                    continue

                data[item.split(':')[0].strip()] = ':'.join(item.split(':')[1:]).strip()

            if (data):
                temp = PackageListing(data.get("Architecture", "ukn"), data.get("Version", "0.0.0"), data.get("Machine", "ukn"), data.get("Location", "???"), load=True, base_path=self.base_path)
                self.listing[temp.package_data.get("SHA256", f"???-{data.get('Version', '???')}")] = temp
                data.clear()

        self.loaded = True

    def write(self, backup_parent):
        hsh = sha256()

        if (not self.loaded or not self.location):
            return
        
        backup_dir = join(backup_parent, dirname(self.location.lstrip('/')))

        if (not isdir(backup_dir)):
            mkdir(backup_dir)

        if (isfile(join(self.base_path, self.location.lstrip('/')))):
            copyfile(join(self.base_path, self.location.lstrip('/')), join(backup_parent, self.location.lstrip('/')))

        for item in self.listing.values():
            if (item.loaded):
                item.write(backup_parent)

        with open(join(self.base_path, self.location.lstrip('/')), 'wb+') as fp:
            last = len(self.listing)

            for i, item in enumerate(self.listing.values()):
                content = f"Architecture: {item.architecture}\nVersion: {item.version}\nMachine: {item.machine}\nLocation: {item.location}\n{'\n' if last - 1 != i else ''}".encode()
                fp.write(content)
                hsh.update(content)

        self.hash = hsh.hexdigest()

class MirrorServer(object):
    def __init__(self, location: str = None, backup_path: str = None, /, load: bool = False):
        if (location):
            location = abspath(location)
            self.checksum: str = hash_file(join(location, "pkgs.list"))
        else:
            self.checksum = 0
        self.packages: dict = {}

        self.location = location.replace('\\', '/')
        self.loaded = False

        self.backup_path = (backup_path if backup_path else join(self.location, "backups"))

        if (load):
            self.load()

    def __str__(self):
        return (f"{'~' if not self.loaded else ''}Mirror<{self.checksum}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.packages[x]}', self.packages))}]")

    def __repr__(self):
        return self.__str__()

    def write(self):
        hsh = sha256()

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
                computed_string = f"{item.name},{item.latest if item.latest else "nul"},{item.location},{item.hash}{'\n' if last - 1 != i else ''}".encode()
                fp.write(computed_string)
                hsh.update(computed_string)

        self.checksum = hsh.hexdigest()

        with open(join(self.location, "checksum.hsh"), 'wb+') as fp:
            fp.write(str(self.checksum).encode())

    def load(self):
        if (not self.location):
            return

        if (isfile(join(self.location, "pkgs.list"))):
            with open(join(self.location, "pkgs.list"), 'r') as fp:
                line = fp.readline()
                while line:
                    line = line.replace('\r\n', '\n').strip()
                    name, version, location, sha256 = tuple(filter(lambda x: len(x), map(lambda x: x.strip(), line.split(','))))
                    self.packages[name] = Package(name, location, version, sha256, load=True, base_path=self.location)

                    line = fp.readline()

        self.loaded = True

    def add_package_register(self, name) -> str:
        if (not self.loaded):
            self.load()

        if (name not in self.packages):
            self.packages[name] = Package(name, join("/register", name + ".list"), base_path=self.location)
            self.packages[name].loaded = True

    def remove_package_register(self, name, *, hard_delete = False):
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
            copyfile(pkg_location, join(self.backup_path, "deleted", basename(pkg_location)))
            remove(pkg_location)
        
        del self.packages[name]

def main() -> int:
    ms = MirrorServer(".")
    ms.load()

    print(ms)
    ms.write()
    return (0)

if (__name__== "__main__"):
    exit(main())