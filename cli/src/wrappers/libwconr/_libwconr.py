from ctypes import CDLL, Structure, POINTER, c_int, c_double, c_size_t, c_ubyte
from sys import platform

from ..dllloader import DLLoader

class Mapper(object):
    def __init__(self, path: str = f"lib/libwconr/libwconr.{'dll' if platform.startswith("win") else 'so'}") -> None:
        self._dll: DLLoader = DLLoader(path)

        self.map_functions()

    def call_function(self, name: str,  *args):
        return self._dll.call_function(name, *args)

    def map_functions(self) -> None:
        pass
