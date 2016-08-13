import threading
import logging
import time
import random

import radio


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

    def __init__(self):
        """Initialize data structures needed by the scheduler."""
        self._logger = logging.getLogger(__name__)
        self._jobs = {}
        self._jobs_lock = threading.Lock()
        self._jobs_sem = threading.Semaphore(value=0)
        self._next_id = 0
        self._current_job = None
        self._sched_thread = threading.Thread(target=self, name="Scheduler")

    def start(self):
        """Start scheduling jobs."""
        self._sched_thread.start()

    def stop(self):
        """Stop scheduling jobs."""
        self._sched_thread.stop()

    def __call__(self):
        """Start scheduling jobs for the radio."""
        while True:
            self._run_next_job()

    def insert_job(self, client, duration, weight):
        """Add a job to the list. Return the job's ID."""
        job_id = self._next_id
        self._next_id += 1
        self._jobs_lock.acquire()
        self._jobs[job_id] = (client, duration, weight)
        self._logger.info("Inserted job %d", job_id)
        self._jobs_sem.release()
        self._jobs_lock.release()
        return job_id

    def remove_job(self, client, job_id):
        """Remove a job from the list."""
        if self._jobs_sem.acquire(blocking=False) is False:
            raise RuntimeError("No jobs queued")
        del self._jobs[job_id]

    def _get_next_job(self):
        # TODO: actually calculate the next job
        self._jobs_lock.acquire()
        job_id, job = self._jobs.popitem()
        self._jobs_lock.release()
        return (job_id, job)

    def _run_next_job(self):
        # Wait for a job to be added
        if self._jobs_sem.acquire(blocking=False) == False:
            self._logger.info("Waiting on next job to arrive...")
            self._jobs_sem.acquire()

        job_id, job = self._get_next_job()
        client, duration, weight = job
        self._logger.info("Next job to run is %d", job_id)
        # TODO: use a Manager so it can be shared across processes
        new_lock = threading.Lock()
        new_lock.acquire()
        # TODO: notify the old server of the current job's expiration
        pass
        # TODO: notify the server of the new job and its lock
        pass

        # Stop the current job
        if self._current_job != None:
            self._logger.info("Stopping job %d...", self._current_job[0])
        radio.colors_lock_lock.acquire()
        radio.colors_lock.acquire()
        if self._current_job != None:
            self._logger.info("Stopped.")
        # Switch to the new job
        radio.colors_lock = new_lock
        self._current_job = (job_id, job)
        radio.colors_lock_lock.release()

        # Let the new job run
        new_lock.release()
        # TODO: notify the client it can start
        pass
        self._logger.info("Job %d is now running.", job_id)
        # Wait for the end of the job
        time.sleep(duration)
        self._logger.info("Job %d expired.", job_id)
