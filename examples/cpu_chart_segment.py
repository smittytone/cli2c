#!/usr/bin/env python3

import signal
from subprocess import run
from psutil import cpu_percent
from sys import exit, argv
from time import sleep

i2c_device = None
i2c_address = "0x70"

def handler(signum, frame):
    run(["segment", i2c_device, "a", "off"])
    print("Done")
    exit(0)

signal.signal(signal.SIGINT, handler)

if len(argv) > 1:
    i2c_device = argv[1]

if i2c_device:
    run(["segment", i2c_device, "w", "a", "on", "b", "4"])

    while True:
        cpu = int(cpu_percent() * 10.0)
        run(["segment", i2c_device, "n", str(cpu), "d", "2"])
        sleep(0.5)
else:
    print("Usage: python cpu_chart_segment.py {device} {i2C address}")