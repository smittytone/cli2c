#!/usr/bin/env python3

'''
IMPORTS
'''
import signal
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
    i2c_address = 0x18

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
                r = write(b'\x69')
                out = bytearray(2)
                out[0] = 0x73
                out[1] = i2c_address << 1
                r = write(out)
                
                while True:
                    write(b'\xC0\x05')
                    port.write(b'\x81')
                    result = port.read(2)
                    
                    # Convert the raw value to a Celsius reading
                    temp_raw = (result[0] << 8) | result[1]
                    temp_col = (temp_raw & 0x0FFF) / 16.0
                    if temp_raw & 0x10000: temp_col -= 256.0
                    print(" Current temperature: {:.2f}°C\r".format(temp_col), end="")
                    sleep(1)
            else:
                show_error("No connection to bus host")
        else:
            show_error("Could not open serial port")
    else:        
        print("Usage: python mcp9808_temp.py /path/to/device [i2C address]")
