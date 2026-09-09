from .pool import Pool, get_default_pool
from .job import Job

from uuid import uuid4

import sys

class TextIOLocal(object):
    def __init__(self, encoding = "utf-8", errors = "strict") -> None:
        self.closed: bool = False
        self.buffer: bytearray = bytearray()
        self.encoding = encoding
        self.errors = errors
        self.line_buffering = True
        self.mode = "rw"

        self.name = "LocalIO"

        self.file: bytearray = bytearray()

    def detach(self):
        return (None)

    def fileno(self) -> int:
        return (-0xd)

    def isatty(self) -> bool:
        return (False)

    def readable(self) -> bool:
        return True

    def writable(self) -> bool:
        return True

    def close(self) -> None:
        self.closed = True

    def read(self, size: int = -1) -> str:
        if (len(self.buffer)):
            self.flush()
    
        value = bytes(self.file).decode(self.encoding, errors=self.errors)

        return value if size == -1 else value[:size]

    def write(self, s: str) -> int:
        self.buffer.extend(s.encode(self.encoding, errors=self.errors))

        if ('\n' in s):
            self.flush()

    def flush(self) -> None:
        self.file.extend(bytes(self.buffer))
        self.buffer = bytearray()

class Task(object):
    def __init__(self, task_callback = print, name: str = None, pool: Pool = None, max_instances: int = -1):
        self.pool = (pool if pool else get_default_pool())
        self.callback = task_callback
        self.name = (name if name else f"task@{hex(id(self))}")
        self.instance_idx = {}
        self.max_instances = max_instances

        self.local_socket = TextIOLocal()

        self.logs: list = []

    def _end_task_instance(self, cout: list, instance_id: int):
        self.logs.append(''.join(map(str, cout)))

        del self.instance_idx[instance_id]

    def _start_task_instance(self, cout: list, instance_id: int):
        try:
            self.local_socket.write(f"instance#{instance_id} started.\n")

            self.callback(self.local_socket)

            self.local_socket.write(f"instance#{instance_id} stopped.\n")
        except Exception as e:
            self.local_socket.write(f"instance#{instance_id} crashed - {str(e)}.\n")

        for item in self.local_socket.read().replace('\r\n', '\n').split('\n'):
            cout.append(f"{item}\n")

    def run(self):
        if (self.max_instances != -1 and len(self.instance_idx) >= self.max_instances):
            return

        instance_id = uuid4().int
        job: Job = Job(
            callback=lambda cout, x=instance_id: self._start_task_instance(cout, x),
            name=f"{self.name}#0~",
            on_end=lambda cout, x=instance_id: self._end_task_instance(cout, x))        
        job.name = f"{self.name}#{job.id}"

        self.pool.add_job(job)
        self.instance_idx[instance_id] = job

class TaskScheduler(object):
    def __init__(self, tick_rate: int = 60):
        self.tasks = []
        self.tick_rate = tick_rate

    def add_task(self, task: Task):
        self.tasks.append(task)

    def tick(self, sleep_until_next_refresh: bool = True):
        from time import sleep

        for item in self.tasks:
            item.run()

        if (sleep_until_next_refresh):
            sleep(self.tick_rate)
