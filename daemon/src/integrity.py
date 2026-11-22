import sys
import os

_cli_src = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..', 'cli', 'src'))
if (_cli_src not in sys.path):
    sys.path.insert(0, _cli_src)

_lib_build = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..', 'lib', 'libwconr', 'build'))

from wrappers import WCRState, add_dll_registry_path

add_dll_registry_path(_lib_build)

def verify_installed_packages(output):
    state = WCRState.load("~/.config/wcr/config")
    if (not state):
        output.write("[integrity] no WCR state found, skipping.\n")
        return

    sources = state.get_sources()
    if (not sources):
        output.write("[integrity] no sources configured, skipping.\n")
        state.close()
        return

    packages = state.list_installed()
    if (not packages):
        output.write("[integrity] no packages installed.\n")
        state.close()
        return

    output.write(f"[integrity] checking {len(packages)} package(s)...\n")

    for pkg in packages:
        result = None
        for src in sources:
            result = state.verify_cached_package(src['proto'], src['url'], pkg['name'])
            if (result is not None):
                break

        if (result is None):
            output.write(f"[integrity] {pkg['name']}: cannot verify\n")
        elif (result['match'] == 1):
            output.write(f"[integrity] {pkg['name']}: OK\n")
        elif (result['match'] == 0):
            output.write(f"[integrity] {pkg['name']}: MISMATCH (expected={result['expected']}, actual={result['actual']})\n")
        else:
            output.write(f"[integrity] {pkg['name']}: not in cache\n")

    state.close()
    output.write("[integrity] check complete.\n")
