#!/usr/bin/env python3

from radio import RadioNetwork
from scheduler import Scheduler


def main():
    """Run the radio scheduler interface."""
    net = RadioNetwork()
    net.start()

    sched = Scheduler()
    sched.start()

    while True:
        pass


if __name__ == "__main__":
    main()
