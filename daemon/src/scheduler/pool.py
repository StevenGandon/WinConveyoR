from .job import Job, JobResult

from time import sleep

_STATIC_DEF_POOL = None

class Pool(object):
    def __init__(self, jobs: list, *, concurrent_threads: int = 5):
        self.concurrent_threads_max: int = concurrent_threads

        self.jobs: list = [*jobs]
        self.running: list = []
        self.results: list = []

    def add_job(self, job: Job):
        self.jobs.append(job)

    def fetch_finish(self):
        if (not self.results):
            return (None)
        return (self.results.pop())

    def collect_result_and_clear_job(self):
        for i, item in enumerate(self.running):
            if (not item.running):
                self.results.append(item.status)
                self.running[i].close()
                self.running[i] = None

        if (None in self.running):
            self.running = list(filter(lambda x: x is not None, self.running))

    def update_pool(self):
        self.collect_result_and_clear_job()

        if (not self.jobs or len(self.running) >= self.concurrent_threads_max):
            return

        job: Job = self.jobs.pop(0)
        self.running.append(job)
        job.run()

    def run_until_end(self):
        while self.jobs:
            self.update_pool()

            sleep(0.01)

        while self.running:
            self.collect_result_and_clear_job()

            sleep(0.01)

    def close(self):
        for item in self.running:
            item.close()

        self.collect_result_and_clear_job()
        
        for item in self.jobs:
            item.close()

        self.jobs.clear()
    
    def __del__(self):
        self.close()

def get_default_pool() -> Pool:
    global _STATIC_DEF_POOL

    if (_STATIC_DEF_POOL):
        return (_STATIC_DEF_POOL)

    _STATIC_DEF_POOL = Pool([])