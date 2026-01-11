import sys
import pathlib
import time

sys.path.append(str(pathlib.Path(__file__).resolve().parent.parent / 'cli' / 'src'))

from thread.job import Job
from thread.pool import Pool

def test_job_run_and_result():
    Job.reset_id()
    called = []

    def cb(cout):
        cout.append("done")
        called.append(True)
        return 123

    job = Job(cb)
    job.run()
    job.close()

    assert called == [True]
    assert job.status.return_value == 123
    assert not job.status.status_crash
    assert job.status.cout == ["done"]

def test_job_ids_and_on_end():
    Job.reset_id()
    on_end_called = []

    def on_end(cout):
        on_end_called.append(cout[0])

    job1 = Job(lambda c: c.append(1) or 1, on_end=on_end)
    job2 = Job(lambda c: c.append(2) or 2)

    assert job1.id == 0
    assert job2.id == 1

    job1.run()
    job2.run()
    job1.close()
    job2.close()

    assert on_end_called == [1]

def test_pool_runs_jobs_and_collects_results():
    Job.reset_id()

    jobs = [Job(lambda c, i=i: c.append(i) or i) for i in range(3)]

    pool = Pool(jobs, concurrent_threads=2)
    pool.run_pool()

    returns = [res.return_value for res in pool.results]
    assert returns == [0, 1, 2]
    assert all(not res.status_crash for res in pool.results)