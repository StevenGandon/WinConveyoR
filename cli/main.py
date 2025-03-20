from sys import exit
from itertools import chain
from signal import SIGINT, SIGTERM, signal

from src import *

import sys

class CLI(object):
    OPTION_TABLE = {
        "help": {"opt": ("-h", "--help", "-?", "/?", "/h"), "exc": ()},
        "download": {"opt": ("-d", "--download", "-dwnld"), "exc": ()}
    }
    def __init__(self):
        self.argsettings = ArgumentParserSettings()
        self.argparser = None
        self.wcr = None

        self._set_argument_setting()
        self._parse_arguments()

        signal(SIGINT, lambda *args, **kwargs: self.close)
        signal(SIGTERM, lambda *args, **kwargs: self.close)

    def _parse_arguments(self) -> int:
        try:
            self.argparser = ArgumentParser(sys.argv, self.argsettings)
        except ArgumentHandlerException as e:
            sys.stderr.write(f"{sys.argv[0]}: {e}\n")
            raise ArgumentHandlerException()

    def _set_argument_setting(self):
        self.argsettings = ArgumentParserSettings()

        for item in chain(*tuple(map(lambda x: x["opt"], CLI.OPTION_TABLE.values()))):
            self.argsettings.define_option(item)

        self.argsettings.validate()

    def show_help(self):
        sys.stdout.write(f"""Usage: {sys.argv[0]} <-d|-h> [options] [arguments]

Options:
  > {', '.join(CLI.OPTION_TABLE['help']['opt'])}\tDisplay this help message
  > {', '.join(CLI.OPTION_TABLE['download']['opt'])}\tDownload a package

Exemples:
""")
        return (0)

    def download_package(self):
        self.wcr.dowload_package("https://developer.mozilla.org/fr/docs/Web/HTTP/Reference/Status/301", "./")
        return (0)

    def run(self) -> int:
        if (not self.wcr):
            self.wcr = WCRState()

        if (any(map(lambda x: x in self.argparser.options, CLI.OPTION_TABLE["help"]["opt"]))):
            return self.show_help()

        if (any(map(lambda x: x in self.argparser.options, CLI.OPTION_TABLE["download"]["opt"]))):
            return self.download_package()

        sys.stderr.write(f"{sys.argv[0]}: no operation specified (use -h for help).\n")
        return (1)

    def close(self):
        if (self.wcr):
            self.wcr.close()

    def __del__(self):
        self.close()

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

    # try:
    status = cli.run()
    # except Exception as e:
    #     sys.stderr.write(f"{sys.argv[0]}: Unhandled exception: {e}\n")
    #     status = 1
    cli.close()
    return (status)

if (__name__ == "__main__"):
    exit(main())
