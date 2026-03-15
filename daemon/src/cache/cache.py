from genericpath import isfile

class Cache(object):
    def __init__(self):
        pass

class CacheSchema(object):
    def __init__(self):
        pass

class CacheService(object):
    def __init__(self):
        self.schemas = {}
        self.data = {}

    def register(self, obj_type: type, obj_schema: CacheSchema, default_replication: Cache):
        self.schemas[obj_type] = (obj_schema, default_replication)

    def apply(self, obj: object):
        if (type(obj) not in self.schemas):
            return

    def store(self, obj: object):
        if (type(obj) not in self.schemas):
            return

    def load(self, file: str):
        if (not isfile(file)):
            return
        with open(file, 'rb+') as fp:
            if (int.from_bytes(fp.read(4), byteorder="big") != 0x00CAC5E0):
                raise RuntimeError("invalid file format.")
            fp.seek(fp.tell() + 11)

            count: int = int.from_bytes(fp.read(8), byteorder="big")

    def save(self, file: str):
        arr = bytearray()

        arr.extend((0x00, 0xCA, 0xC5, 0xE0))
        arr.extend(b"cache-cache")
        arr.extend(len(self.data).to_bytes(8, byteorder="big"))

        with open(file, 'wb+') as fp:
            fp.write(bytes(arr))
