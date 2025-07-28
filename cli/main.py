from sys import exit
from itertools import chain
from signal import SIGINT, SIGTERM, signal
from traceback import format_exc
from time import sleep
from pathlib import Path
from locale import getpreferredencoding

from src import *

import sys

class CLI(object):
    OPTION_TABLE = {
        "help": {"opt": ("-h", "--help", "-?", "/?", "/h"), "exc": ()},
        "download": {"opt": ("-d", "--download", "-dwnld"), "exc": ()},
        "nocolor": {"opt": ("--no-color", "-ncolor")},
        "noansi": {"opt": ("--no-ansi", "-nansi")},
        "ascii": {"opt": ("--ascii", "-ascii")}
    }

    def __init__(self):
        self.argsettings = ArgumentParserSettings()
        self.argparser = None
        self.wcr = None
        self.running: bool = True

        self.patterns = P = PatternLoader("assets/cli/graphic/patterns/pattern_loading.xml")

        self._set_argument_setting()
        self._parse_arguments()

        signal(SIGINT, lambda *args, **kwargs: self.close())
        signal(SIGTERM, lambda *args, **kwargs: self.close())

        if (hasattr(sys.stdout, 'isatty') and not sys.stdout.isatty()):
            P.deploy("noansi", "ascii", _globals=globals(), _locals=locals())
            self._graphic = Graphic(GraphicSettings(False, False, mode=MODE_DISPLAY_NO_ANIMATION))
        else:
            ansi_settings = "16color"
            charset_settings = "utf8" if getpreferredencoding() == "UTF-8" else "ascii"

            if (self.has_arg("nocolor")):
                ansi_settings = "colorless"
            if (self.has_arg("noansi")):
                ansi_settings = "noansi"
            if (self.has_arg("ascii")):
                charset_settings = "ascii"

            P.deploy(ansi_settings, charset_settings, _globals=globals(), _locals=locals())
            self._graphic = Graphic(GraphicSettings(ansi_settings != "noansi", ansi_settings not in ("colorless", "noansi"), mode=MODE_DISPLAY_SIMPLE if ansi_settings != "noansi" else MODE_DISPLAY_NO_ANIMATION))

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

    def has_arg(self, arg: str):
        if (arg not in CLI.OPTION_TABLE):
            return (False)
        return (any(map(lambda x: x in self.argparser.options, CLI.OPTION_TABLE[arg]["opt"])))

    def show_help(self):
        sys.stdout.write(f"""Usage: {sys.argv[0]} <-d|-h|...> [options] [arguments]

Options:
  > {', '.join(CLI.OPTION_TABLE['help']['opt'])}\tDisplay this help message
  > {', '.join(CLI.OPTION_TABLE['download']['opt'])}\tDownload a package
  > {', '.join(CLI.OPTION_TABLE['nocolor']['opt'])}\t\tDisable color rendering
  > {', '.join(CLI.OPTION_TABLE['noansi']['opt'])}\t\tDisable ansi rendering
  > {', '.join(CLI.OPTION_TABLE['ascii']['opt'])}\t\tRendering only in ascii

Exemples:
""")
        return (0)

    def download_package(self):
        lb = LoadingBar(100, 0, 1)
        lb1 = LoadingBar(10, 0, 1)
        lb2 = LoadingBar(1000, 0, 1)
        lb3 = LoadingBar(75, 0, 1)
        lb4 = LoadingBar(50, 0, 1)
        self._graphic.add_elements(lb)
        self._graphic.add_elements(lb1)
        self._graphic.add_elements(lb2)
        self._graphic.add_elements(lb3)
        self._graphic.add_elements(lb4)
        for i in range(1, 100):
            if (not self.running):
                return (1)
            lb.push(f"item_{i}")
            lb1.push(f"item_{i}")
            lb2.push(f"item_{i}")
            lb3.push(f"item_{i}")
            lb4.push(f"item_{i}")
            self._graphic.update()
            self._graphic.draw()
            sleep(0.1)
        #self.wcr.dowload_package("https://developer.mozilla.org/fr/docs/Web/HTTP/Reference/Status/301", "./")
        return (0)

    def run(self) -> int:
        if (not self.wcr):
            self.wcr = WCRState()

        if (self.has_arg("help")):
            return self.show_help()

        if (self.has_arg("download")):
            return self.download_package()

        sys.stderr.write(f"{sys.argv[0]}: no operation specified (use -h for help).\n")
        return (1)

    def close(self):
        self.running: bool = False
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

    try:
        status = cli.run()
    except Exception as e:
        sys.stderr.write(f"{sys.argv[0]}: Unhandled exception: {format_exc()}\n")
        status = 1
    cli.close()
    return (status)

if (__name__ == "__main__"):
    exit(main())
