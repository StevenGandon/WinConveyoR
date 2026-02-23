from src import *

from sys import exit, argv

def main():
    if (len(argv) <= 1):
        print("warning: no password specified, using 'None' as key password.")
    if (len(argv) > 2):
        print("too many arguments.")
        return (1)

    private_key = PrivateSecurityKey.generate()
    public_key = PublicSecurityKey.generate(private_key)

    private_key.write("private.pem", argv[1] if len(argv) > 1 else None)
    public_key.write("public.pem")
    
    return (0)

if (__name__ == "__main__"):
    exit(main())