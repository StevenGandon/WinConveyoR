from sys import exit

from src import *

import sys

def main() -> int:
    argsettings = ArgumentParserSettings()

    argsettings.define_option("-h")
    argsettings.define_option("--help")
    argsettings.define_option("-?")

    argsettings.define_option("/h")
    argsettings.define_option("/?")
    argsettings.validate()

    try:
        argparser = ArgumentParser(sys.argv, argsettings)
    except ArgumentHandlerException as e:
        sys.stderr.write(f"{sys.argv[0]}: {e}\n")

    if (any(map(lambda x: x in argparser.options, ("-h", "--help", "-?", "/?", "/h")))):
        sys.stdout.write(f"""Usage: {sys.argv[0]}
""")

    wcr = WCRState()
    wcr.dowload_package("https://stackoverflow.com/questions/61294630/ctypes-passing-a-string-as-a-pointer-from-python-to-c", "./")
    wcr.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())
