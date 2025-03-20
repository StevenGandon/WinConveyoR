from ctypes import CDLL
from genericpath import isfile

import os

_DLL_PATHS_POSIX = {
    "./",
    "./lib/",
    *tuple(map(lambda x: x if x.endswith('/') else x + '/', filter(lambda x: x, os.environ.get("PATH", "").replace('\\', '/').split(':'))))
}

_DLL_PATHS_NT = {
    ".\\",
    ".\\lib\\",
    *tuple(map(lambda x: x if x.endswith('\\') else x + '\\', filter(lambda x: x, os.environ.get("PATH", "").replace('/', '\\').split(':'))))
}

def find_dll(name: str, platform = "posix") -> str:
    if (platform.lower == "nt"):
        registry: set = _DLL_PATHS_NT
        name: str = name.replace('/', '\\').strip('\\')
    else:
        registry: set = _DLL_PATHS_POSIX
        name: str = name.replace('\\', '/').strip('/')

    for item in registry:
        if (isfile(item + name)):
            return (item + name)
    return (None)


def add_dll_registry_path(path: str) -> None:
    if (not path.endswith('/') and not path.endswith('\\')):
        path += "/"
    _DLL_PATHS_NT.add(path.replace('/', '\\'))
    _DLL_PATHS_POSIX.add(path.replace('\\', '/'))

class _SymbolRegister(object):
    def __init__(self, name: str, symbol_ptr, response_type, *arguments) -> None:
        self.name: str = name
        self.symbol_ptr = symbol_ptr
        self.response_type = response_type
        self.arguments = [*arguments]

        self.symbol_ptr.restype = response_type
        self.symbol_ptr.argtypes = tuple(arguments)

class DLLoader(object):
    def __init__(self, path: str) -> None:
        self._path = path
        self._registered_functions = {}
        self._dll: CDLL = CDLL(path)

    def register_function(self, name: str, response_type, *arguments) -> None:
        if (not self._dll):
            raise RuntimeError("dll not open.")
        if (not hasattr(self._dll, name)):
            raise ReferenceError(f"symbol {name} not found in dll {self._path}.")
        self._registered_functions[name] = _SymbolRegister(name, getattr(self._dll, name), response_type, *arguments)

    def call_function(self, name: str, *arguments):
        if (not self._registered_functions[name]):
            raise ReferenceError(f"symbol {name} not registered.")
        return (self._registered_functions[name].symbol_ptr(*arguments))

    def __del__(self) -> None:
        del self._dll

        self._dll = None