from libwconr import *
from platform import uname
from os import listdir, remove
from os.path import join, expandvars

def clean_packages(output):
    output.write("[cleanup] starting cleanup.\n")

    cache_path = expandvars("%LOCALAPPDATA%\\cache\\wcr" if uname().system == "Windows" else "${HOME}/.cache/wcr")

    for item in listdir(cache_path):
        full_path = join(cache_path, item)

        if (full_path.endswith(".tar.gz")):
            remove(full_path)

    output.write("[cleanup] done.\n")

