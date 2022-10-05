#!/usr/bin/env python3

import signal
from subprocess import run
from psutil import cpu_percent
from sys import exit, argv
from time import sleep

i2cdevice = None

def handler(signum, frame):
    run(["segment", i2cdevice, "-a", "off"])
    print("Done")
    exit(0)

signal.signal(signal.SIGINT, handler)

if len(argv) > 1:
    i2cdevice = argv[1]

if i2cdevice:
    run(["segment", i2cdevice, "-w", "-a", "on", "-b", "4"])

    while True:
        cpu = int(cpu_percent() * 10.0)
        run(["segment", i2cdevice, "-n", str(cpu), "-d", "2", "-z"])
        sleep(0.5)
else:
    print("[ERROR] No device specified")