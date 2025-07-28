from .job import Job, JobResult

from time import sleep

class Pool(object):
    def __init__(self, jobs: list, *, concurrent_threads: int = 5):
        self.concurrent_threads_max: int = concurrent_threads

        self.pool_base_id: int = Job._job_id

        self.jobs: list = [*jobs]
        self.running: list = []
        self.results: list = [None for _ in self.jobs]

    def collect_result_and_clear_job(self):
        for i, item in enumerate(self.running):
            if (not item.running):
                self.results[item.id - self.pool_base_id] = item.status
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

    def run_pool(self):
        while self.jobs:
            self.update_pool()

            sleep(0.01)

        while self.running:
            self.collect_result_and_clear_job()

            sleep(0.01)
