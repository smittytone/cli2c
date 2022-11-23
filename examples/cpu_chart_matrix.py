#!/usr/bin/env python3

import signal
from subprocess import run, TimeoutExpired, STDOUT
from psutil import cpu_percent
from sys import exit, argv
from time import sleep

#matrix = "matrix"
matrix = "/Users/smitty/Library/Developer/Xcode/DerivedData/cli2c-dwftsezvbxnzhwcqphbablmhweao/Build/Products/Debug/matrix"
device = None
i2c_address = "0x70"
col = 0
cols = [0,0,0,0,0,0,0,0]

def handler(signum, frame):
    run([matrix, device, "a", "off"])
    print("Done")
    exit(0)

signal.signal(signal.SIGINT, handler)

if len(argv) > 1:
    device = argv[1]

if len(argv) > 2:
    i2c_device = int(argv[2])
    print(f"Using I2C device at 0x{i2c_device:02X}")

if device:
    run([matrix, device, "w", "a", "on", "b", "2"])
    err = ""
        
    while True:
        cpu = int(cpu_percent())

        if col > 7:
            del cols[0]
            cols.append(cpu)
        else:
            cols[7 - col] = cpu
        col += 1

        data_string = ""
        for i in range(0, 8):
            a = cols[i];
            b = 0
            if a > 87:
                b = 0xFF
            elif a > 75: 
                b = 0x7F
            elif a > 62:
                b = 0x3F
            elif a > 49:
                b = 0x1F
            elif a > 36:
                b = 0x0F
            elif a > 25:
                b = 0x07
            elif a > 12:
                b = 0x03
            elif a > 0:
                b = 0x01
            data_string += "0x{:02x},".format(b)
        data_string = data_string[:-1]
        
        try:
            run([matrix, device, "g", data_string], timeout=1.0)
        except TimeoutExpired:
            print("Attempt to write data timed out")
            exit(1)

        sleep(0.5)
else:
    print("Usage: python cpu_chart_matrix.py {device} {i2C address}")
