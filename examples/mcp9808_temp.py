#!/usr/bin/env python3

'''
IMPORTS
'''
import signal
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
    if await_data(0, timeout) == "OK":
        return True
    # Error -- No OK received
    return False


def await_data(count=0, timeout=5000):
    buffer = bytes()
    now = (time_ns() // 1000000)
    while ((time_ns() // 1000000) - now) < timeout:
        if port.in_waiting > 0:
            buffer += port.read(port.in_waiting)
            if count == 0 and len(buffer) >= 4:
                if "\r\n" in buffer.decode():
                    return buffer.decode()[0:-2]
                break
            if count != 0 and len(buffer) >= count:
                print(buffer)
                return buffer.decode()[:count]
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
            r = port.write(b'\x23\x69\x21')
            if await_ok() is True:
                r = write(b'\x23\x69\x69')
                out = bytearray(4)
                out[0] = 0x23
                out[1] = 0x69
                out[2] = 0x73
                out[3] = (i2c_address << 1)
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
