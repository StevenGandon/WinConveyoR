from locale import getpreferredencoding
from os import environ
from itertools import chain
from signal import SIGINT, SIGTERM, signal
from time import sleep

from ..arghandler import *
from ..wrappers import *
from ..graphic import *
from ..packaging import resource_path

class CLI(object):
    OPTION_TABLE: dict = {
        "help": {"opt": ("-h", "--help", "-?", "/?", "/h"), "exc": ()},
        "download": {"opt": ("-d", "--download", "-dwnld"), "exc": ()},
        "nocolor": {"opt": ("--no-color", "-ncolor")},
        "noansi": {"opt": ("--no-ansi", "-nansi")},
        "ascii": {"opt": ("--ascii", "-ascii")}
    }

    PARAMETER_TABLE: dict = {
        "terminal-support": {"opt": ("-ts", "--terminal-support")},
        "charset": {"opt": ("-chst", "--charset")}
    }

    def __init__(self):
        self.argsettings = ArgumentParserSettings()
        self.argparser = None
        self.wcr = None
        self.running: bool = True

        self.patterns = [
            PatternLoader(resource_path("assets/cli/graphic/patterns/pattern_loading.xml"))
        ]

        self._set_argument_setting()
        self._parse_arguments()
        self._build_graphic_ui()

        signal(SIGINT, lambda *args, **kwargs: self.close())
        signal(SIGTERM, lambda *args, **kwargs: self.close())

    def _build_graphic_ui(self):
        if (hasattr(sys.stdout, 'isatty') and not sys.stdout.isatty()):
            self._deploy_patterns("noansi", "ascii", _globals=globals(), _locals=locals())
            self._graphic = Graphic(GraphicSettings(False, False, mode=MODE_DISPLAY_NO_ANIMATION))
        else:
            ansi_settings = self._determine_term_support()
            charset_settings = self._determine_charset()

            self._deploy_patterns(ansi_settings, charset_settings, _globals=globals(), _locals=locals())
            self._graphic = Graphic(GraphicSettings(
                ansi_settings != "noansi",
                ansi_settings not in ("colorless", "noansi"),
                mode=MODE_DISPLAY_SIMPLE if ansi_settings != "noansi" else MODE_DISPLAY_NO_ANIMATION
            ))

    def _determine_charset(self) -> str:
        charset_settings = "ascii"
        prefered = getpreferredencoding()

        if (prefered == "UTF-8"):
            charset_settings = "utf8"

        user_defined = self.get_arg("charset")

        if (user_defined):
            if (user_defined.value not in CHARSET_PRIORITY):
                raise ArgumentHandlerException(f"Charset '{user_defined.value}' not supported.")
            charset_settings = user_defined.value

        if (self.has_opt("ascii")):
            charset_settings = "ascii"

        return (charset_settings)

    def _determine_term_support(self) -> str:
        ansi_settings = "colorless"

        if ("TERM" in environ):
            if (environ["TERM"] in ("dumb", "xterm-old")):
                ansi_settings = "noansi"
            if ("16color" in environ["TERM"] or environ["TERM"] in ("rxvt", "konsole", "xterm", "xterm-new")):
                ansi_settings = "16color"
            if ("88color" in environ["TERM"]):
                ansi_settings = "88color"
            if ("256color" in environ["TERM"]):
                ansi_settings = "256color"

        user_defined = self.get_arg("terminal-support")

        if (user_defined):
            if (user_defined.value not in STANDARD_PRIORITY):
                raise ArgumentHandlerException(f"Terminal standard '{user_defined.value}' not supported.")
            ansi_settings = user_defined.value

        if (self.has_opt("nocolor")):
            ansi_settings = "colorless"
        if (self.has_opt("noansi")):
            ansi_settings = "noansi"

        return (ansi_settings)

    def _deploy_patterns(self, standard, charset, _globals, _locals) -> None:
        for item in self.patterns:
            item.deploy(standard, charset, _globals=_globals, _locals=_locals)

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

        for item in chain(*tuple(map(lambda x: x["opt"], CLI.PARAMETER_TABLE.values()))):
            self.argsettings.define_parameter(item)

        self.argsettings.validate()

    def has_opt(self, arg: str):
        if (arg not in CLI.OPTION_TABLE):
            return (False)
        return (any(map(lambda x: x in self.argparser.options, CLI.OPTION_TABLE[arg]["opt"])))

    def get_arg(self, arg: str):
        if (arg not in CLI.PARAMETER_TABLE):
            return (None)
        if (not any(map(lambda x: x in self.argparser.parameters, CLI.PARAMETER_TABLE[arg]["opt"]))):
            return (None)
        return self.argparser.parameters[tuple(filter(lambda x: x in self.argparser.parameters, CLI.PARAMETER_TABLE[arg]["opt"]))[-1]]

    def show_help(self):
        sys.stdout.write(f"""Usage: {sys.argv[0]} <-d|-h|...> [options] [arguments]

Options:
  > {', '.join(CLI.OPTION_TABLE['help']['opt'])}\tDisplay this help message
  > {', '.join(CLI.OPTION_TABLE['download']['opt'])}\tDownload a package
  > {', '.join(CLI.OPTION_TABLE['nocolor']['opt'])}\t\tDisable color rendering
  > {', '.join(CLI.OPTION_TABLE['noansi']['opt'])}\t\tDisable ansi rendering
  > {', '.join(CLI.OPTION_TABLE['ascii']['opt'])}\t\tRendering only in ascii

Parameters
  > {', '.join(CLI.PARAMETER_TABLE['terminal-support']['opt'])}\tDefine a specific generic terminal support
  > {', '.join(CLI.PARAMETER_TABLE['charset']['opt'])}\t\tDefine a specific generic charset

Terminal supports:
  > {'\x0a  > '.join(STANDARD_PRIORITY)}

Charsets:
  > {'\x0a  > '.join(CHARSET_PRIORITY)}

Exemples:
""")
        return (0)

    def download_package(self):
        # lb = LoadingBar(100, 0, 1)
        # lb1 = LoadingBar(10, 0, 1)
        # lb2 = LoadingBar(1000, 0, 1)
        # lb3 = LoadingBar(75, 0, 1)
        # lb4 = LoadingBar(50, 0, 1)
        # self._graphic.add_elements(lb)
        # self._graphic.add_elements(lb1)
        # self._graphic.add_elements(lb2)
        # self._graphic.add_elements(lb3)
        # self._graphic.add_elements(lb4)
        # for i in range(1, 100):
        #     if (not self.running):
        #         return (1)
        #     lb.push(f"item_{i}")
        #     lb1.push(f"item_{i}")
        #     lb2.push(f"item_{i}")
        #     lb3.push(f"item_{i}")
        #     lb4.push(f"item_{i}")
        #     self._graphic.update()
        #     self._graphic.draw()
        #     sleep(0.1)
        self.wcr.dowload_package("https://developer.mozilla.org/fr/docs/Web/HTTP/Reference/Status/301", "./")
        return (0)

    def run(self) -> int:
        if (not self.wcr):
            self.wcr = WCRState()

        if (self.has_opt("help")):
            return self.show_help()

        if (self.has_opt("download")):
            return self.download_package()

        sys.stderr.write(f"{sys.argv[0]}: no operation specified (use -h for help).\n")
        return (1)

    def close(self):
        self.running: bool = False
        if (self.wcr):
            self.wcr.close()

    def __del__(self):
        self.close()