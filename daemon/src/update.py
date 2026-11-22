from libwconr import *

def update_packages(output):
    state = WCRState.load("~/.config/wcr/config")
    if (not state):
        output.write("[update] no WCR state found, skipping.\n")
        return

    sources = state.get_sources()
    if (not sources):
        output.write(f"[update] no sources configured.\n")
        state.close()
        return

    failed = 0
    for src in sources:
        output.write(f"[update] syncing from {src['url']}...\n")
        rc = state.sync_package_list(src['proto'], src['url'])
        if (rc != 0):
            output.write(f"[update] sync failed for {src['url']} (rc={rc}).\n")
            failed += 1

    if (failed == len(sources)):
        output.write(f"[update] all sources failed.\n")
        state.close()
        return

    packages = state.list_installed()
    if (not packages):
        output.write("[update] No packages installed.\n")
        return

    upgraded = 0
    failed = 0
    up_to_date = 0

    for pkg in packages:
        meta = None
        src_used = None
        for src in sources:
            meta = state.get_package_metadata(src['proto'], src['url'], pkg['name'])
            if (meta):
                src_used = src
                break

        if (not meta):
            output.write(f"[update] cannot fetch metadata for '{pkg['name']}'.\n")
            failed += 1
            continue

        if (meta['version'] == pkg['version']):
            output.write(f"  {pkg['name']} {pkg['version']} is up to date.\n")
            up_to_date += 1
            continue

        output.write(f"  {pkg['name']} {pkg['version']} -> {meta['version']}\n")
        rc = state.install_package(src_used['proto'], src_used['url'], pkg['name'])
        if (rc == 0):
            upgraded += 1
        else:
            output.write(f"[update] failed to upgrade '{pkg['name']}'.\n")
            failed += 1

    output.write(f"\n{upgraded} upgraded, {up_to_date} up to date, {failed} failed.\n")

    state.close()
    output.write("[update] check complete.\n")
