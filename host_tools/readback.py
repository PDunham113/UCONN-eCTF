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
import SecretsAndBytes
import os
import random
import shutil
import subprocess

from intelhex import IntelHex

from Crypto.Cipher import AES
from Crypto.Random.random import StrongRandom
import os, struct

# inlined code
def bytesToHexList(data):
    #data_new = ["{:x}".format(x) for x in data]
    data_new = ["{:02x}".format(x) for x in data]
    return ["\\x{}".format(b) for b in data_new]
def bytesToCString(data):
    return "".join(bytesToHexList(data))
def encryptFileAES(key, iv, input_file, output_file):
    """ Takes in a key, initialization vector, and a file location of the input, and location of the output"""
    encryptor = AES.new(key, AES.MODE_CFB, iv,segment_size=128)
    with open(input_file, 'rb') as infile:
        with open(output_file, 'wb') as outfile:
            while True:
                chunk = infile.read(16)
                if len(chunk) == 0:
                    break
                elif len(chunk) % 16 != 0:
                    chunk += b' ' * (16-len(chunk)%16)
                outfile.write(encryptor.encrypt(chunk))
def decryptFileAES(key, iv, input_file, output_file):
    """ Takes in a key, initialization vector, and a file location of the input, and location of the output"""
    decryptAES = AES.new(key, AES.MODE_CFB, iv,segment_size=128)
    with open(input_file, 'rb') as infile:
        with open(output_file, 'wb') as outfile:
            while True:
                chunk = infile.read(16)
                if len(chunk)==0:
                    break
                outfile.write(decryptAES.decrypt(chunk))

def generate128Entropy():
    return os.urandom(16) 
def generate256Entropy():
    return os.urandom(32) 
# end inlined code

# copy pasted encrypt function modified to not write to file
def encryptStringAES(key, iv, input_string):
    """ Takes in a key, initialization vector, and a file location of the input, and location of the output"""
    outString = ""
    encryptor = AES.new(key, AES.MODE_CFB, iv,segment_size=128)
        for i in range(len(input_string) // 16):
            chunk = infile.read(16)
                if len(chunk) == 0:
                    break
                elif len(chunk) % 16 != 0:
                    chunk += b' ' * (16-len(chunk)%16)
            outString += encryptor.encrypt(chunk)
    return outString

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
    SECRET_PASSWORD = readFile('secret_configure_output.txt')
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
    SECRET_KEY = readFile('secret_readback_key.txt')
    IV = readFile('secret_initialization_vector.txt')
    request = encryptStringAES(SECRET_KEY, IV, request)

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
