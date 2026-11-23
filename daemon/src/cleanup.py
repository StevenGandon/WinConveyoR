from libwconr import *
from platform import uname
from os import listdir, remove, makedirs
from os.path import join, expandvars

def clean_packages(output):
    state = WCRState.load("~/.config/wcr/config")

    if (not state):
        output.write("[cleanup] no WCR state found, skipping.\n")
        return

    cache_path = state.get_cache_path()

    makedirs(cache_path, exist_ok=True)

    for item in listdir(cache_path):
        full_path = join(cache_path, item)

        if (full_path.endswith(".gz")):
            remove(full_path)
            output.write(f"[cleanup] removed cache for {item}.\n")

    state.close()
    output.write("[cleanup] done.\n")

