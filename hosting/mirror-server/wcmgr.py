#!/usr/bin/python3

from sys import exit
from string import hexdigits

from src import *
from src.cli.commands import *
from src.cli.middlewares import *

def main():
    try:
        nc = NetworkCLI()
    except ConnectionError as e:
        print(f"failed to connect to server. ({e})")
        return (1)

    nc.add_command(CLICommand(
        "connect",
        connect_command,
        [
            CLICommandArg("password", CLICommandArg.ARG_MANDATORY)
        ]
    ))

    nc.add_command(CLICommand(
        "disconnect",
        disconnect_command,
        [
            CLICommandArg("session_id", CLICommandArg.ARG_MANDATORY, argument_parser=int, argument_checker=lambda x: x.isnumeric())
        ]
    ), True)

    nc.add_command(CLICommand(
        "list_sessions",
        list_sessions_command,
        []
    ))

    nc.add_command(CLICommand(
        "switch_session",
        switch_session_command,
        [
            CLICommandArg("session_id", CLICommandArg.ARG_OPTIONAL, argument_parser=lambda x: int(x, 16), argument_checker=lambda x: all(c in hexdigits for c in x))
        ]
    ))

    nc.add_command(CLICommand(
        "list_packages",
        list_packages_command,
        [
            CLICommandArg("session_id", CLICommandArg.ARG_MANDATORY, argument_parser=int, argument_checker=lambda x: x.isnumeric())
        ]
    ), True)

    nc.add_command(CLICommand(
        "write",
        write_command,
        [
            CLICommandArg("session_id", CLICommandArg.ARG_MANDATORY, argument_parser=int, argument_checker=lambda x: x.isnumeric())
        ]
    ), True)

    nc.add_command(CLICommand(
        "quit",
        quit_command,
        []
    ))

    nc.run()
    nc.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())