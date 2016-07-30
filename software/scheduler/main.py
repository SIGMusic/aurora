#!/usr/bin/env python3

from radio import RadioNetwork
from scheduler import Scheduler

net = RadioNetwork()
net.start()

sched = Scheduler()
sched.start()
