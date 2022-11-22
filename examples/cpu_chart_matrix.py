#!/usr/bin/env python3

'''
IMPORTS
'''
import signal
from psutil import cpu_percent
from sys import exit, argv
from time import sleep, time_ns, monotonic_ns


'''
GLOBALS
'''
port = None


'''
FUNCTIONS
'''
def await_ok():
    return await_data() == "OK"


def await_data(count=0, timeout=5000):
    buffer = bytes()
    then = monotonic_ns() // 1000000
    now = then
    while now - then < timeout:
        if port.in_waiting > 0:
            buffer += port.read(1)
            if count == 0 and len(buffer) > 3:
                if buffer.decode().endswith("\r\n"):
                    return buffer.decode()[:-2]
            if count > 0 and len(buffer) >= count:
                return buffer.decode()[:count]
        now = monotonic_ns() // 1000000
    # Error -- Timeout
    return buffer.decode()


def await_ack(timeout=2000):
    buffer = bytes()
    then = monotonic_ns() // 1000000
    now = then
    while now - then < timeout:
        if port.in_waiting > 0:
            r = port.read(1)
            return r[0] == 0x0F
        now = monotonic_ns() // 1000000
    # Error -- Timeout
    return False


def write(data):
    port.write(data)
    return await_ack()


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
        port.write(b'\x78')
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
            r = port.write(b'\x21')
            if await_ok() is True:
                write(b'\x69')
                out = bytearray(2)
                out[0] = 0x73
                out[1] = i2c_address << 1
                write(out)
                write(b'\xC0\x21')
                write(b'\xC0\x81')
                write(b'\xC0\xE2')
                
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

                    # Write out matrix buffer
                    if write(out) is False:
                        # Not ACK'd -- get error code
                        port.write(b'\x24')
                        err = await_data(1)
                        if len(err) > 0: show_error(f"Code: {err[0]:02x}")
                        show_error("Lost contact with Bus Host")

                    sleep(0.5)
            else:
                show_error("No connection to bus host")
        else:
            show_error("Could not open serial port")
    else:        
        print("Usage: python cpu_chart_matrix.py /path/to/device [i2C address]")
        