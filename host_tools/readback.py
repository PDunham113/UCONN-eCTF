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
import random
import argparse
import shutil
import struct
import json
import zlib
import os
import random
from Crypto.Cipher import AES 
from Crypto.Random.random import StrongRandom

# Check the following file for byte manipulation functions

from intelhex import IntelHex
# This function takes a bytes object representing the HEX file
# and strips the appropriate data that nobody wants/needs.
# Returns a bytes object [size (0x2)][addr (0x4)][data (size)]
def stripLine(intelLine):
    # intLine = intelLine[1:7] + intelLine[9:-3]
    intLine = intelLine[9:-3]
    if len(intLine) == 0:
        return b''
    if intelLine[7:9] == "00":
        final = b''
        for i in range(len(intLine)//2):
            final += struct.pack(">B",int(intLine[(2*i):(2*i+2)],16))
        return struct.pack(">{}s".format(len(intLine)//2),final)
    else:
        return b''
    # return (int(intLine,16)).to_bytes(len(intLine),byteorder='big')
def CMACHash(key,inBytes):
    encryptor = AES.new(key,AES.MODE_CBC,b'\x00'*16,segment_size=128)
    if len(inBytes) % 16 != 0:
        block = inBytes + b'\x00'*(len(inBytes)%16)
    else:
        block = inBytes
    output = (encryptor.encrypt(block))
    return output[-16:]
def encryptCBC(key, iv, inBytes,outfile):
    """ Takes in a key, initialization vector, and a file location of the     input, and location of the output"""
    encryptor = AES.new(key, AES.MODE_CBC, iv,segment_size=128)
    if len(inBytes) % 16 != 0:
        block = inBytes + b'\x00'*(len(inBytes)%16)
    else:
        block = inBytes
    outfile.write(encryptor.encrypt(block))
def encryptAES(key, iv, inBytes):
    """ Takes in a key, initialization vector, and a file location of the     input, and location of the output"""
    encryptor = AES.new(key, AES.MODE_CFB, iv,segment_size=128)
    if len(inBytes) % 16 != 0:
        block = inBytes + b'\x00'*(len(inBytes)%16)
    else:
        block = inBytes
    return encryptor.encrypt(block)


def readFromFile(filename):
    # Read in secret key from file.
    try:
        with open(filename, 'rb') as secret_file:
            contents = secret_file.read()
            # remove those pesky newlines
            return contents.rstrip()
    except:
        print("File not found")
        exit()

RESP_OK = b'\x00'
RESP_ERROR = b'\x01'

def construct_request(start_addr, num_bytes):
    """Construct a request frame to send the the AVR.
    """
    SECRET_PASSWORD = "000000000000000000000000"
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
    # Read in secret key from file.
    SECRET_KEY = "0000000000000000"
    IV = "0000000000000000"
    request = encryptAES(SECRET_KEY, IV, request)
    request_hash = CMACHash(SECRET_KEY, request)
    request = struct.pack('>' + str(len(request)) + 's' +
        str(len(request_hash)) + 's', request, request_hash)
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
