#!/usr/bin/env python3

import signal
from subprocess import run, TimeoutExpired
from psutil import cpu_percent
from sys import exit, argv
from time import sleep

i2c_device = None
i2c_address = "0x60"
col = 0
cols = [0,0,0,0,0,0,0,0,0,0]

def handler(signum, frame):
    run(["/Users/smitty/Library/Developer/Xcode/DerivedData/cli2c-dwftsezvbxnzhwcqphbablmhweao/Build/Products/Debug/matrix", i2c_device, i2c_address, "a", "off"])
    print("Done")
    exit(0)

signal.signal(signal.SIGINT, handler)

if len(argv) > 1:
    i2c_device = argv[1]

if len(argv) > 2:
    i2c_address = argv[2]
    print("Using device at", i2c_address)

if i2c_device:
    run(["/Users/smitty/Library/Developer/Xcode/DerivedData/cli2c-dwftsezvbxnzhwcqphbablmhweao/Build/Products/Debug/matrix", i2c_device, i2c_address, "w", "a", "on", "b", "64"])
    
    while True:
        cpu = int(cpu_percent())

        if col > 9:
            del cols[0]
            cols.append(cpu)
        else:
            cols[9 - col] = cpu
        col += 1

        data_string = ""
        for i in range(0, 10):
            a = cols[i];
            b = 0
            if a > 89:
                b = 0x7F
            elif a > 74: 
                b = 0x7E
            elif a > 59:
                b = 0x7C
            elif a > 44:
                b = 0x78
            elif a > 29:
                b = 0x70
            elif a > 14:
                b = 0x60
            elif a > 0:
                b = 0x40
            data_string += "0x{:02x},".format(b)
        data_string = data_string[:-1]
        
        try:
            run(["/Users/smitty/Library/Developer/Xcode/DerivedData/cli2c-dwftsezvbxnzhwcqphbablmhweao/Build/Products/Debug/matrix", i2c_device, i2c_address, "g", data_string], timeout=2)
        except TimeoutExpired:
            print("Communications with Bus Host timed out. Check the heartbeat LED")
            exit(1)
        sleep(0.5)
else:
    print("Usage: python cpu_chart_matrix.py {device} {i2C address}")
