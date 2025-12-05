from time import time, sleep

class Clock(object):
    def __init__(self) -> None:
        self.old_time = time()

    def tick(self, fps = -1) -> int:
        execution_time = 1.0 / fps
        delta = time() - self.old_time

        if (fps == -1):
            self.old_time = time()
            return (delta)
        else:
            sleep_time = execution_time - delta

            if (sleep_time >= 0.0):
                sleep(round(sleep_time))

            self.old_time = time()
            return (delta + sleep_time if sleep_time > 0.0 else delta)
