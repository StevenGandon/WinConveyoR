from ctypes import CDLL
from os.path import abspath, dirname, isfile, join

import os
import sys

# Lookup order (first match wins, deterministic):
#   1. next to the frozen executable (PyInstaller): the lib shipped with this build
#   2. ./ and ./lib/ relative to the current directory
#   3. directories added with add_dll_registry_path()
#   4. every directory of $PATH
# The result is always an absolute path: on Windows a drive-less path such as
# "/Users/x/bin/libwconr.dll" passes isfile() but ctypes refuses to load it.

_DEFAULT_DIRS = [".", "lib"]
_REGISTERED_DIRS = []

def _frozen_dirs() -> list:
    if (not getattr(sys, "frozen", False)):
        return ([])
    dirs = [dirname(sys.executable)]
    if (hasattr(sys, "_MEIPASS")):
        dirs.append(sys._MEIPASS)
    return (dirs)

def _env_path_dirs() -> list:
    # os.pathsep: ';' on Windows (':' is part of "C:\..."), ':' elsewhere
    return ([item for item in os.environ.get("PATH", "").split(os.pathsep) if item])

def find_dll(name: str, platform = None) -> str:
    # `platform` is kept for compatibility: os.path already handles both separators
    for directory in (*_frozen_dirs(), *_DEFAULT_DIRS, *_REGISTERED_DIRS, *_env_path_dirs()):
        candidate = abspath(join(directory, name))
        if (isfile(candidate)):
            return (candidate)
    return (None)


def add_dll_registry_path(path: str) -> None:
    if (path not in _REGISTERED_DIRS):
        _REGISTERED_DIRS.append(path)

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
        if (hasattr(self, "_dll")):
            del self._dll

        self._dll = None