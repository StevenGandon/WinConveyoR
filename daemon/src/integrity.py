from libwconr import *

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
