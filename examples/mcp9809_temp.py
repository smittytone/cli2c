#!/usr/bin/env python3

import signal
from subprocess import run, PIPE
from psutil import cpu_percent
from sys import exit, argv
from time import sleep

i2c_device = None
i2c_address = None

def handler(signum, frame):
    run(["cli2c", i2c_device, "x"])
    print("\nDone")
    exit(0)

signal.signal(signal.SIGINT, handler)

if len(argv) > 1:
    i2c_device = argv[1]

    if len(argv) > 2:
        i2c_address = argv[2]
    if i2c_address == None:
        i2c_address = 0x18

if i2c_device:
    while True:
        result = run(["cli2c", i2c_device, "w", str(i2c_address), "0x05", "r", str(i2c_address), "2"], stdout=PIPE)
        temp_raw = (int(result.stdout[0:2], 16) << 8) | int(result.stdout[2:], 16)
        temp_col = (temp_raw & 0x0FFF) / 16.0
        if temp_raw & 0x10000: temp_col -= 256.0
        print(" Current temperature: {:.2f}°C\r".format(temp_col), end="")
        sleep(1)
else:
    print("Usage: python mcp9809_temp.py {device} {i2C address}")