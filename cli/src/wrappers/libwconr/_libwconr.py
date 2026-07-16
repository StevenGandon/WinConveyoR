from ctypes import Structure, POINTER, c_short, c_ushort, c_int, c_uint, c_long, c_ulong, c_double, c_float, c_size_t, c_ssize_t, c_void_p, c_char_p
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

class protocol_type(Enum):
    PROT_HTTP = 0
    PROT_WCR = 1


# ==== Structs ==== #

class wcr_system_s(Structure):
    pass

wcr_system_s._fields_ = [
    ("arch", c_short),
    ("platform", c_short)
]

class wcr_source_s(Structure):
    pass

wcr_source_s._fields_ = [
    ("url", c_char_p),
    ("proto", c_int),
    ("access_key", c_char_p),
    ("server_pubkey_path", c_char_p)
]

class wcr_state_s(Structure):
    pass

wcr_state_s._fields_ = [
    ("system_informations", wcr_system_s),
    ("cache_path", c_char_p),
    ("config_path", c_char_p),
    ("sources", POINTER(POINTER(wcr_source_s))),
    ("sources_count", c_size_t),
    ("lock", c_char * 64),
    ("event_callback", c_void_p),
    ("event_user_data", POINTER(None))
]


class wcr_installed_pkg_s(Structure):
    pass

wcr_installed_pkg_s._fields_ = [
    ("name", c_char_p),
    ("version", c_char_p),
    ("is_dependency", c_int)
]

class wcr_available_pkg_s(Structure):
    pass

wcr_available_pkg_s._fields_ = [
    ("name", c_char_p),
    ("version", c_char_p)
]

class wcr_pkg_variant_s(Structure):
    pass

wcr_pkg_variant_s._fields_ = [
    ("version", c_char_p),
    ("arch", c_char_p),
    ("machine", c_char_p)
]

class wcr_pkg_metadata_s(Structure):
    pass

wcr_pkg_metadata_s._fields_ = [
    ("name", c_char_p),
    ("version", c_char_p),
    ("arch", c_char_p),
    ("machine", c_char_p),
    ("description", c_char_p),
    ("address", c_char_p),
    ("sha256", c_char_p),
    ("md5", c_char_p),
    ("depends", POINTER(c_char_p)),
    ("depends_count", c_size_t),
    ("size", c_long),
    ("added_at", c_long)
]

class wcr_hash_check_s(Structure):
    pass

wcr_hash_check_s._fields_ = [
    ("expected", c_char_p),
    ("actual", c_char_p),
    ("match", c_int)
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
        self.dll_location: str = find_dll(self.base_dll_path, 'nt' if platform.startswith("win") else 'posix') or self.base_dll_path

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
        self._dll.register_function("download_package", c_int, c_char_p, c_char_p)
        self._dll.register_function("write_state", c_int, POINTER(wcr_state_s), c_char_p)
        self._dll.register_function("load_state", POINTER(wcr_state_s), c_char_p)
        self._dll.register_function("wcr_state_add_source", c_int, POINTER(wcr_state_s), c_int, c_char_p)
        self._dll.register_function("sync_package_list", c_int, POINTER(wcr_state_s), c_int, c_char_p)
        self._dll.register_function("install_package", c_int, POINTER(wcr_state_s), c_int, c_char_p, c_char_p)
        self._dll.register_function("uninstall_package", c_int, POINTER(wcr_state_s), c_char_p)
        self._dll.register_function("list_installed", c_int, POINTER(wcr_state_s), POINTER(POINTER(wcr_installed_pkg_s)), POINTER(c_size_t))
        self._dll.register_function("free_installed_list", None, POINTER(wcr_installed_pkg_s), c_size_t)
        self._dll.register_function("wcr_source_set_auth", c_int, POINTER(wcr_state_s), c_size_t, c_char_p, c_char_p)
        self._dll.register_function("search_available", c_int, POINTER(wcr_state_s), c_char_p, POINTER(POINTER(wcr_available_pkg_s)), POINTER(c_size_t))
        self._dll.register_function("free_available_list", None, POINTER(wcr_available_pkg_s), c_size_t)
        self._dll.register_function("list_package_variants", c_int, POINTER(wcr_state_s), c_int, c_char_p, c_char_p, POINTER(POINTER(wcr_pkg_variant_s)), POINTER(c_size_t))
        self._dll.register_function("free_variant_list", None, POINTER(wcr_pkg_variant_s), c_size_t)
        self._dll.register_function("get_package_metadata", c_int, POINTER(wcr_state_s), c_int, c_char_p, c_char_p, POINTER(wcr_pkg_metadata_s))
        self._dll.register_function("free_package_metadata", None, POINTER(wcr_pkg_metadata_s))
        self._dll.register_function("verify_cached_package", c_int, POINTER(wcr_state_s), c_int, c_char_p, c_char_p, POINTER(wcr_hash_check_s))
        self._dll.register_function("free_hash_check", None, POINTER(wcr_hash_check_s))
        self._dll.register_function("wcr_set_event_callback", None, POINTER(wcr_state_s), c_void_p, POINTER(None))

