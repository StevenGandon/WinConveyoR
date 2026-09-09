from locale import getpreferredencoding
from os import environ, pipe, close
from itertools import chain
from signal import SIGINT, SIGTERM, signal
from time import sleep
from json import dumps

from ..arghandler import *
from ..wrappers import *
from ..graphic import *
from ..packaging import resource_path
from .. import api_client

class CLI(object):
    OPTION_TABLE: dict = {
        "help": {"opt": ("help", "-h", "--help", "-?", "/?", "/h"), "exc": ()},
        "download": {"opt": ("-d", "--download", "-dwnld"), "exc": ()},
        "install": {"opt": ("install", "-i", "--install"), "exc": ()},
        "uninstall": {"opt": ("uninstall", "--uninstall"), "exc": ()},
        "list": {"opt": ("list", "--list"), "exc": ()},
        "update": {"opt": ("update", "-u", "--update"), "exc": ()},
        "register": {"opt": ("register",), "exc": ()},
        "login": {"opt": ("login",), "exc": ()},
        "search": {"opt": ("search",), "exc": ()},
        "info": {"opt": ("info",), "exc": ()},
        "check": {"opt": ("check",), "exc": ()},
        "upgrade": {"opt": ("upgrade",), "exc": ()},
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
            PatternLoader(resource_path("assets/cli/graphic/patterns/pattern_loading.xml")),
            PatternLoader(resource_path("assets/cli/graphic/patterns/pattern_wizard.xml"))
        ]

        self._pipes = list(pipe())

        if (self._pipes[0] <= -1 or self._pipes[1] <= -1):
            raise (OSError("Failed to open pipes."))

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
        col = 32
        opts = [
            (', '.join(CLI.OPTION_TABLE['help']['opt']), "Display this help message"),
            (', '.join(CLI.OPTION_TABLE['install']['opt']), "Install a package"),
            (', '.join(CLI.OPTION_TABLE['uninstall']['opt']), "Uninstall a package"),
            (', '.join(CLI.OPTION_TABLE['update']['opt']), "Sync package lists from sources"),
            (', '.join(CLI.OPTION_TABLE['list']['opt']), "List installed packages"),
            (', '.join(CLI.OPTION_TABLE['search']['opt']), "Search available packages"),
            (', '.join(CLI.OPTION_TABLE['info']['opt']), "Show package variants"),
            (', '.join(CLI.OPTION_TABLE['check']['opt']), "Verify cached package hash"),
            (', '.join(CLI.OPTION_TABLE['upgrade']['opt']), "Upgrade all packages (or a specific one)"),
            (', '.join(CLI.OPTION_TABLE['register']['opt']), "Create an account"),
            (', '.join(CLI.OPTION_TABLE['login']['opt']), "Log in to your account"),
            (', '.join(CLI.OPTION_TABLE['nocolor']['opt']), "Disable color rendering"),
            (', '.join(CLI.OPTION_TABLE['noansi']['opt']), "Disable ansi rendering"),
            (', '.join(CLI.OPTION_TABLE['ascii']['opt']), "Rendering only in ascii"),
        ]
        params = [
            (', '.join(CLI.PARAMETER_TABLE['terminal-support']['opt']), "Define a specific generic terminal support"),
            (', '.join(CLI.PARAMETER_TABLE['charset']['opt']), "Define a specific generic charset"),
        ]

        lines = [f"Usage: {sys.argv[0]} <-d|-h|...> [options] [arguments]", "", "Options:"]
        for label, desc in opts:
            lines.append(f"  > {label.ljust(col)}{desc}")
        lines.append("")
        lines.append("Parameters:")
        for label, desc in params:
            lines.append(f"  > {label.ljust(col)}{desc}")
        lines.append("")
        lines.append("Terminal supports:")
        for item in STANDARD_PRIORITY:
            lines.append(f"  > {item}")
        lines.append("")
        lines.append("Charsets:")
        for item in CHARSET_PRIORITY:
            lines.append(f"  > {item}")
        lines.append("")
        lines.append("Exemples:")
        lines.append("")

        sys.stdout.write('\n'.join(lines))
        return (0)

    def download_package(self):
        # w = WizardGraphic()
        # lb = LoadingBar(100, 0, 1)
        # lb1 = LoadingBar(10, 0, 1)
        # lb2 = LoadingBar(1000, 0, 1)
        # lb3 = LoadingBar(75, 0, 1)
        # lb4 = LoadingBar(50, 0, 1)
        # self._graphic.add_elements(w)
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

    def install_package(self):
        package_name = self.argparser.arguments[1].value if len(self.argparser.arguments) > 1 else None

        if (not package_name):
            sys.stderr.write(f"{sys.argv[0]} install: no package name provided.\n")
            return (1)

        sources = self.wcr.get_sources()
        if (not sources):
            sys.stderr.write(f"{sys.argv[0]} install: no sources configured.\n")
            return (1)

        for src in sources:
            sys.stdout.write(f"{sys.argv[0]} install: trying '{package_name}' from {src['url']}...\n")
            rc = self.wcr.install_package(src['proto'], src['url'], package_name)
            if (rc == 0):
                sys.stdout.write(f"{sys.argv[0]} install: ok.\n")
                return (0)

        sys.stderr.write(f"{sys.argv[0]} install: failed from all sources.\n")
        return (1)

    def uninstall_package(self):
        package_name = self.argparser.arguments[1].value if len(self.argparser.arguments) > 1 else None

        if (not package_name):
            sys.stderr.write(f"{sys.argv[0]} uninstall: no package name provided.\n")
            return (1)

        rc = self.wcr.uninstall_package(package_name)
        if (rc == 0):
            sys.stdout.write(f"{sys.argv[0]} uninstall: ok.\n")
            return (0)

        sys.stderr.write(f"{sys.argv[0]} uninstall: failed.\n")
        return (1)

    def list_packages(self):
        packages = self.wcr.list_installed()

        if (not hasattr(sys.stdout, 'isatty') or not sys.stdout.isatty()):
            sys.stdout.write(dumps(packages) + "\n")
            return (0)

        if (not packages):
            sys.stdout.write("No packages installed.\n")
            return (0)

        for p in packages:
            sys.stdout.write(f"{p['name']} {p['version']}\n")

        sys.stdout.write(f"\n{len(packages)} package(s) installed.\n")
        return (0)

    def update_sources(self):
        sources = self.wcr.get_sources()
        if (not sources):
            sys.stderr.write(f"{sys.argv[0]} update: no sources configured.\n")
            return (1)

        failed = 0
        for src in sources:
            sys.stdout.write(f"{sys.argv[0]} update: syncing from {src['url']}...\n")
            rc = self.wcr.sync_package_list(src['proto'], src['url'])
            if (rc != 0):
                sys.stderr.write(f"{sys.argv[0]} update: sync failed for {src['url']} (rc={rc}).\n")
                failed += 1

        if (failed == len(sources)):
            sys.stderr.write(f"{sys.argv[0]} update: all sources failed.\n")
            return (1)

        self.wcr.save("~/.config/wcr/config")
        sys.stdout.write(f"{sys.argv[0]} update: ok.\n")
        return (0)

    def register_user(self):
        from getpass import getpass

        sys.stdout.write("Username: ")
        sys.stdout.flush()
        username = input()
        sys.stdout.write("Email: ")
        sys.stdout.flush()
        email = input()
        password = getpass("Password: ")
        confirm = getpass("Confirm password: ")

        if (password != confirm):
            sys.stderr.write(f"{sys.argv[0]} register: passwords do not match.\n")
            return (1)

        sys.stdout.write("Full name (optional): ")
        sys.stdout.flush()
        full_name = input() or None

        try:
            user = api_client.register(username, email, password, full_name)
            sys.stdout.write(f"{sys.argv[0]} register: account created ({user['username']}).\n")
            return (0)
        except RuntimeError as e:
            sys.stderr.write(f"{sys.argv[0]} register: {e}\n")
            return (1)

    def _has_specifier_filters(self, name):
        return (':' in name or '@' in name or '#' in name)

    def _info_variants(self, package_name, sources):
        variants = []
        for src in sources:
            variants = self.wcr.list_package_variants(src['proto'], src['url'], package_name)
            if (variants):
                break

        if (not hasattr(sys.stdout, 'isatty') or not sys.stdout.isatty()):
            sys.stdout.write(dumps(variants) + "\n")
            return (0)

        if (not variants):
            sys.stdout.write(f"No variants found for '{package_name}'.\n")
            return (0)

        col = 16
        sys.stdout.write(f"Variants for '{package_name}':\n\n")
        sys.stdout.write(f"  {'VERSION'.ljust(col)}{'ARCH'.ljust(col)}MACHINE\n")
        sys.stdout.write(f"  {'-' * (col - 1) + ' '}{'-' * (col - 1) + ' '}{'-' * (col - 1)}\n")
        for v in variants:
            sys.stdout.write(f"  {v['version'].ljust(col)}{v['arch'].ljust(col)}{v['machine']}\n")

        sys.stdout.write(f"\n{len(variants)} variant(s).\n")
        return (0)

    def _info_metadata(self, package_spec, sources):
        meta = None
        for src in sources:
            meta = self.wcr.get_package_metadata(src['proto'], src['url'], package_spec)
            if (meta):
                break

        if (not hasattr(sys.stdout, 'isatty') or not sys.stdout.isatty()):
            sys.stdout.write(dumps(meta) + "\n")
            return (0)

        if (not meta):
            sys.stdout.write(f"No metadata found for '{package_spec}'.\n")
            return (0)

        col = 16
        sys.stdout.write(f"\n  {'Package:'.ljust(col)}{meta['name']}\n")
        sys.stdout.write(f"  {'Version:'.ljust(col)}{meta['version']}\n")
        sys.stdout.write(f"  {'Arch:'.ljust(col)}{meta['arch']}\n")
        sys.stdout.write(f"  {'Machine:'.ljust(col)}{meta['machine']}\n")

        if (meta['description']):
            sys.stdout.write(f"  {'Description:'.ljust(col)}{meta['description']}\n")
        if (meta['depends']):
            sys.stdout.write(f"  {'Depends:'.ljust(col)}{', '.join(meta['depends'])}\n")
        if (meta['size']):
            sys.stdout.write(f"  {'Size:'.ljust(col)}{meta['size']} bytes\n")
        if (meta['SHA256']):
            sys.stdout.write(f"  {'SHA256:'.ljust(col)}{meta['SHA256']}\n")
        if (meta['MD5sum']):
            sys.stdout.write(f"  {'MD5:'.ljust(col)}{meta['MD5sum']}\n")

        sys.stdout.write("\n")
        return (0)

    def info_package(self):
        package_name = self.argparser.arguments[1].value if len(self.argparser.arguments) > 1 else None

        if (not package_name):
            sys.stderr.write(f"{sys.argv[0]} info: no package name provided.\n")
            return (1)

        sources = self.wcr.get_sources()
        if (not sources):
            sys.stderr.write(f"{sys.argv[0]} info: no sources configured.\n")
            return (1)

        if (self._has_specifier_filters(package_name)):
            return self._info_metadata(package_name, sources)

        return self._info_variants(package_name, sources)

    def search_packages(self):
        query = self.argparser.arguments[1].value if len(self.argparser.arguments) > 1 else ""

        packages = self.wcr.search_available(query)

        if (not hasattr(sys.stdout, 'isatty') or not sys.stdout.isatty()):
            sys.stdout.write(dumps(packages) + "\n")
            return (0)

        if (not packages):
            sys.stdout.write(f"No packages found{' for ' + repr(query) if query else ''}.\n")
            return (0)

        col = 32
        for p in packages:
            sys.stdout.write(f"  {p['name'].ljust(col)}{p['version']}\n")

        sys.stdout.write(f"\n{len(packages)} package(s) available.\n")
        return (0)

    def check_package(self):
        package_spec = self.argparser.arguments[1].value if len(self.argparser.arguments) > 1 else None

        if (not package_spec):
            sys.stderr.write(f"{sys.argv[0]} check: no package specifier provided.\n")
            return (1)

        sources = self.wcr.get_sources()
        if (not sources):
            sys.stderr.write(f"{sys.argv[0]} check: no sources configured.\n")
            return (1)

        result = None
        for src in sources:
            result = self.wcr.verify_cached_package(src['proto'], src['url'], package_spec)
            if (result is not None):
                break

        if (result is None):
            sys.stderr.write(f"{sys.argv[0]} check: cannot verify '{package_spec}'.\n")
            return (1)

        if (result['match'] == -1):
            sys.stdout.write(f"  Package '{package_spec}' not found in cache.\n")
            sys.stdout.write(f"  Expected SHA256: {result['expected']}\n")
            return (1)

        col = 16
        sys.stdout.write(f"\n  {'Package:'.ljust(col)}{package_spec}\n")
        sys.stdout.write(f"  {'Expected:'.ljust(col)}{result['expected']}\n")
        sys.stdout.write(f"  {'Actual:'.ljust(col)}{result['actual']}\n")

        if (result['match'] == 1):
            sys.stdout.write(f"  {'Status:'.ljust(col)}OK\n\n")
            return (0)

        sys.stdout.write(f"  {'Status:'.ljust(col)}MISMATCH\n\n")
        return (1)

    def upgrade_packages(self):
        package_name = self.argparser.arguments[1].value if len(self.argparser.arguments) > 1 else None

        packages = self.wcr.list_installed()
        if (not packages):
            sys.stdout.write("No packages installed.\n")
            return (0)

        sources = self.wcr.get_sources()
        if (not sources):
            sys.stderr.write(f"{sys.argv[0]} upgrade: no sources configured.\n")
            return (1)

        if (package_name):
            packages = [p for p in packages if p['name'] == package_name]
            if (not packages):
                sys.stderr.write(f"{sys.argv[0]} upgrade: '{package_name}' is not installed.\n")
                return (1)

        upgraded = 0
        failed = 0
        up_to_date = 0

        for pkg in packages:
            meta = None
            src_used = None
            for src in sources:
                meta = self.wcr.get_package_metadata(src['proto'], src['url'], pkg['name'])
                if (meta):
                    src_used = src
                    break

            if (not meta):
                sys.stderr.write(f"{sys.argv[0]} upgrade: cannot fetch metadata for '{pkg['name']}'.\n")
                failed += 1
                continue

            if (meta['version'] == pkg['version']):
                sys.stdout.write(f"  {pkg['name']} {pkg['version']} is up to date.\n")
                up_to_date += 1
                continue

            sys.stdout.write(f"  {pkg['name']} {pkg['version']} -> {meta['version']}\n")
            rc = self.wcr.install_package(src_used['proto'], src_used['url'], pkg['name'])
            if (rc == 0):
                upgraded += 1
            else:
                sys.stderr.write(f"{sys.argv[0]} upgrade: failed to upgrade '{pkg['name']}'.\n")
                failed += 1

        sys.stdout.write(f"\n{upgraded} upgraded, {up_to_date} up to date, {failed} failed.\n")
        return (1 if failed else 0)

    def login_user(self):
        from getpass import getpass

        sys.stdout.write("Email: ")
        sys.stdout.flush()
        email = input()
        password = getpass("Password: ")

        try:
            api_client.login(email, password)
            sys.stdout.write(f"{sys.argv[0]} login: ok.\n")
            return (0)
        except RuntimeError as e:
            sys.stderr.write(f"{sys.argv[0]} login: {e}\n")
            return (1)

    def _make_event_callback(self):
        use_color = hasattr(self, '_graphic') and self._graphic._settings.color

        colors = {
            0: "\033[90m",
            1: "\033[34m",
            2: "\033[33m",
            3: "\033[31m",
            4: "\033[36m",
        }
        reset = "\033[0m"

        def _cb(event_type, message, bytes_done, bytes_total):
            return

        return (_cb)

    def _load_state(self):
        state = WCRState.load("~/.config/wcr/config")
        if state is not None:
            return state
        state = WCRState()
        state.add_source(0, "http://localhost:8080")
        state.add_source(1, "localhost:1674")
        return state

    def run(self) -> int:
        if (not self.wcr):
            self.wcr = self._load_state()
            self.wcr.set_event_callback(self._make_event_callback())

        if (self.has_opt("help")):
            return self.show_help()

        if (self.has_opt("download")):
            return self.download_package()

        if (self.has_opt("install")):
            return self.install_package()

        if (self.has_opt("uninstall")):
            return self.uninstall_package()

        if (self.has_opt("list")):
            return self.list_packages()

        if (self.has_opt("update")):
            return self.update_sources()

        if (self.has_opt("search")):
            return self.search_packages()

        if (self.has_opt("info")):
            return self.info_package()

        if (self.has_opt("check")):
            return self.check_package()

        if (self.has_opt("upgrade")):
            return self.upgrade_packages()

        if (self.has_opt("register")):
            return self.register_user()

        if (self.has_opt("login")):
            return self.login_user()

        sys.stderr.write(f"{sys.argv[0]}: no operation specified (use -h for help).\n")
        return (1)

    def close(self):
        self.running: bool = False

        if (self.wcr):
            self.wcr.close()

        if (self._pipes[0] > -1):
            close(self._pipes[0])
            self._pipes[0] = -1
        if (self._pipes[1] > -1):
            close(self._pipes[1])
            self._pipes[1] = -1

    def __del__(self):
        self.close()