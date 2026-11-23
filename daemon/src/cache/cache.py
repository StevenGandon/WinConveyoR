import copy
import io
import os
import struct
from typing import Any, Dict, Hashable, Optional

MAGIC = b"\x00\xca\xc5\xe0"
LABEL = b"cache-cache"
_NONE, _BOOL, _INT, _FLOAT, _STR, _BYTES, _LIST, _DICT = range(8)

def _read_exact(fp, n: int) -> bytes:
    data = fp.read(n)
    if len(data) != n:
        raise RuntimeError("invalid file format: unexpected end of data.")
    return data


def _read_u32(fp) -> int:
    return int.from_bytes(_read_exact(fp, 4), byteorder="big")


def _encode(value: Any, out: bytearray) -> None:
    """Append a self-describing encoding of `value` to `out`."""
    if value is None:
        out.append(_NONE)
    elif isinstance(value, bool):
        out.append(_BOOL)
        out.append(1 if value else 0)
    elif isinstance(value, int):
        size = (value.bit_length() + 8) // 8
        out.append(_INT)
        out.extend(size.to_bytes(2, byteorder="big"))
        out.extend(value.to_bytes(size, byteorder="big", signed=True))
    elif isinstance(value, float):
        out.append(_FLOAT)
        out.extend(struct.pack(">d", value))
    elif isinstance(value, str):
        raw = value.encode("utf-8")
        out.append(_STR)
        out.extend(len(raw).to_bytes(4, byteorder="big"))
        out.extend(raw)
    elif isinstance(value, (bytes, bytearray)):
        out.append(_BYTES)
        out.extend(len(value).to_bytes(4, byteorder="big"))
        out.extend(value)
    elif isinstance(value, (list, tuple)):
        out.append(_LIST)
        out.extend(len(value).to_bytes(4, byteorder="big"))
        for item in value:
            _encode(item, out)
    elif isinstance(value, dict):
        out.append(_DICT)
        out.extend(len(value).to_bytes(4, byteorder="big"))
        for k, v in value.items():
            _encode(k, out)
            _encode(v, out)
    else:
        raise TypeError(f"cannot cache value of type {type(value).__name__}.")


def _decode(fp) -> Any:
    tag = _read_exact(fp, 1)[0]
    if tag == _NONE:
        return None
    if tag == _BOOL:
        return _read_exact(fp, 1)[0] != 0
    if tag == _INT:
        size = int.from_bytes(_read_exact(fp, 2), byteorder="big")
        return int.from_bytes(_read_exact(fp, size), byteorder="big", signed=True)
    if tag == _FLOAT:
        return struct.unpack(">d", _read_exact(fp, 8))[0]
    if tag in (_STR, _BYTES):
        raw = _read_exact(fp, _read_u32(fp))
        return raw.decode("utf-8") if tag == _STR else raw
    if tag == _LIST:
        return [_decode(fp) for _ in range(_read_u32(fp))]
    if tag == _DICT:
        result = {}
        for _ in range(_read_u32(fp)):
            k = _decode(fp)
            result[k] = _decode(fp)
        return result
    raise RuntimeError(f"invalid file format: unknown value tag {tag}.")


def _type_name(obj_type: type) -> str:
    return f"{obj_type.__module__}.{obj_type.__qualname__}"

class Cache(object):
    """A snapshot of an object's cached field values (field name -> value)."""

    def __init__(self, values: Optional[Dict[str, Any]] = None):
        self.values: Dict[str, Any] = dict(values) if values else {}

    def copy(self) -> "Cache":
        return Cache(copy.deepcopy(self.values))

    def __repr__(self) -> str:
        return f"Cache({self.values!r})"


