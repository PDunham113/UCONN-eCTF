
# coding: utf-8

# In[ ]:

"""
Firmware Updater Tool
A frame consists of two sections:
1. Two bytes for the length of the data section
2. A data section of length defined in the length section
[ 0x02 ]  [ variable ]
--------------------
| Length | Data... |
--------------------
In our case, the data is from one line of the Intel Hex formated .hex file
We write a frame to the bootloader, then wait for it to respond with an
OK message so we can write the next frame. The OK message in this case is
just a zero
"""

import argparse
import json
import serial
import struct
import sys
import zlib
import time

from intelhex import IntelHex

RESP_OK = b'\x06'


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Firmware Update Tool')

    parser.add_argument("--port", help="Serial port to send update over.",
                        required=True)
    parser.add_argument("--firmware", help="Path to firmware image to load.",
                        required=True)
    parser.add_argument("--debug", help="Enable debugging messages.",
                        action='store_true')
    args = parser.parse_args()

    # Open serial port. Set baudrate to 115200. Set timeout to 2 seconds.
    print('Opening serial port...')
    ser = serial.Serial(args.port, baudrate=115200, timeout=2)

    print('Waiting for bootloader to enter update mode...')
    while ser.read(1) != 'U':
        pass

    with open(args.firmware, 'rb') as firmware:
        chunk = firmware.read(256)
        i = 0
        while (len(chunk)!=0):
            if args.debug:
                print("Writing frame {} ({} bytes)...".format(i, len(chunk)))
            ser.write(b'\x00'*256)
            # ser.write(chunk)  # Write the frame...

            resp = ser.read()  # Wait for an OK from the bootloader

            time.sleep(0.1)

            if resp != RESP_OK:
                raise RuntimeError("ERROR: Bootloader responded with {}".format(repr(resp)))

            i+=1
            chunk = firmware.read(256)
    print("Done writing firmware.")

    # Send a zero length payload to tell the bootlader to finish writing
    # it's page.
    ser.write(struct.pack('>H', 0x0000))

