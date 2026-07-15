from os.path import join, isfile, isdir, dirname
from os import mkdir, stat, makedirs
from shutil import copyfile
from json import load, dump
from hashlib import sha256, md5

from ..common import hash_file

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
            else:
                print(f"warning: package {pkg_location} not found, remember, remote package is not implemented.")

        backup_dir = join(backup_parent, dirname(self.location.lstrip('/')))
        live_dir = join(self.base_path, dirname(self.location.lstrip('/')))

        if (not isdir(backup_dir)):
            makedirs(backup_dir, exist_ok=True)

        if (not isdir(live_dir)):
            makedirs(live_dir, exist_ok=True)

        if (isfile(join(self.base_path, self.location.lstrip('/')))):
            copyfile(join(self.base_path, self.location.lstrip('/')), join(backup_parent, self.location.lstrip('/')))

        with open(join(self.base_path, self.location.lstrip('/')), "w+") as fp:
            dump(self.package_data, fp, indent=4)