class CacheSchema(object):
    """
    Describes how instances of a type are cached.

    key:    name of the attribute that uniquely identifies an instance
            (must be a str, int, float, bool or bytes).
    fields: attribute name -> expected type, for every attribute to cache.
    """

    _KEY_TYPES = (str, int, float, bool, bytes)

    def __init__(self, key: str, fields: Dict[str, type]):
        if key in fields:
            raise ValueError("the key attribute must not also be a cached field.")
        self.key = key
        self.fields = dict(fields)

    def key_of(self, obj: object) -> Hashable:
        key = getattr(obj, self.key)
        if not isinstance(key, self._KEY_TYPES):
            raise TypeError(
                f"key '{self.key}' must be str, int, float, bool or bytes, "
                f"got {type(key).__name__}."
            )
        return key

    def extract(self, obj: object) -> Cache:
        """Read the cached fields from `obj` into a new Cache."""
        values = {}
        for name, ftype in self.fields.items():
            value = getattr(obj, name)
            if value is not None and not isinstance(value, ftype):
                if ftype is float and isinstance(value, int):
                    value = float(value)
                else:
                    raise TypeError(
                        f"field '{name}' expected {ftype.__name__}, "
                        f"got {type(value).__name__}."
                    )
            values[name] = value
        return Cache(copy.deepcopy(values))

    def inject(self, obj: object, cache: Cache) -> None:
        """Write the values held by `cache` onto `obj`."""
        for name in self.fields:
            if name in cache.values:
                setattr(obj, name, copy.deepcopy(cache.values[name]))


class CacheService(object):
    def __init__(self):
        self.schemas: Dict[type, tuple] = {}
        self.data: Dict[type, Dict[Hashable, Cache]] = {}

    def register(self, obj_type: type, obj_schema: CacheSchema, default_replication: Cache):
        self.schemas[obj_type] = (obj_schema, default_replication)

    def apply(self, obj: object) -> bool:
        """
        Restore cached state onto `obj`. Falls back to the default replication
        if nothing has been stored for this object yet.
        Returns True if stored data was applied, False otherwise.
        """
        if type(obj) not in self.schemas:
            return False

        schema, default = self.schemas[type(obj)]
        stored = self.data.get(type(obj), {}).get(schema.key_of(obj))

        schema.inject(obj, stored if stored is not None else default)
        return stored is not None

    def store(self, obj: object):
        """Snapshot the current state of `obj` into the cache."""
        if type(obj) not in self.schemas:
            return

        schema, _ = self.schemas[type(obj)]
        self.data.setdefault(type(obj), {})[schema.key_of(obj)] = schema.extract(obj)

    def load(self, file: str) -> bool:
        """
        Merge entries from `file` into the cache (loaded entries overwrite
        existing ones). Entries of types that are not registered are skipped.
        Returns False if the file doesn't exist.
        """
        if not os.path.isfile(file):
            return False

        types_by_name = {_type_name(t): t for t in self.schemas}
        loaded = []

        with open(file, "rb") as fp:
            if _read_exact(fp, 4) != MAGIC:
                raise RuntimeError("invalid file format.")
            if _read_exact(fp, len(LABEL)) != LABEL:
                raise RuntimeError("invalid file format.")

            count: int = int.from_bytes(_read_exact(fp, 8), byteorder="big")

            for _ in range(count):
                entry = io.BytesIO(_read_exact(fp, _read_u32(fp)))
                try:
                    name = _decode(entry)
                    key = _decode(entry)
                    values = _decode(entry)
                except (struct.error, UnicodeDecodeError, ValueError) as e:
                    raise RuntimeError("invalid file format.") from e

                if not isinstance(name, str) or not isinstance(values, dict):
                    raise RuntimeError("invalid file format.")

                obj_type = types_by_name.get(name)
                if obj_type is None:
                    continue

                if not isinstance(key, CacheSchema._KEY_TYPES):
                    raise RuntimeError("invalid file format.")

                loaded.append((obj_type, key, Cache(values)))

        for obj_type, key, cache in loaded:
            self.data.setdefault(obj_type, {})[key] = cache

        return True

    def save(self, file: str):
        arr = bytearray()

        arr.extend(MAGIC)
        arr.extend(LABEL)
        arr.extend(sum(len(entries) for entries in self.data.values()).to_bytes(8, byteorder="big"))

        for obj_type, entries in self.data.items():
            name = _type_name(obj_type)
            for key, cache in entries.items():
                body = bytearray()
                _encode(name, body)
                _encode(key, body)
                _encode(cache.values, body)

                arr.extend(len(body).to_bytes(4, byteorder="big"))
                arr.extend(body)

        tmp = file + ".tmp"
        with open(tmp, "wb") as fp:
            fp.write(bytes(arr))
        os.replace(tmp, file)


