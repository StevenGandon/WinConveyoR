from sys import exit
from signal import signal, SIGINT, SIGTERM

from src import *

INTEGRITY_CHECK_INTERVAL = 60.0
INTEGRITY_TASK_NAME = "integrity-check"
INTEGRITY_MAX_INSTANCES = 1

class Daemon(object):
    def __init__(self):
        self.pool = Pool([])
        self.task_scheduler = TaskScheduler(INTEGRITY_CHECK_INTERVAL)
        self.cache_service = CacheService()

        self.running: bool = False

        self.cache_service.load("./test.cache")

        integrity_task = Task(
            task_callback=verify_installed_packages,
            name=INTEGRITY_TASK_NAME,
            pool=self.pool,
            max_instances=INTEGRITY_MAX_INSTANCES
        )
        self.task_scheduler.add_task(integrity_task)

        signal(SIGINT, lambda *args, **kwargs: self.close())
        signal(SIGTERM, lambda *args, **kwargs: self.close())

    def run(self):
        self.running = True
        while (self.running):
            self.pool.update_pool()
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
