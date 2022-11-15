#!/usr/bin/env python3

'''
IMPORTS
'''
from os import path
from psutil import cpu_percent
from sys import exit, argv
from time import sleep, time_ns


'''
FUNCTIONS
'''
def await_ok(uart, timeout=2000):
    if await_data(uart, timeout) == "OK":
        return True
    # Error -- No OK received
    return False


def await_data(uart, timeout=2000):
    buffer = bytes()
    now = (time_ns() // 1000000)
    while ((time_ns() // 1000000) - now) < timeout:
        if uart.in_waiting > 0:
            buffer += uart.read(uart.in_waiting)
            if len(buffer) == 4:
                if "\r\n" in buffer.decode():
                    return buffer.decode()[0:-2]
                break
    # Error == No data received (or mis-formatted)
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


'''
RUNTIME START
'''
if __name__ == '__main__':

    device = None
    i2c_address = 0x60
    col = 0
    cols = [0,0,0,0,0,0,0,0,0,0]

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
            port = serial.Serial(port=device, baudrate=500000)
        except:
            show_error(f"An invalid device file specified: {device}")

        if port:
            # Check we can connect
            out = b'\x23\x69\x21'
            r = port.write(out)
            if await_ok(port) is True:
                r = port.write(b'\x23\x69\x69')
                out = bytearray(4)
                out[0] = 0x23
                out[1] = 0x69
                out[2] = 0x73
                out[3] = (i2c_address << 1)
                r = write(port, out)
                r = write(port, b'\xC1\x00\x18')
                r = write(port, b'\xC1\x0D\x0E')
                r = write(port, b'\xC1\x19\x40')
                r = write(port, b'\xC1\x0C\x00')
                
                while True:
                    cpu = int(cpu_percent())

                    if col > 9:
                        del cols[0]
                        cols.append(cpu)
                    else:
                        cols[9 - col] = cpu
                    col += 1

                    for i in range(0, 10):
                        if i == 0 or i == 5:
                            out = bytearray(9)
                            if i == 0: out[0] = 0x0E
                            if i == 5: out[0] = 0x01
                        
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
                        out[1 + i] = b

                        if i == 4 or i == 9:
                            r = write(port, out)
                            if r is False:
                                # Not ACK'd -- get error code
                                r = port.write(b'\x23\x69\x24')
                                r = await_data(port, 1)
                                if r[0] != 21:
                                    show_error(f"Code: {int(r[0])}")
                                    exit(1)
                    
                    out = b'\xC1\x0C\x00'
                    r = port.write(out)
                    await_ack(port)
                    sleep(0.5)
            else:
                show_error("No connection to bus host")
        else:
            show_error("Could not open serial port")
    else:        
        print("Usage: python cpu_chart_matrix_ltp305.py {device} {i2C address}")
        