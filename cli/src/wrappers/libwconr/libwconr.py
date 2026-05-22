from ._libwconr import *

from ctypes import c_char_p, pointer, cast, POINTER, c_ubyte, c_void_p, c_int, c_size_t, create_string_buffer, CFUNCTYPE, Structure
from enum import Enum
import os

class wcr_event_type(Enum):
    WCR_EVENT_DEBUG = 0
    WCR_EVENT_INFO = 1
    WCR_EVENT_WARNING = 2
    WCR_EVENT_ERROR = 3
    WCR_EVENT_PROGRESS = 4

class wcr_event(Structure):
    _fields_ = [
        ("type", c_int),
        ("message", c_char_p),
        ("bytes_done", c_size_t),
        ("bytes_total", c_size_t)
    ]

wcr_event_callback_t = CFUNCTYPE(None, POINTER(wcr_event), c_void_p)

_MAPPER = Mapper()

def init():
    _MAPPER.init_mapper()

class WCRState(object):
    __mapper: Mapper = _MAPPER

    def __init__(self, _raw_ptr=None) -> None:
        self._event_cb_ref = None
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

    def sync_package_list(self, proto: int, source_uri: str) -> int:
        return int(self.__mapper.call_function("sync_package_list", self._cstate, proto, source_uri.encode('utf-8')))

    def install_package(self, proto: int, source_uri: str, package_name: str) -> int:
        return int(self.__mapper.call_function("install_package", self._cstate, proto, source_uri.encode('utf-8'), package_name.encode('utf-8')))

    def set_event_callback(self, callback):
        def _c_callback(event_ptr, user_data):
            ev = event_ptr.contents
            msg = ev.message.decode('utf-8', errors='replace') if ev.message else ""
            callback(wcr_event_type(ev.type), msg, ev.bytes_done, ev.bytes_total)

        self._event_cb_ref = wcr_event_callback_t(_c_callback)
        self.__mapper.call_function("wcr_set_event_callback", self._cstate, self._event_cb_ref, None)

    def close(self):
        if (not self._cstate):
            return
        self.__mapper.call_function("close_state", self._cstate)
        self._cstate = None
        self._event_cb_ref = None

    def __del__(self):
        self.close()
