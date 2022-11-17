#!/usr/bin/env python3

'''
IMPORTS
'''
import signal
from psutil import cpu_percent
from sys import exit, argv
from time import sleep, time_ns


'''
GLOBALS
'''
port = None


'''
FUNCTIONS
'''
def await_ok(timeout=2000):
    if await_data(timeout) == "OK":
        return True
    # Error -- No OK received
    return False


def await_data(timeout=2000):
    buffer = bytes()
    now = (time_ns() // 1000000)
    while ((time_ns() // 1000000) - now) < timeout:
        if port.in_waiting > 0:
            buffer += port.read(port.in_waiting)
            if len(buffer) == 4:
                if "\r\n" in buffer.decode():
                    return buffer.decode()[0:-2]
                break
    # Error -- No data received (or mis-formatted)
    return ""


def await_ack(timeout=2000):
    buffer = bytes()
    now = (time_ns() // 1000000)
    while ((time_ns() // 1000000) - now) < timeout:
        if port.in_waiting > 0:
            r = port.read(1)
            if r[0] == 0x0F:
                return True
    # Error -- No Ack received
    return False


def write(data, timeout=2000):
    l = port.write(data)
    return await_ack(timeout)


def show_error(message):
    print("[ERROR]", message)
    if port: port.close()
    exit(1)


def str_to_int(num_str):
    num_base = 10
    if num_str[:2] == "0x": num_base = 16
    try:
        return int(num_str, num_base)
    except ValueError:
        return -1


def handler(signum, frame):
    if port:
        # Reset the host's I2C bus
        port.write(b'\x23\x69\x78')
        port.close()
        print("\nDone")
    exit(0)

'''
RUNTIME START
'''
if __name__ == '__main__':

    signal.signal(signal.SIGINT, handler)
    
    device = None
    i2c_address = 0x70
    col = 0
    cols = [0,0,0,0,0,0,0,0]

    if len(argv) > 1:
        device = argv[1]

    if len(argv) > 2:
        i2c_address = str_to_int(argv[2])
        if i2c_address == -1:
            show_error("Invalid I2C address")
        print(f"Using device at 0x{i2c_address:2X}")

    if device:
        # Set the port or fail
        try:
            import serial
            port = serial.Serial(port=device, baudrate=1000000)
        except:
            show_error(f"An invalid device file specified: {device}")

        if port:
            # Check we can connect
            r = port.write(b'\x23\x69\x21')
            if await_ok() is True:
                write(b'\x23\x69\x69')
                out = bytearray(4)
                out[0] = 0x23
                out[1] = 0x69
                out[2] = 0x73
                out[3] = (i2c_address << 1)
                write(out)
                write(b'\xC0\x21')
                write(b'\xC0\x81')
                write( b'\xC0\xE2')
                
                while True:
                    cpu = int(cpu_percent())

                    if col > 7:
                        del cols[0]
                        cols.append(cpu)
                    else:
                        cols[7 - col] = cpu
                    col += 1

                    out = bytearray(18)
                    out[0] = 0xD0
                    out[1] = 0x00
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
                        out[2 + (i * 2)] = (b >> 1) + ((b << 7) & 0xFF)

                    r = write(out)
                    if r is False:
                        # Not ACK'd -- get error code
                        port.write(b'\x23\x69\x24')
                        r = await_data(1)
                        if len(r) > 0:
                            show_error(f"Code: {int(r[0])}")
                        show_error("Lost contact with Bus Host")

                    sleep(0.5)
            else:
                show_error("No connection to bus host")
        else:
            show_error("Could not open serial port")
    else:        
        print("Usage: python cpu_chart_matrix.py /path/to/device [i2C address]")
        