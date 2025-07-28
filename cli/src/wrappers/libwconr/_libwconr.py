from ctypes import Structure, POINTER, c_short, c_ushort, c_int, c_uint, c_long, c_ulong, c_double, c_float, c_size_t, c_ssize_t
from ctypes import c_byte as c_char
from ctypes import c_ubyte as c_uchar
from enum import Enum
from sys import platform

from ..dllloader import DLLoader, find_dll

# ==== Enums ==== #

class SUPPORTED_ARCHITECTURES(Enum):
    UNSUPPORTED_ARCH = -(1 << 0)
    X86_64_ARCH = (1 << 0)
    ARM64_ARCH = (1 << 1)
    I686_ARCH = (1 << 2)

class SUPPORTED_PLATFORMS(Enum):
    UNSUPPORTED_PLTF = -(1 << 0)
    NT_PLTF = (1 << 0)
    DARWIN_PLTF = (1 << 1)
    GEN_LINUX_PLTF = (1 << 2)


# ==== Structs ==== #

class wcr_system_s(Structure):
    pass

wcr_system_s._fields_ = [
    ("arch", c_short),
    ("platform", c_short)
]

class wcr_state_s(Structure):
    pass

wcr_state_s._fields_ = [
    ("system_informations", wcr_system_s)
]


# ==== Interfaces ==== #

class Mapper(object):
    def __init__(self, path: str = f"libwconr.{'dll' if platform.startswith('win') else ('dylib' if platform.startswith('darwin') else 'so')}") -> None:
        self.inited: bool = False
        self.base_dll_path: str = path
        self.dll_location: str = None
        self._dll: DLLoader = None

    def __del__(self):
        self.inited = False

        del self._dll

        self._dll = None

    def init_mapper(self):
        self.dll_location: str = self.base_dll_path # find_dll(self.base_dll_path, 'nt' if platform.startswith("win") else 'posix')

        if (not self.dll_location):
            raise OSError(f"DLL {self.base_dll_path} not found in any $PATH or registered path via `add_dll_registry_path` nor cwd and cwd/lib.")

        self._dll: DLLoader = DLLoader(self.dll_location)
        self.inited: bool = True

        self.map_functions()

    def call_function(self, name: str,  *args):
        if (not self.inited):
            self.init_mapper()

        return self._dll.call_function(name, *args)

    def map_functions(self) -> None:
        if (not self._dll):
            self.init_mapper()
        self._dll.register_function("new_state", POINTER(wcr_state_s))
        self._dll.register_function("close_state", None, POINTER(wcr_state_s))
        self._dll.register_function("download_package", c_int, POINTER(None), POINTER(None))
