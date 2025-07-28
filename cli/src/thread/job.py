from threading import Thread
from time import time

class JobResult(object):
    def __init__(self, status_crash = False, return_value = None, job_id: int = 0, name: str = "uknown", delta_time = 0, cout = []):
        self.status_crash = status_crash
        self.return_value = return_value
        self.delta_time = delta_time
        self.job_id = job_id
        self.name = name
        self.cout = cout

class Job(object):
    _job_id = 0

    def __init__(self, callback, *, name = None, on_end = None):
        self._thread = Thread(target=self._run)
        self._callback = callback

        self.name = f"job#{Job._job_id}" if (not name) else name

        self.id = Job._job_id
        Job._job_id += 1

        self.cout = []

        self.created_at = time()
        self.started_at = None
        self.ended_at = None

        self.running = False
        self.status = None

        self.on_end = on_end

    def _run(self):
        crashed = False
        return_value = None
        try:
            return_value = self._callback(self.cout)
        except Exception:
            crashed = True
        self.ended_at = time()
        self.status = JobResult(crashed, return_value, self.id, self.name, self.ended_at - self.started_at, self.cout)
        self.running = False

        if (self.on_end):
            self.on_end(self.cout)

        self._thread = None

    def run(self):
        if (self._thread is None or self._thread.is_alive()):
            return
        self.running = True
        self.started_at = time()
        self._thread.start()

    def close(self):
        if (self._thread is None):
            return
        if (self.running or self._thread.is_alive()):
            self._thread.join()
        self._thread = None
        self.ended_at = time()
        self.running = False

    @staticmethod
    def reset_id():
        Job._job_id = 0

    def __del__(self):
        self.close()
