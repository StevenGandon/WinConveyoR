from uuid import uuid4

from .client import Client

class Session(object):
    def __init__(self, client = Client(None), *, session_id = None, session_instance = None):
        self._id = uuid4().int if session_id is None else session_id

        self.opened = True
        self.client = client
        self.session_instance = session_instance
        self.flags = 0x00

    def set_flags(self, flags):
        self.flags = (self.flags | flags)

    def has_flags(self, flags):
        return ((self.flags & flags) > 0)

    def get_id(self):
        return (self._id)

    def isopen(self):
        return (self.opened and self.client.isopen())

    def close(self):
        self.opened = False
        if (hasattr(self.session_instance, "close")):
            self.session_instance.close()
        self.session_instance = None

    def __del__(self):
        self.close()
