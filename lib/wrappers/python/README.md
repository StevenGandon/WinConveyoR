# libwconr (Python wrapper)

`ctypes` bindings for the native **libwconr** shared library.

> **This package contains only the Python wrapper.** The native library
> (`libwconr.so` / `libwconr.dylib` / `libwconr.dll`) must be installed
> separately and be discoverable (see below).

## Install

```bash
pip install libwconr
```

The distribution is named `libwconr`, but the import package is `libwconr`:

```python
from libwconr import libwconr
from libwconr import add_dll_registry_path

add_dll_registry_path("/opt/wconr/lib")   # optional: where the native lib lives
libwconr.init()

state = libwconr.WCRState()
print(state.list_installed())
state.close()
```

## Native library lookup

`libwconr.init()` looks for `libwconr.{so,dylib,dll}` in, in order:

1. `./` and `./lib/` relative to the current working directory
2. every directory in `$PATH`
3. any directory added with `libwconr.add_dll_registry_path(path)`

If nothing is found it falls back to the bare file name, so the OS loader
rules (e.g. `LD_LIBRARY_PATH`, `DYLD_LIBRARY_PATH`) apply.

## Development

```bash
pip install -e ".[test]"
pytest
```
