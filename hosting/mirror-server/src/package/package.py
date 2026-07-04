from os.path import join, isfile, isdir, basename, dirname, splitext, relpath
from datetime import datetime
from os import mkdir, remove
from shutil import copyfile
from hashlib import sha256, md5

from ..common import hash_file

from .package_listing import PackageListing

class Package(object):
    def __init__(self, name="", location=None, latest = None, hsh = 0, backup_path: str = None, /, load: bool=False, recursive_load: bool = False, base_path = None):
        self.name = name
        self.latest = latest
        self.location = location.replace('\\', '/')
        self.hash = hsh
        self.listing = {}


        if (not base_path):
            base_path = "."

        self.backup_path = backup_path if backup_path else join(base_path, "backups")
        self.base_path = base_path

        self.loaded = False
        self.recursive_load = recursive_load

        if (load):
            self.load()

    def __str__(self):
        return f"{'~' if not self.loaded else ''}Package[{self.name}]<{self.hash}~{self.latest}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.listing[x]}', self.listing))}]"

    def __repr__(self):
        return (self.__str__())

    def load(self):
        if (not self.location):
            return

        with open(join(self.base_path, self.location.lstrip('/')), 'r') as fp:
            data = {}

            for item in fp.readlines():
                if not item.strip():
                    temp = PackageListing(data.get("Architecture", "ukn"), data.get("Version", "0.0.0"), data.get("Machine", "ukn"), data.get("Location", "???"), load=self.recursive_load, base_path=self.base_path)
                    self.listing[temp.package_data.get("SHA256", f"???-{data.get('Version', '???')}")] = temp
                    data.clear()
                    continue

                data[item.split(':')[0].strip()] = ':'.join(item.split(':')[1:]).strip()

            if (data):
                temp = PackageListing(data.get("Architecture", "ukn"), data.get("Version", "0.0.0"), data.get("Machine", "ukn"), data.get("Location", "???"), load=self.recursive_load, base_path=self.base_path)
                self.listing[temp.package_data.get("SHA256", f"???-{data.get('Version', '???')}")] = temp
                data.clear()

        self.loaded = True

    def write(self, backup_parent):
        hsh = sha256()
        _linebreak: str = "\n" 

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
                content = f"Architecture: {item.architecture}\nVersion: {item.version}\nMachine: {item.machine}\nLocation: {item.location}\n{_linebreak if last - 1 != i else ''}".encode()
                fp.write(content)
                hsh.update(content)

        self.hash = hsh.hexdigest()

    def add_package_listing(self, package_metadata, package_location, *, package_files_dir = None) -> PackageListing:
        if (not self.loaded):
            self.load()

        if (package_files_dir is None):
            package_files_dir = join(self.base_path, "pkgs")

        if (not isfile(package_location)):
            print("file not found, remember, remote package is not implemented.")
            return
        
        arch = package_metadata.get("architecture", "x64")
        version = package_metadata.get("version", "1.0.0")
        machine = package_metadata.get("machine", "Gen-Linux")
        description = package_metadata.get("description", f"{self.name} package.")
        deps = package_metadata.get("deps", [])
        name = f"{self.name}_{arch}_{machine.lower()}_{version.replace('.', '-')}"
        path = copyfile(package_location, join(package_files_dir, name + splitext(package_location)[1]))
        json_path = join(package_files_dir, name + ".json")
        package_hash = hash_file(path)

        self.listing[package_hash] = PackageListing(
            arch,
            version,
            machine,
            '/' + relpath(json_path, self.base_path).replace('\\', '/').lstrip('/'), base_path=self.base_path
        )
        self.listing[package_hash].package_data = {
            "package": self.name,
            "version": version,
            "architecture": arch,
            "machine": machine,
            "depends": deps,
            "description": description,
            "address": '/' + relpath(path, self.base_path).replace('\\', '/').lstrip('/'),
            "added_at": round(datetime.now().timestamp()),
            "SHA256": package_hash,
            "MD5sum": hash_file(path, md5)
        }

        self.listing[package_hash].loaded = True

        if (self.latest):
            self.latest = version if int(version.replace('.', '')) > int(self.latest.replace('.', '')) else self.latest
        else:
            self.latest = version

        return (self.listing[package_hash])

    def remove_package_listing(self, hsh, *, hard_delete = False):
        if (not self.loaded):
            self.load()
        
        listing_location = join(self.base_path, self.listing[hsh].location.lstrip('/'))

        if (hard_delete and isfile(listing_location)):
            if (not isdir(self.backup_path)):
                mkdir(self.backup_path)
            if (not isdir(join(self.backup_path, "deleted"))):
                mkdir(join(self.backup_path, "deleted"))
            filename, *extension = basename(listing_location).split('.')
            copyfile(listing_location, join(self.backup_path, "deleted", filename + '-' + hash_file(listing_location, md5) + '.' + '.'.join(extension)))
            remove(listing_location)

        del self.listing[hsh]
