from os import environ
from os.path import abspath, expanduser, isfile, dirname, join, realpath
from shutil import which, rmtree
from subprocess import run
from tempfile import mkdtemp

import sys

# F30: user procedure replacing the one of a package (wcr install <pkg> --procedure=<file>).
# A .yaml/.yml uses the wcrpackager syntax (options:/install:/uninstall:, metadata optional)
# and is compiled into a .WIZARD with `wcrpackager --wizard`; any other file is
# expected to be an already compiled .WIZARD.

YAML_EXTENSIONS = (".yaml", ".yml")
COMPILED_NAME = "procedure.WIZARD"
PACKAGER_ENV = "WCR_PACKAGER"
PACKAGER_NAME = "wcrpackager"
REPO_PACKAGER = join("hosting", "mirror-server", "wcrpackager.py")


def _find_packager() -> list:
    # explicit path, then the repo next to this CLI (dev), then the installed tool
    if (environ.get(PACKAGER_ENV)):
        return ([sys.executable, environ[PACKAGER_ENV]])

    repo_root = dirname(dirname(realpath(sys.argv[0])))
    if (isfile(join(repo_root, REPO_PACKAGER))):
        return ([sys.executable, join(repo_root, REPO_PACKAGER)])

    if (which(PACKAGER_NAME)):
        return ([which(PACKAGER_NAME)])
    return (None)


class Procedure(object):
    """Resolves --procedure into a .WIZARD path; use as a context manager
    so the compiled file is removed afterwards."""

    def __init__(self, path: str):
        self.source = abspath(expanduser(path))
        self.path = None
        self.error = None
        self._tmp_dir = None

    def __enter__(self):
        if (not isfile(self.source)):
            self.error = f"procedure '{self.source}' not found."
        elif (self.source.lower().endswith(YAML_EXTENSIONS)):
            self._compile()
        else:
            self.path = self.source
        return (self)

    def __exit__(self, *exc):
        if (self._tmp_dir):
            rmtree(self._tmp_dir, ignore_errors=True)
        return (False)

    def _compile(self):
        packager = _find_packager()

        if (not packager):
            self.error = f"cannot compile '{self.source}': {PACKAGER_NAME} not found (set {PACKAGER_ENV})."
            return

        self._tmp_dir = mkdtemp(prefix="wcr-procedure-")
        output = join(self._tmp_dir, COMPILED_NAME)
        # the packager loads its instruction sets relative to its own directory
        result = run([*packager, "--wizard", self.source, output],
                     cwd=dirname(packager[-1]), capture_output=True, text=True)

        if (result.returncode != 0 or not isfile(output)):
            details = (result.stdout + result.stderr).strip()
            self.error = f"cannot compile '{self.source}':\n{details}"
            return
        self.path = output
