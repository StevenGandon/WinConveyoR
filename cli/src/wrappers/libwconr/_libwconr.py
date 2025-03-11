from ctypes import Structure, POINTER, c_byte, c_ubyte, c_short, c_ushort, c_int, c_uint, c_long, c_ulong, c_double, c_float, c_size_t
from enum import Enum
from sys import platform

from ..dllloader import DLLoader

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
    def __init__(self, path: str = f"lib/libwconr/libwconr.{'dll' if platform.startswith("win") else 'so'}") -> None:
        self._dll: DLLoader = DLLoader(path)

        self.map_functions()

    def call_function(self, name: str,  *args):
        return self._dll.call_function(name, *args)

    def map_functions(self) -> None:
        self._dll.register_function("new_state", POINTER(wcr_state_s))
        self._dll.register_function("close_state", None, POINTER(wcr_state_s))
