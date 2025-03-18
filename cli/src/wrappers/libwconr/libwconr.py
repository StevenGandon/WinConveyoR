from ._libwconr import *

from ctypes import c_char_p, pointer, cast, POINTER, c_ubyte, create_string_buffer

_MAPPER = Mapper()

class WCRState(object):
    __mapper: Mapper = _MAPPER

    def __init__(self) -> None:
        self._cstate = self.__mapper.call_function("new_state")


    def dowload_package(self, url, location) -> None:
        if int(self.__mapper.call_function("download_package", cast(create_string_buffer(url.encode('utf-8')), POINTER(c_ubyte)), cast(create_string_buffer(location.encode('utf-8')), POINTER(c_ubyte))) < 0):
            raise RuntimeError("failed to downlaod")

    def close(self):
        if (not self._cstate):
            return
        self.__mapper.call_function("close_state", self._cstate)
        self._cstate = None

    def __del__(self):
        self.close()
