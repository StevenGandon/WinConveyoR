from ctypes import CDLL, Structure, POINTER, c_int, c_double, c_size_t, c_ubyte
from sys import platform

class _SymbolRegister(object):
    def __init__(self, name: str, symbol_ptr, response_type, *arguments) -> None:
        self.name: str = name
        self.symbol_ptr = symbol_ptr
        self.response_type = response_type
        self.arguments = [*arguments]

        self.symbol_ptr.restype = response_type
        self.symbol_ptr.argtypes = tuple(arguments)

class _DLLoader(object):
    def __init__(self, path: str) -> None:
        self._path = path
        self._registered_functions = {}
        self._dll: CDLL = CDLL(path)

    def register_function(self, name: str, response_type, *arguments) -> None:
        if (not hasattr(self._dll, name)):
            raise ReferenceError(f"symbol {name} not found in dll {self._path}.")
        self._registered_functions[name] = _SymbolRegister(name, getattr(self._dll, name), response_type, *arguments)

    def call_function(self, name: str, *arguments):
        if (not self._registered_functions[name]):
            raise ReferenceError(f"symbol {name} not registered.")
        return (self._registered_functions[name].symbol_ptr(*arguments))

    def __del__(self) -> None:
        del self._dll

class _Mapper(object):
    def __init__(self, path: str = f"lib/libwconr/libwconr.{'dll' if platform.startswith("win") else 'so'}") -> None:
        self._dll: _DLLoader = _DLLoader(path)

        self.map_functions()

    def call_function(self, name: str,  *args):
        return self._dll.call_function(name, *args)

    def map_functions(self) -> None:
        pass