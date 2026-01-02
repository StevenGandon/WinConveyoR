from sys import exit
from src import *

def main():
    S = Server()
    S.run()
    S.close()
    return (0)

if (__name__ == "__main__"):
    exit(main())
