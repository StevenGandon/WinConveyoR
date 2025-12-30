from sys import exit

from src import *

def main() -> int:
    ms = MirrorServer(".")
    ms.load()

    new_package = ms.add_package_register("test")
    listing = new_package.add_package_listing({

    }, "./test1-0-0.tar.gz")

    ms.write()

    new_package.remove_package_listing(listing.package_data["SHA256"], hard_delete=True)
    ms.remove_package_register("test", hard_delete=True)

    print(ms)
    ms.write()
    return (0)

if (__name__== "__main__"):
    exit(main())