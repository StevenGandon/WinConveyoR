from ._libwconr import *

_MAPPER = Mapper()

class WCRState(object):
    __mapper: Mapper = _MAPPER

    def __init__(self) -> None:
        self._cstate = self.__mapper.call_function("new_state")

    def close(self):
        if (not self._cstate):
            return
        self.__mapper.call_function("close_state", self._cstate)
        self._cstate = None

    def __del__(self):
        self.close()
