from time import sleep, time

class Clock(object):
    def __init__(self):
        self.old_time: int = time()
        self.last_dt: int = 0

        self.interrupted: bool = False

    def _sleep(self, sleep_time: float):
        initial = time()
        end_at = initial + sleep_time

        while (end_at - time() > 0.0 and not self.interrupted):
            sleep(0.1)

    def tick(self, ticks = -1):
        if (ticks == 0):
            return
        wait = 1000.0 / ticks if ticks != -1 else -1
        delta: int = time() - self.old_time

        if (wait == -1):
            self.old_time = time()
            self.last_dt = delta
            return (self.last_dt)

        sleep_time = wait - delta

        if (sleep_time > 0.0):
            sleep(sleep_time / 1000.0)
        self.old_time = time()
        self.last_dt = (delta + sleep_time if sleep_time > 0.0 else delta)
        return (self.last_dt)

    def interrupt(self):
        self.interrupted: bool = True

    def resume(self):
        self.interrupted: bool = False