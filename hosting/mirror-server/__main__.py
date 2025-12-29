from sys import exit
from hashlib import sha256, md5
from json import load
from shutil import copyfile
from os import mkdir
from os.path import join, abspath, isfile, isdir
from datetime import datetime

def hash_file(file, algorithm = sha256, /, buffer_size = 65536):
    base_hash = algorithm()

    with open(file, 'rb') as fp:
        data = fp.read(buffer_size)

        while data:
            base_hash.update(data)
            data = fp.read(buffer_size)

    return base_hash.hexdigest()

class PackageListing(object):
    def __init__(self, architecture, version, machine, location = None, /, load: bool = False, base_path = None):
        self.architecture = architecture
        self.version = version
        self.machine = machine
        self.package_data = {}

        if (not base_path):
            base_path = "."

        self.base_path = base_path

        self.location = location

        if (load):
            self.load()

        self.loaded = False

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

class Package(object):
    def __init__(self, name="", location=None, latest = None, /, load: bool=False, base_path = None):
        self.name = name
        self.latest = latest
        self.location = location
        self.listing = {}

        if (not base_path):
            base_path = "."

        self.base_path = base_path

        self.loaded = False

        if (load):
            self.load()

    def __str__(self):
        return f"{'~' if not self.loaded else ''}Package[{self.name}]<{self.latest}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.listing[x]}', self.listing))}]"

    def __repr__(self):
        return (self.__str__())

    def load(self):
        if (not self.location):
            return

        with open(join(self.base_path, self.location.lstrip('/')), 'r') as fp:
            data = {}

            for item in fp.readlines():
                if not item.strip():
                    temp = PackageListing(data.get("Architecture", "ukn"), data.get("Version", "0.0.0"), data.get("Machine", "ukn"), data.get("Location", "???"), load=True, base_path=self.base_path)
                    self.listing[temp.package_data.get("SHA256", "???")] = temp
                    data.clear()
                    continue

                data[item.split(':')[0].strip()] = ':'.join(item.split(':')[1:]).strip()

            if (data):
                temp = PackageListing(data.get("Architecture", "ukn"), data.get("Version", "0.0.0"), data.get("Machine", "ukn"), data.get("Location", "???"), load=True, base_path=self.base_path)
                self.listing[temp.package_data.get("SHA256", "???")] = temp
                data.clear()

            self.listing

        self.loaded = True

class MirrorServer(object):
    def __init__(self, location: str = None, /, load: bool = False):
        if (location):
            location = abspath(location)
            self.checksum: str = hash_file(join(location, "pkgs.list"))
        else:
            self.checksum = 0
        self.packages: dict = {}

        self.location = location
        self.loaded = False

        if (load):
            self.load()

    def __str__(self):
        return (f"{'~' if not self.loaded else ''}Mirror<{self.checksum}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.packages[x]}', self.packages))}]")

    def __repr__(self):
        return self.__str__()

    def write(self):
        hsh = sha256()

        if (not self.location):
            return
        
        if (not isdir(join(self.location, "backups"))):
            mkdir(join(self.location, "backups"))
        
        backups_location = join("backups", datetime.now().strftime("%Y%m%d-%H%M%S"))

        if (not isdir(join(self.location, backups_location))):
            mkdir(join(self.location, backups_location))

        if (isfile(join(self.location, "pkgs.list"))):
            copyfile(join(self.location, "pkgs.list"), join(self.location, backups_location, "pkgs.list"))
        if (isfile(join(self.location, "checksum.hsh"))):
            copyfile(join(self.location, "pkgs.list"), join(self.location, backups_location, "pkgs.list"))
            
    
        with open(join(self.location, "pkgs.list"), 'wb+') as fp:
            for item in self.packages.values():
                computed_string = f"{item.name},{item.listing[item.latest].version},{item.location},{item.latest}".encode()
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
                    self.packages[name] = Package(name, location, sha256, load=True, base_path=self.location)

                    line = fp.readline()

        self.loaded = True

    def generate_package_listing():
        pass

    def add_package():
        pass

    def update_package():
        pass

def main() -> int:
    ms = MirrorServer(".")
    ms.load()

    print(ms)
    ms.write()
    return (0)

if (__name__== "__main__"):
    exit(main())