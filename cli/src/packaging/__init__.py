from os.path import dirname, abspath, join
from os import getcwd

import sys

def resource_path(relative_path):
    base_path = getattr(sys, '_MEIPASS', getcwd())
    return join(base_path, relative_path)
