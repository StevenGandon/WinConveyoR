from os.path import join, abspath, isfile, isdir, isabs, basename
from datetime import datetime
from os import mkdir, remove, listdir, makedirs
from shutil import copyfile, copy2, rmtree
from hashlib import sha256, md5
from pathlib import Path

from ..common import hash_file
from ..undo import UndoStack, UndoStepSetItem, UndoStepDelItem

from .package import Package

class MirrorServer(object):
    def __init__(self, location: str = None, backup_path: str = None, /, load: bool = False, recursive_load: bool = False, register_path = "register", packages_path = "pkgs", metadata_path = "pkgs"):
        if (location):
            location = abspath(location)

        self.checksum = "CD" * 32
        self.packages: dict = {}

        self.undo_stack = UndoStack()

        self.register_path = register_path.replace('\\', '/')
        self.package_path = packages_path.replace('\\', '/')
        self.metadata_path = metadata_path.replace('\\', '/')

        self.location = location.replace('\\', '/') if location else location
        self.loaded = False
        self.recursive_load = recursive_load

        if (backup_path):
            self.backup_path = backup_path if isabs(backup_path) else join(self.location or ".", backup_path)
        else:
            self.backup_path = join(self.location, "backups") if self.location else "backups"

        self.backup_path = self.backup_path.replace('\\', '/')

        if (load):
            self.load()

    def __str__(self):
        return (f"{'~' if not self.loaded else ''}Mirror<{self.checksum}>@{self.location} -> [{', '.join(map(lambda x: f'{x}: {self.packages[x]}', self.packages))}]")

    def __repr__(self):
        return self.__str__()

    def write(self) -> str | None:
        hsh = sha256()
        _linebreak: str = '\n'

        if (not self.location or not self.loaded):
            return None

        if (not isdir(self.backup_path)):
            makedirs(self.backup_path, exist_ok=True)

        backup_name = datetime.now().strftime("%Y%m%d-%H%M%S")
        backups_location = join(self.backup_path, backup_name)

        if (not isdir(backups_location)):
            makedirs(backups_location, exist_ok=True)

        if (isfile(join(self.location, "pkgs.list"))):
            copyfile(join(self.location, "pkgs.list"), join(backups_location, "pkgs.list"))
        if (isfile(join(self.location, "checksum.hsh"))):
            copyfile(join(self.location, "checksum.hsh"), join(backups_location, "checksum.hsh"))

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

        return backup_name

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
                    self.packages[name] = Package(name, location, version, sha256, load=self.recursive_load, recursive_load=self.recursive_load, base_path=self.location, undo_stack=self.undo_stack)

                    line = fp.readline()
        else:
            self.checksum = "0"
        self.loaded = True

    def add_package_register(self, name) -> Package:
        if (not self.loaded):
            self.load()

        if (name not in self.packages):
            self.undo_stack.start_regisering_undo(f"add_package_register:{name}")

            self.packages[name] = Package(name, join(f"/{self.register_path}", name + ".list"), base_path=self.location, undo_stack=self.undo_stack)
            self.packages[name].loaded = True

            self.undo_stack.register_action(UndoStepSetItem(self.packages, name))
            self.undo_stack.end_regisering_undo()

        return (self.packages[name])

    def get_package_register(self, name) -> Package:
        if (not self.loaded):
            self.load()

        return (self.packages[name])

    def has_package_register(self, name) -> Package:
        if (not self.loaded):
            self.load()

        return (name in self.packages)

    def remove_package_register(self, name, *, hard_delete = False) -> None:
        if (not self.loaded):
            self.load()

        if (not self.location):
            return

        if (name not in self.packages):
            return

        pkg_location = join(self.location, self.packages[name].location.lstrip('/'))

        self.undo_stack.start_regisering_undo(f"remove_package_register:{name}")

        if (hard_delete and isfile(pkg_location)):
            if (not isdir(self.backup_path)):
                mkdir(self.backup_path)
            if (not isdir(join(self.backup_path, "deleted"))):
                mkdir(join(self.backup_path, "deleted"))
            filename, *extension = basename(pkg_location).split('.')
            copyfile(pkg_location, join(self.backup_path, "deleted", filename + '-' + hash_file(pkg_location, md5) + '.' + '.'.join(extension)))
            remove(pkg_location)

        self.undo_stack.register_action(UndoStepDelItem(self.packages[name], self.packages, name))

        del self.packages[name]

        self.undo_stack.end_regisering_undo()

    def undo(self):
        self.undo_stack.undo()

    def redo(self):
        self.undo_stack.redo()

    def list_backups(self) -> list:
        if (not isdir(self.backup_path)):
            return ([])

        return sorted(
            [
                p
                for p in listdir(self.backup_path)
                if isdir(join(self.backup_path, p))
            ]
        )

    def latest_backup(self) -> str | None:
        backups = self.list_backups()
        return backups[-1] if backups else None

    def _snapshot_live(self, name: str) -> str:
        snapshot_dir = Path(join(self.backup_path, name))
        snapshot_dir.mkdir(parents=True, exist_ok=True)

        location = Path(self.location)
        backup_root = Path(self.backup_path)

        for src in location.rglob("*"):
            if src.is_dir():
                continue

            try:
                src.relative_to(backup_root)
                continue
            except ValueError:
                pass

            rel = src.relative_to(location)
            dst = snapshot_dir / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            copy2(src, dst)

        return name

    def restore(self, backup_name: str) -> str:
        backups = self.list_backups()

        if backup_name not in backups:
            raise FileNotFoundError(join(self.backup_path, backup_name))

        safety_name = datetime.now().strftime("%Y%m%d-%H%M%S-%f") + "-prerestore"
        self._snapshot_live(safety_name)

        backups = self.list_backups()
        relevant = backups[backups.index(backup_name):]

        resolved: dict[str, Path] = {}

        for name in relevant:
            backup_dir = Path(join(self.backup_path, name))

            if not backup_dir.is_dir():
                continue

            for src in backup_dir.rglob("*"):
                if src.is_dir():
                    continue

                rel = src.relative_to(backup_dir).as_posix()

                if rel not in resolved:
                    resolved[rel] = src

        for rel, src in resolved.items():
            dst = Path(self.location) / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            copy2(src, dst)

        self.packages = {}
        self.loaded = False
        self.undo_stack = UndoStack()
        self.load()

        return safety_name
        latest = self.latest_backup()

        if latest is None:
            raise RuntimeError("No backups found.")

        return self.restore(latest)

    def verify(self) -> bool:
        if (not self.location or not isfile(join(self.location, "pkgs.list"))):
            return False

        return hash_file(join(self.location, "pkgs.list")) == self.checksum

    def delete_backup(self, backup_name: str) -> None:
        backup = Path(join(self.backup_path, backup_name))

        if (not backup.exists() or not backup.is_dir()):
            raise FileNotFoundError(backup)

        rmtree(backup)

    def prune_backups(self, keep: int = 5) -> list:
        backups = self.list_backups()
        to_delete = backups[:-keep] if keep > 0 else backups

        for name in to_delete:
            self.delete_backup(name)

        return to_delete