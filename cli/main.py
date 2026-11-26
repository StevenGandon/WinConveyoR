#!/usr/bin/env python3
from src import *
from libwconr import *
from src.packaging import get_resource, is_frozen

from traceback import format_exc
from sys import exit

import os
import sys

WIZARD_ASSETS = "assets/wizard"
ASSETS_ENV = "WCR_ASSETS_PATH"

def main() -> int:
    add_dll_registry_path("./lib/libwconr/")
    add_dll_registry_path("./lib/libwconr/build/")

    # the lib defaults to the repository assets (build machine path): in the
    # executable, point it to the bundled copy (wizard VM instruction sets)
    if (is_frozen()):
        os.environ.setdefault(ASSETS_ENV, get_resource(WIZARD_ASSETS))

    status = 0

    try:
        init()
    except OSError as e:
        sys.stderr.write(f"{sys.argv[0]}: {e}\n")
        return (1)

    try:
        cli: CLI = CLI()
    except ArgumentHandlerException as e:
        sys.stderr.write(f"{sys.argv[0]}: {e}\n")
        return (1)

    try:
        status = cli.run()
    except Exception as e:
        sys.stderr.write(f"{sys.argv[0]}: Unhandled exception: {format_exc()}\n")
        status = 1
    cli.close()
    return (status)

if (__name__ == "__main__"):
    exit(main())
