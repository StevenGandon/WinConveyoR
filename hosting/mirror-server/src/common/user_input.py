import sys

if (sys.platform.startswith("win")):
    from msvcrt import getch, kbhit

    def init_terminal(fileno = None):
        return (None)

    def uninit_terminal(old_settings, fileno = None):
        pass

    def non_blocking_read():
        if (kbhit()):
            return getch()
        return (None)
else:
    from os import read
    from tty import setcbreak, setraw
    from termios import tcsetattr, tcgetattr, TCSADRAIN, TCSAFLUSH, BRKINT, ICRNL, INPCK, ISTRIP, IXON, OPOST, CSIZE, PARENB, CS8, ECHO, ICANON, IEXTEN, VMIN, VTIME
    from select import select

    IFLAG = 0
    OFLAG = 1
    CFLAG = 2
    LFLAG = 3
    ISPEED = 4
    OSPEED = 5
    CC = 6

    def _setraw(fd, when=TCSAFLUSH):
        mode = tcgetattr(fd)
        mode[IFLAG] = mode[IFLAG] & ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON)
        mode[OFLAG] = mode[OFLAG] # & ~(OPOST)
        mode[CFLAG] = mode[CFLAG] & ~(CSIZE | PARENB)
        mode[CFLAG] = mode[CFLAG] | CS8
        mode[LFLAG] = mode[LFLAG] & ~(ECHO | ICANON | IEXTEN)
        mode[CC][VMIN] = 1
        mode[CC][VTIME] = 0
        tcsetattr(fd, when, mode)

    def init_terminal(fileno = None):
        if (fileno is None):
            fileno = sys.stdin.fileno()

        attrs = tcgetattr(fileno)

        _setraw(fileno)
        setcbreak(fileno, TCSAFLUSH)

        return (attrs)

    def uninit_terminal(old_settings, fileno = None):
        if (fileno is None):
            fileno = sys.stdin.fileno()
        tcsetattr(fileno, TCSADRAIN, old_settings)

    def non_blocking_read():
        rlist, _, _ = select([sys.stdin.fileno()], [], [], 0)

        for item in rlist:
            if (item == sys.stdin.fileno()):
                return read(sys.stdin.fileno(), 1)
        
        return (None)