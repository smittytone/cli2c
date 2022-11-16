#!/usr/bin/env python3

'''
IMPORTS
'''
from os import path
from psutil import cpu_percent
from sys import exit, argv
from time import sleep, time_ns


'''
GLOBALS
'''
CHARSET = b'\x3F\x06\x5B\x4F\x66\x6D\x7D\x07\x7F\x6F\x5F\x7C\x58\x5E\x7B\x71\x40\x63'
POS = (0, 2, 6, 8)


'''
FUNCTIONS
'''
def await_ok(uart, timeout=2000):
    if await_data(uart, 0, timeout) == "OK":
        return True
    # Error -- No OK received
    return False


def await_data(uart, count=0, timeout=5000):
    buffer = bytes()
    now = (time_ns() // 1000000)
    while ((time_ns() // 1000000) - now) < timeout:
        if uart.in_waiting > 0:
            buffer += uart.read(uart.in_waiting)
            if count == 0 and len(buffer) >= 4:
                if "\r\n" in buffer.decode():
                    return buffer.decode()[0:-2]
                break
            if count != 0 and len(buffer) >= count:
                print(buffer)
                return buffer.decode()[:count]
    # Error -- No data received (or mis-formatted)
    return ""


def await_ack(uart, timeout=2000):
    buffer = bytes()
    now = (time_ns() // 1000000)
    while ((time_ns() // 1000000) - now) < timeout:
        if uart.in_waiting > 0:
            r = uart.read(1)
            if r[0] == 0x0F:
                return True
    # Error -- No Ack received
    return False


def write(uart, data, timeout=2000):
    l = uart.write(data)
    return await_ack(uart, timeout)


def show_error(message):
    print("[ERROR]", message)
    exit(1)


def str_to_int(num_str):
    num_base = 10
    if num_str[:2] == "0x": num_base = 16
    try:
        return int(num_str, num_base)
    except ValueError:
        return -1


def bcd(base):
    if base > 9999: base = 9999
    for i in range(0, 16):
        base = base << 1
        if i == 15: break
        if (base & 0x000F0000) > 0x0004FFFF: base += 0x00030000
        if (base & 0x00F00000) > 0x004FFFFF: base += 0x00300000
        if (base & 0x0F000000) > 0x04FFFFFF: base += 0x03000000
        if (base & 0xF0000000) > 0x4FFFFFFF: base += 0x30000000
    return (base >> 16) & 0xFFFF


'''
RUNTIME START
'''
if __name__ == '__main__':

    device = None
    i2c_address = 0x70
    last_cpu = 0

    if len(argv) > 1:
        device = argv[1]

    if len(argv) > 2:
        i2c_address = str_to_int(argv[2])
        if i2c_address == -1:
            show_error("Invalid I2C address")
        print(f"Using device at 0x{i2c_address:2X}")

    if device:
        # Set the port or fail
        port = None
        try:
            import serial
            port = serial.Serial(port=device, baudrate=1000000)
        except:
            show_error(f"An invalid device file specified: {device}")

        if port:
            # Check we can connect
            r = port.write(b'\x23\x69\x21')
            if await_ok(port) is True:
                r = write(port, b'\x23\x69\x69')
                out = bytearray(4)
                out[0] = 0x23
                out[1] = 0x69
                out[2] = 0x73
                out[3] = (i2c_address << 1)
                r = write(port, out)
                r = write(port, b'\xC0\x21')
                r = write(port, b'\xC0\x81')
                r = write(port, b'\xC0\xE4')
                
                while True:
                    cpu = int(cpu_percent() * 10.0)
                    if cpu != last_cpu:
                        last_cpu = cpu
                        out = bytearray(18)
                        out[0] = 0xD0
                        
                        a = 0
                        for i in range(0, 4):
                            if i == 0:
                                a = bcd(cpu) >> 12
                            elif i == 1:
                                a = bcd(cpu) >> 8
                            elif i == 2:
                                a = bcd(cpu) >> 4
                            else:
                                a = bcd(cpu)
                            
                            a &= 0x0F
                            out[2 + POS[i]] = CHARSET[a]
                            if i == 2: out[2 + POS[i]] |= 0x80

                        r = write(port, out)
                        if r is False:
                            # Not ACK'd -- get error code
                            r = port.write(b'\x23\x69\x24')
                            r = await_data(port, 1)
                            if len(r) > 0 and r[0] != 21:
                                show_error(f"Code: {int(r[0])}")
                                exit(1)
                    sleep(0.5)
            else:
                show_error("No connection to bus host")
        else:
            show_error("Could not open serial port")
    else:        
        print("Usage: python cpu_chart_segment.py /path/to/device [i2C address]")
