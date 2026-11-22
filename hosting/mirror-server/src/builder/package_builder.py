from json import dump, load
from os.path import join, basename, isdir
from os import mkdir, walk
from glob import glob
from tarfile import open as open_tar
from src.common import hash_file
from hashlib import md5, sha256

from .package_info import PackageInfo

from ..wizard import PackageWizard

class PackageBuilder(object):
    def __init__(self, asset_path, pkg_info, temp_dir = "./temp"):
        self.asset_path = asset_path
        self.pkg_info: PackageInfo = pkg_info
        self.temp_dir = temp_dir
        self.hashs = {}
        self.hash = ""

        self.wizard = PackageWizard()

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

        for file_name in glob(join(self.asset_path, "*"), include_hidden=True):
            tar.add(file_name, basename(file_name))

        tar.close()

    def pack_package(self, output_path: str):
        tar = open_tar(output_path, "w:gz")

        for file in glob(join(self.temp_dir, self.hash, "*"), include_hidden=True):
            tar.add(file, basename(file))

        tar.close()

    def generate_package(self):
        self.pkg_info.generate_json(join(
            self.temp_dir,
            f"infos_{self.pkg_info.name}_{self.pkg_info.architecture}_{self.pkg_info.machine}_{self.pkg_info.version.replace('.', '-')}"
        ))

        self.pack_assets(join(self.temp_dir, self.hash , "package.tar.gz"))

        self.wizard.write(join(self.temp_dir, self.hash, ".WIZARD"))

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

        self.pack_package(join(
            self.temp_dir,
            f"{self.pkg_info.name}_{self.pkg_info.architecture}_{self.pkg_info.machine}_{self.pkg_info.version.replace('.', '-')}.tar.gz"
        ))
