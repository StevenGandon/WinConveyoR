import sys
from sys import exit
from signal import signal, SIGINT, SIGTERM
from time import sleep, time
from platform import uname
from os.path import expandvars, isfile, dirname
from os import makedirs, environ
from src import *
from libwconr import *

add_dll_registry_path(".")

WIZARD_ASSETS = "assets/wizard"
ASSETS_ENV = "WCR_ASSETS_PATH"

# packages installed by the update job run their wizard: in the executable the lib
# must use the bundled instruction sets, not the build machine repository path
if (is_frozen()):
    environ.setdefault(ASSETS_ENV, get_resource(WIZARD_ASSETS))

CACHE_STORES = expandvars("%LOCALAPPDATA%\\wcr\\cache\\" if uname().system == "Windows" else "${HOME}/.cache/wcr/")
INTEGRITY_TASK_NAME = "integrity-check"
CLEANUP_TASK_NAME = "cleanup"
UPDATE_TASK_NAME = "update"
INTEGRITY_MAX_INSTANCES = 1
CLEANUP_MAX_INSTANCES = 1
UPDATE_MAX_INSTANCES = 1
CONFIG = expandvars("%APPDATA%\\wcr\\daemon.conf" if uname().system == "Windows" else "/etc/wcr.d/daemon.conf")

class Daemon(object):
    def __init__(self):
        makedirs(dirname(CACHE_STORES), exist_ok = True)

        if (isfile(CONFIG)):
            self.settings = Settings.from_file(CONFIG)
        else:
            self.settings = Settings()

        self.pool = Pool([])
        self.task_scheduler = TaskScheduler(self.settings.interval)
        self.cache_service = CacheService()

        self.running: bool = False

        self.cache_service.load(f"{CACHE_STORES}daemon.cache")

        if (self.settings.auto_verify):
            integrity_task = Task(
                task_callback=verify_installed_packages,
                name=INTEGRITY_TASK_NAME,
                pool=self.pool,
                max_instances=INTEGRITY_MAX_INSTANCES
            )
            self.task_scheduler.add_task(integrity_task)

        if (self.settings.auto_update):
            update_task = Task(
                task_callback=update_packages,
                name=UPDATE_TASK_NAME,
                pool=self.pool,
                max_instances=UPDATE_MAX_INSTANCES
            )
            self.task_scheduler.add_task(update_task)

        if (self.settings.auto_clean):
            cleanup_task = Task(
                task_callback=clean_packages,
                name=CLEANUP_TASK_NAME,
                pool=self.pool,
                max_instances=CLEANUP_MAX_INSTANCES
            )
            self.task_scheduler.add_task(cleanup_task)

        signal(SIGINT, lambda *args, **kwargs: self.close())
        signal(SIGTERM, lambda *args, **kwargs: self.close())

    def _flush_logs(self):
        for task in self.task_scheduler.tasks:
            while task.logs:
                sys.stdout.write(task.logs.pop(0))
                sys.stdout.flush()

    def _clock(self, tick_time):
        start = time()

        while (self.running and ((time() - start) < tick_time)):
            sleep(1)

    def run(self):
        self.running = True
        sys.stdout.write(f"[daemon] started, running jobs every {int(self.settings.interval)}s.\n")
        sys.stdout.flush()
        while (self.running):
            self.task_scheduler.tick(False)
            self.pool.run_until_end()
            self._flush_logs()
            self._clock(self.settings.interval)

    def close(self) -> None:
        self.running = False

        self.cache_service.save(f"{CACHE_STORES}daemon.cache")

        self.pool.close()

    def __del__(self):
        self.close()

DAEMON_NAME = "wcr-daemon"
VERSION_ARGS = ("-v", "--version")
HELP_ARGS = ("-h", "--help")

def _wrapper_version() -> str:
    # version of the libwconr wrapper bundled with the daemon (absent when frozen without metadata)
    try:
        from importlib.metadata import version
        return (version("libwconr"))
    except Exception:
        return ("unknown")

# answers without starting the service loop (installer smoke tests call --version)
def handle_arguments(args) -> int:
    if (any(a in VERSION_ARGS for a in args)):
        sys.stdout.write(f"{DAEMON_NAME} (libwconr wrapper {_wrapper_version()})\n")
        return (0)
    if (any(a in HELP_ARGS for a in args)):
        sys.stdout.write(f"Usage: {DAEMON_NAME} [--version | --help]\n"
                         f"Runs the WinConveyoR background jobs (integrity, update, cleanup).\n"
                         f"Configuration: {CONFIG}\n")
        return (0)
    return (-1)

def main() -> int:
    rc = handle_arguments(sys.argv[1:])
    if (rc >= 0):
        return (rc)

    D: Daemon = Daemon()
    D.run()
    D.close()

    return (0)

exit(main())
