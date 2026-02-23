#!/usr/bin/python3

from sys import exit, argv
from string import hexdigits

from src import *
from src.cli.commands import *
from src.cli.middlewares import *
from src.arghandler import *

import sys

def main():
    argsettings = ArgumentParserSettings(2, 2)

    argsettings.define_argument(str, "program")
    argsettings.define_argument(str, "host")

    argsettings.define_parameter("--port", int)
    argsettings.define_parameter("--key", str)

    argsettings.define_option("--secure")

    argsettings.define_option("-h", is_help=True)
    argsettings.define_option("--help", is_help=True)
    argsettings.define_option("-?", is_help=True)

    argsettings.validate()


    try:
        argparser = ArgumentParser(sys.argv, argsettings)
    except ArgumentHandlerException as e:
        sys.stderr.write(f"{sys.argv[0]}: {e}\n")
        return (1)

    if ("-h" in argparser.options or "--help" in argparser.options or "-?" in argparser.options):
        help_message = f"Usage: $prgm_name [options] $args\nOptions:\n$options"
        args = ''.join(str(item.name) if i < argsettings.min_argv - 1 else f"({item.name})" for i, item in enumerate(argsettings.arguments[1:]))
        options = '  ' + '\n  '.join(item.name for item in (list(argsettings.parameters.values()) + list(argsettings.options.values())))

        print(help_message.replace("$prgm_name", str(argparser.arguments[0].value)).replace("$args", args).replace("$options", options))
        return (0)

    if ("--key" in argparser.parameters):
        Message.PUBLIC_KEY = PublicSecurityKey.from_file(argparser.parameters["--key"].value)
    else:
        print("warning: no client side encryption, use --key to provide a server public key.")

    if ("--secure" in argparser.options):
        private_key = PrivateSecurityKey.generate()
    else:
        print("warning: no server message encryption, use --secure to enable it.")
        private_key = None

    try:
        nc = NetworkCLI(argparser.arguments[1].value, 1674 if "--port" not in argparser.parameters else argparser.parameters["--port"].value)
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
        "user",
        user_command,
        [
            CLICommandArg("password", CLICommandArg.ARG_MANDATORY)
        ]
    ))

    nc.add_command(CLICommand(
        "get_hash",
        gh_command,
        [
            CLICommandArg("session_id", CLICommandArg.ARG_MANDATORY, argument_parser=int, argument_checker=lambda x: x.isnumeric())
        ]
    ), True)

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

    try:
        nc.run(private_key)
    except Exception as e:
        print(e)
        return (1)
    nc.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())