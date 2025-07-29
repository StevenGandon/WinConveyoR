from src import *

from traceback import format_exc
from sys import exit

import sys

def main() -> int:
    add_dll_registry_path("./lib/libwconr/")

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
