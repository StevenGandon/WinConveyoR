from sys import exit
from signal import signal, SIGINT, SIGTERM

from src import *



def main():
    try:
        nc = NetworkCLI()
    except ConnectionError as e:
        print(f"failed to connect to server. ({e})")
        return (1)
    nc.run()
    nc.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())