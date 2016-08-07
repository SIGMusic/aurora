import threading
import sched
import time
import random

from radio import colors, colors_lock, colors_lock_lock


class Scheduler:

    """Schedule and run incoming jobs according to Weighted Fair
    Queueing.

    Given a limited resource (the radio network) and unlimited demand
    (connected clients' animations), a scheduler is necessary to
    arbitrate access to the resource.

    This class implements Weighted Fair Queueing. WFQ is a
    packet-based approximation of Generalized Processor Sharing.
    In this case, the radio represents the processor and each animation
    represents a packet. For this to be feasible, each animation must
    specify a duration to serve as the packet length. This restriction
    prevents clients from running infinite animations but also
    guarantees their atomicity. Clients wishing to use infinite
    animations can instead schedule multiple discrete animations.

    The way this class gatekeeps the radio is by assigning a unique
    lock to each job. The color data is protected by the unique lock,
    which is changed out every time a new job runs.

    """

    _jobs = {}
    _jobs_lock = threading.Lock()
    _jobs_sem = threading.Semaphore(value=0)

    _sched = sched.scheduler()

    _next_id = random.randint(1000, 10000)

    def __init__(self):
        """Initialize data structures needed by the scheduler."""
        # TODO
        pass

    def start(self):
        """Start scheduling jobs for the radio."""
        # TODO: start thread
        self._start_next_job()

    def insert_job(self, client, duration, weight):
        """Add a job to the list. Return the job's ID."""
        job_id = self._next_id
        self._next_id += random.randint(1, 10)
        self._jobs_lock.acquire()
        self._jobs[job_id] = (client, duration, weight)
        self._jobs_sem.release()
        self._jobs_lock.release()
        return job_id

    def remove_job(self, client, job_id):
        """Remove a job from the list."""
        # TODO
        if self._jobs_sem.acquire(blocking=False) is False:
            raise RuntimeError("No jobs queued")
        del self._jobs[job_id]

    def _get_next_job(self):
        # TODO: actually calculate the next job
        self._jobs_lock.acquire()
        job_id, job = self._jobs.popitem()
        self._jobs_lock.release()
        return (job_id, job)

    def _start_next_job(self):
        # Wait for a job to be added
        self._jobs_sem.acquire()
        
        job_id, job = self._get_next_job()
        client, duration, weight = job
        # TODO: use a Manager so it can be shared across processes
        new_lock = threading.Lock()
        new_lock.acquire()
        # TODO: notify the server of the new job and its lock
        pass

        # Stop the current job
        radio.colors_lock_lock.acquire()
        radio.colors_lock.acquire()
        # Switch to the new job
        radio.colors_lock = lock
        radio.colors_lock_lock.release()

        # Schedule the end of the job
        self._sched.enter(duration, 1, _start_next_job)
        # Let the new job run
        lock.release()
        # Notify the client it can start
        # TODO
        pass
