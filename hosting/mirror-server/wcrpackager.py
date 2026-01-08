from sys import exit
from sys import argv

from genericpath import isdir

def main() -> int:
    if (len(argv) < 2):
        print(f"{argv[0]}: not enough argument.")
        return (1)

    if (not isdir(argv[1])):
        print(f"{argv[0]}: {argv[1]}: not a directory.")
        return (1)

    return (0)

if (__name__ == "__main__"):
    exit(main())
