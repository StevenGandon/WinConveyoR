from sys import exit
from signal import signal, SIGINT, SIGTERM

from src import *

class Daemon(object):
    def __init__(self):
        self.pool = Pool([])
        self.task_scheduler = TaskScheduler(60.0)
        self.cache_service = CacheService()

        self.running: bool = False

        self.cache_service.load("./test.cache")

        signal(SIGINT, lambda *args, **kwargs: self.close())
        signal(SIGTERM, lambda *args, **kwargs: self.close())

    def run(self):
        self.running = True
        while (self.running):
            self.task_scheduler.tick(True)

    def close(self) -> None:
        self.running = False

        self.cache_service.save("./test.cache")

        self.pool.close()

    def __del__(self):
        self.close()

def main() -> int:
    D: Daemon = Daemon()
    D.run()
    D.close()

    return (0)

exit(main())
