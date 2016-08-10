#!/usr/bin/env python3

import threading
from time import sleep
import logging
import argparse

from radio import RadioNetwork
from scheduler import Scheduler


parser = argparse.ArgumentParser()
parser.add_argument("--logging", default=logging.WARNING,
                    choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
                    help="how verbose of messages to log")
args = parser.parse_args()

logging.basicConfig(level=getattr(logging, args.logging.upper()),
                    format="%(asctime)s (%(levelname)s) %(filename)s:%(lineno)d: %(message)s")

logger = logging.getLogger(__name__)


def main():
    """Run the radio scheduler interface."""
    net = RadioNetwork()
    net.start()

    sched = Scheduler()
    sched_thread = threading.Thread(target=sched, name="Scheduler")
    sched_thread.start()

    sleep(2)
    job_id = sched.insert_job(None, 15, None)
    logger.info("Scheduled job %d", job_id)

    while True:
        pass


if __name__ == "__main__":
    main()
