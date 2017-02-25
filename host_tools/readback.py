#!/usr/bin/env python
"""Memory Readback Tool

A frame consists of four sections:
1. One byte for the length of the password.
2. The variable-length password.
3. Four bytes for the start address.
4. Four bytes for the number of bytes to read.

  [ 0x01 ]  [ variable ]  [ 0x04 ]    [ 0x04 ]
-------------------------------------------------
| PW Length | Password | Start Addr | Num Bytes |
-------------------------------------------------
"""

import serial
import struct
import sys
import argparse

RESP_OK = b'\x00'
RESP_ERROR = b'\x01'

def construct_request(start_addr, num_bytes):
    """Construct a request frame to send the the AVR.
    """
    # Read in secret password from file.
    SECRET_PASSWORD = ''
    secret = 'secret_configure_output.txt'
    try:
        with open(secret, 'rb') as secret_file:
            SECRET_PASSWORD = secret_file.read()
            # remove those pesky newlines
            SECRET_PASSWORD = SECRET_PASSWORD.rstrip()
    except:
        print("File not found")
        exit()
    formatstring = '>' + str(len(SECRET_PASSWORD)) + 'sII'
    return struct.pack(formatstring, SECRET_PASSWORD, start_addr, num_bytes)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Memory Readback Tool')

    parser.add_argument("--port", help="Serial port to send update over.",
                        required=True)
    parser.add_argument("--address", help="First address to read from.",
                        required=True)
    parser.add_argument("--num-bytes", help="Number of bytes to read.",
                        required=True)
    parser.add_argument("--datafile", help="File to write data to (optional).")
    args = parser.parse_args()

    request = construct_request(int(args.address), int(args.num_bytes))

    # Open serial port. Set baudrate to 115200. Set timeout to 2 seconds.
    ser = serial.Serial(args.port, baudrate=115200, timeout=2)

    # Wait for bootloader to reset/enter readback mode.
    while ser.read(1) != 'R':
        pass

    # Send the request.
    ser.write(request)

    # Read the data and write it to stdout (hex encoded).
    data = ser.read(int(args.num_bytes))
    print(data.encode('hex'))

    # Write raw data to file (optional).
    if args.datafile:
        with open(args.datafile, 'wb+') as datafile:
            datafile.write(data)
