#!/usr/bin/env python
"""Memory Readback Tool
The number of bytes you read must be a multiple of 16!
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

   
from intelhex import IntelHex
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

def decryptAES(key, iv, inBytes):
    """ Takes in a key, initialization vector, and a file location of the     input, and location of the output"""
    encryptor = AES.new(key, AES.MODE_CFB, iv,segment_size=128)
    if len(inBytes) % 16 != 0:
        block = inBytes + b'\x00'*(len(inBytes)%16)
    else:
        block = inBytes
    return encryptor.decrypt(block)


def readSecrets():
    with open("secret_configure_output.txt",'r') as keyFile:
        y = keyFile.readline()
        keyValues = {}
        while len(y) > 5:
            y = y.split(" ")
            z = y[2]
            z=z[1:-2]
            key = []
            for i in range(len(z)//4):
                    key.append(struct.pack(">B",int(z[4*i+2:4*i+4],16)))
            key = b''.join(key)
            keyValues[y[1]] = key
            y = keyFile.readline()
        return keyValues
     
secrets = readSecrets()
def construct_request(start_addr, num_bytes):
    """Construct a request frame to send the the AVR.
    The frame consists of a 24 byte password followed by the start address (4
    bytes) and then the number of bytes to read (4 bytes).
    """
    # yeah, this function depends on variables defined outside of it. Sorry. 
    PASSWORD = secrets['RB_PW'] #b'\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x20\x21\x22\x23' 
    formatstring = '>' + str(len(PASSWORD)) + 'sII'
    return struct.pack(formatstring, PASSWORD, start_addr, num_bytes)

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
    SECRET_KEY = secrets['RB_KEY']
    IV = secrets['RB_IV']
    HASH_KEY =  secrets['RBH_KEY']
    request = encryptAES(SECRET_KEY, IV, request)
    request_hash = CMACHash(HASH_KEY, request)
    request = struct.pack('>' + str(len(request)) + 's' +
        str(len(request_hash)) + 's', request, request_hash)
    # Open serial port. Set baudrate to 115200. Set timeout to 2 seconds.
    ser = serial.Serial(args.port, baudrate=115200)

    # Wait for bootloader to reset/enter readback mode.
    while ser.read(1) != 'R':
        pass

    # Send the request.
    ser.write(request)
    
    data = ser.read(int(args.num_bytes)) # read raw data
    data = decryptAES(SECRET_KEY, IV, data)
    printable = ["{:02x}".format(ord(x)) for x in data]
    print(":".join(printable))


