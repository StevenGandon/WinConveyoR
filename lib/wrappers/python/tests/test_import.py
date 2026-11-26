"""Smoke tests: the wrapper must import and expose its API without the
native library being present (the DLL is only loaded on `init()`)."""

import pytest


def test_import_package():
    import libwconr

    assert hasattr(libwconr, "add_dll_registry_path")
    assert hasattr(libwconr, "find_dll")


def test_public_api():
    from libwconr import libwconr

    assert hasattr(libwconr, "WCRState")
    assert hasattr(libwconr, "init")
    assert libwconr.wcr_event_type.WCR_EVENT_ERROR.value == 3


def test_find_dll_missing_returns_none():
    from libwconr import find_dll

    assert find_dll("definitely_not_a_real_library.so") is None


def test_add_dll_registry_path(tmp_path):
    from libwconr import add_dll_registry_path, find_dll

    (tmp_path / "libfake.so").write_bytes(b"")
    add_dll_registry_path(str(tmp_path))
    assert find_dll("libfake.so") is not None


def test_init_without_native_lib_raises():
    from libwconr import libwconr

    with pytest.raises(OSError):
        libwconr.init()


def test_find_dll_windows_path_keeps_drive(monkeypatch):
    """Windows $PATH uses ';' and drive letters: the drive must survive
    (regression: splitting on ':' gave '/Users/x/bin/libwconr.dll')."""
    import ntpath
    from libwconr import dllloader

    wanted = r"C:\Users\ethan\AppData\Local\Programs\WinConveyoR\bin\libwconr.dll"
    monkeypatch.setattr(dllloader.os, "pathsep", ";")
    monkeypatch.setenv("PATH", r"C:\Windows\system32;C:\Users\ethan\AppData\Local\Programs\WinConveyoR\bin")
    monkeypatch.setattr(dllloader, "join", ntpath.join)
    monkeypatch.setattr(dllloader, "abspath", lambda p: p)
    monkeypatch.setattr(dllloader, "isfile", lambda p: p == wanted)

    assert dllloader.find_dll("libwconr.dll", "nt") == wanted


def test_find_dll_returns_absolute_path(tmp_path, monkeypatch):
    from libwconr import find_dll

    (tmp_path / "libfake2.so").write_bytes(b"")
    monkeypatch.chdir(tmp_path)
    found = find_dll("libfake2.so")
    assert found is not None and found.startswith("/") and found.endswith("libfake2.so")
