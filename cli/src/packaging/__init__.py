from os.path import dirname, abspath, join

import sys

# cli/src/packaging -> repository root
_REPO_ROOT = abspath(join(dirname(__file__), "..", "..", ".."))

def get_resource(relative_path: str) -> str:
    """'assets/x.xml' -> <repo>/assets/x.xml from sources, <pyinstaller temp dir>/assets/x.xml
    in the executable (the build bundles it with --add-data)."""
    base_path = getattr(sys, "_MEIPASS", _REPO_ROOT)
    return (join(base_path, relative_path))

def is_frozen() -> bool:
    return (bool(getattr(sys, "frozen", False)))

# former name, kept for existing callers
resource_path = get_resource
