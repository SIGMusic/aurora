#!/usr/bin/env python3

import threading
from time import sleep
import logging
import argparse

from radio import RadioNetwork
from scheduler import Scheduler


parser = argparse.ArgumentParser()
parser.add_argument("--loglevel", default="WARNING",
                    choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL",
                             "debug", "info", "warning", "error", "critical"],
                    help="how verbose of messages to log")
args = parser.parse_args()

logging.basicConfig(level=args.loglevel.upper(),
                    format="%(asctime)s [%(levelname)s] %(message)s")

logger = logging.getLogger(__name__)


def main():
    """Run the radio scheduler interface."""
    net = RadioNetwork()
    net.start()

    sched = Scheduler()
    sched.start()

    sleep(1.5)
    job_id = sched.insert_job(None, 5, None)
    sleep(0.75)
    job_id = sched.insert_job(None, 8, None)

    while True:
        pass


if __name__ == "__main__":
    main()
