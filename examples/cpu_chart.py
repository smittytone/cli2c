#!/usr/bin/env python3

from subprocess import run
from psutil import cpu_percent
from sys import exit
from time import sleep
import os

print("---------------- INIT ----------------")

#with open(os.devnull, 'w') as devnull:
run(["matrix", "/dev/cu.usbmodem1401", "0x72", "-a", "on", "-b", "2", "-w"])
sleep(1)

print("---------------- START ----------------")

col = 0
cols = [0,0,0,0,0,0,0,0]
cycles = 0

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
    
    #with open(os.devnull, 'w') as devnull:
    run(["matrix", "/dev/cu.usbmodem1401", "0x72", "-g", data_string])
    
    cycles += 1
    print("----------------",cycles,"----------------")
    sleep(0.5)
