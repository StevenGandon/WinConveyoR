from uuid import uuid4

from .client import Client

class Session(object):
    def __init__(self, client = Client(None)):
        self._id = uuid4().int

        self.opened = True
        self.client = client

    def get_id(self):
        return (self._id)

    def isopen(self):
        return (self.opened and self.client.isopen())

    def close(self):
        self.opened = False

    def __del__(self):
        self.close()
