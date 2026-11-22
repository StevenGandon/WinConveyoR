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
            entry = {"url": src.url.decode('utf-8'), "proto": src.proto}
            if src.access_key:
                entry["access_key"] = src.access_key.decode('utf-8')
            if src.server_pubkey_path:
                entry["server_pubkey_path"] = src.server_pubkey_path.decode('utf-8')
            result.append(entry)
        return result

    def dowload_package(self, url, location) -> None:
        if int(self.__mapper.call_function("download_package", cast(create_string_buffer(url.encode('utf-8')), POINTER(c_ubyte)), cast(create_string_buffer(location.encode('utf-8')), POINTER(c_ubyte))) < 0):
            raise RuntimeError("failed to download")

    def source_set_auth(self, index: int, access_key: str = None, server_pubkey_path: str = None) -> int:
        ak = access_key.encode('utf-8') if access_key else None
        pk = server_pubkey_path.encode('utf-8') if server_pubkey_path else None
        return int(self.__mapper.call_function("wcr_source_set_auth", self._cstate, index, ak, pk))

    def sync_package_list(self, proto: int, source_uri: str) -> int:
        return int(self.__mapper.call_function("sync_package_list", self._cstate, proto, source_uri.encode('utf-8')))

    def install_package(self, proto: int, source_uri: str, package_name: str) -> int:
        return int(self.__mapper.call_function("install_package", self._cstate, proto, source_uri.encode('utf-8'), package_name.encode('utf-8')))

    def uninstall_package(self, package_name: str) -> int:
        return int(self.__mapper.call_function("uninstall_package", self._cstate, package_name.encode('utf-8')))

    def get_package_metadata(self, proto: int, source_uri: str, package_spec: str) -> dict:
        meta = wcr_pkg_metadata_s()
        rc = int(self.__mapper.call_function("get_package_metadata", self._cstate, proto,
                                             source_uri.encode('utf-8'), package_spec.encode('utf-8'),
                                             pointer(meta)))
        if (rc != 0):
            return None

        def _ds(v):
            return v.decode('utf-8', errors='replace') if v else ""

        deps = []
        for i in range(meta.depends_count):
            if (meta.depends[i]):
                deps.append(meta.depends[i].decode('utf-8', errors='replace'))

        result = {
            "name": _ds(meta.name),
            "version": _ds(meta.version),
            "arch": _ds(meta.arch),
            "machine": _ds(meta.machine),
            "description": _ds(meta.description),
            "address": _ds(meta.address),
            "SHA256": _ds(meta.sha256),
            "MD5sum": _ds(meta.md5),
            "depends": deps,
            "size": meta.size,
            "added_at": meta.added_at
        }

        self.__mapper.call_function("free_package_metadata", pointer(meta))
        return result

    def list_package_variants(self, proto: int, source_uri: str, package_name: str) -> list:
        out = POINTER(wcr_pkg_variant_s)()
        count = c_size_t(0)
        rc = int(self.__mapper.call_function("list_package_variants", self._cstate, proto,
                                             source_uri.encode('utf-8'), package_name.encode('utf-8'),
                                             pointer(out), pointer(count)))
        if (rc != 0):
            return []
        result = []
        for i in range(count.value):
            ver = out[i].version.decode('utf-8', errors='replace') if out[i].version else ""
            arch = out[i].arch.decode('utf-8', errors='replace') if out[i].arch else ""
            mach = out[i].machine.decode('utf-8', errors='replace') if out[i].machine else ""
            result.append({"version": ver, "arch": arch, "machine": mach})
        if (count.value > 0):
            self.__mapper.call_function("free_variant_list", out, count)
        return result

    def search_available(self, query: str = "") -> list:
        out = POINTER(wcr_available_pkg_s)()
        count = c_size_t(0)
        q = query.encode('utf-8') if query else None
        rc = int(self.__mapper.call_function("search_available", self._cstate, q, pointer(out), pointer(count)))
        if (rc != 0):
            return []
        result = []
        for i in range(count.value):
            name = out[i].name.decode('utf-8', errors='replace') if out[i].name else ""
            version = out[i].version.decode('utf-8', errors='replace') if out[i].version else ""
            result.append({"name": name, "version": version})
        if (count.value > 0):
            self.__mapper.call_function("free_available_list", out, count)
        return result

    def verify_cached_package(self, proto: int, source_uri: str, package_spec: str) -> dict:
        check = wcr_hash_check_s()
        rc = int(self.__mapper.call_function("verify_cached_package", self._cstate, proto,
                                             source_uri.encode('utf-8'), package_spec.encode('utf-8'),
                                             pointer(check)))
        if (rc != 0):
            return None

        def _ds(v):
            return v.decode('utf-8', errors='replace') if v else ""

        result = {
            "expected": _ds(check.expected),
            "actual": _ds(check.actual),
            "match": check.match
        }

        self.__mapper.call_function("free_hash_check", pointer(check))
        return result

    def list_installed(self) -> list:
        out = POINTER(wcr_installed_pkg_s)()
        count = c_size_t(0)
        rc = int(self.__mapper.call_function("list_installed", self._cstate, pointer(out), pointer(count)))
        if (rc != 0):
            return []
        result = []
        for i in range(count.value):
            name = out[i].name.decode('utf-8', errors='replace') if out[i].name else ""
            version = out[i].version.decode('utf-8', errors='replace') if out[i].version else ""
            result.append({"name": name, "version": version, "is_dependency": bool(out[i].is_dependency)})
        if (count.value > 0):
            self.__mapper.call_function("free_installed_list", out, count)
        return result

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
