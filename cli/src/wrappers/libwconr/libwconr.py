from ._libwconr import *

from ctypes import c_char_p, pointer, cast, POINTER, c_ubyte, create_string_buffer
import os

_MAPPER = Mapper()

def init():
    _MAPPER.init_mapper()

class WCRState(object):
    __mapper: Mapper = _MAPPER

    def __init__(self, _raw_ptr=None) -> None:
        if _raw_ptr is not None:
            self._cstate = _raw_ptr
        else:
            self._cstate = self.__mapper.call_function("new_state")

    @classmethod
    def load(cls, filepath: str):
        path = os.path.expanduser(filepath)
        ptr = cls.__mapper.call_function("load_state", path.encode('utf-8'))
        if not ptr:
            return None
        return cls(_raw_ptr=ptr)

    def save(self, filepath: str) -> int:
        path = os.path.expanduser(filepath)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        return int(self.__mapper.call_function("write_state", self._cstate, path.encode('utf-8')))

    def add_source(self, proto: int, url: str) -> int:
        return int(self.__mapper.call_function("wcr_state_add_source", self._cstate, proto, url.encode('utf-8')))

    def get_sources(self) -> list:
        state = self._cstate.contents
        result = []
        for i in range(state.sources_count):
            src = state.sources[i].contents
            result.append({"url": src.url.decode('utf-8'), "proto": src.proto})
        return result

    def dowload_package(self, url, location) -> None:
        if int(self.__mapper.call_function("download_package", cast(create_string_buffer(url.encode('utf-8')), POINTER(c_ubyte)), cast(create_string_buffer(location.encode('utf-8')), POINTER(c_ubyte))) < 0):
            raise RuntimeError("failed to download")

    def sync_package_list(self, source_uri) -> int:
        return int(self.__mapper.call_function("sync_package_list", self._cstate, source_uri.encode('utf-8')))

    def install_package(self, source_uri, package_name) -> int:
        return int(self.__mapper.call_function("install_package", self._cstate, source_uri.encode('utf-8'), package_name.encode('utf-8')))

    def close(self):
        if (not self._cstate):
            return
        self.__mapper.call_function("close_state", self._cstate)
        self._cstate = None

    def __del__(self):
        self.close()
