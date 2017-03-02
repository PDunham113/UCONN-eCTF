#!/usr/bin/env python2
"""
Firmware Bundle-and-Protect Tool

"""
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
# grabKeys() takes the secret_build_ouput.txt file and parse it
# to acquire all secret names and secret values. The names and 
# values are stored as a hash map
def grabKeys():
    """Parses secret_build_output.txt to read in all secret names and values.
    These are stored in a dictionary. 
    """
    with open("secret_build_output.txt",'r') as keyFile:
        keyDefinition = keyFile.readline()
        keyValues = {}
        while len(keyDefinition) > 0:
            # Lines in the .h file are composed of 3 space seperated params
            # #define $VAR_NAME $VAR_VALUE
            keyDefinition = keyDefinition.split(" ")
            z = keyDefinition[2]
            
            # Cut away the " characters and \n
            z=z[1:-2]
            key = []
            for i in range(len(z)//4):
                    key.append(struct.pack(">B",int(z[4*i+2:4*i+4],16)))
            key = b''.join(key)
            keyValues[keyDefinition[1]] = key 
            keyDefinition = keyFile.readline()
        return keyValues


# This function takes a bytes object representing the HEX file
# and strips the appropriate data that nobody wants/needs.
# Returns a bytes object [data (size)]
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
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Firmware Update Tool')

    parser.add_argument("--infile",
                        help="Path to the firmware image to protect.",
                        required=True)
    parser.add_argument("--outfile", help="Filename for the output firmware.",
                        required=True)
    parser.add_argument("--version", help="Version number of this firmware.",
                        required=True)
    parser.add_argument("--message", help="Release message for this firmware.",
                        required=True)
    args = parser.parse_args()

    keyMap = grabKeys()
    # finalList will be the output data file and then encrypted and writtin
    # to outfile 
    finalList = []

    # Parse Intel hex file.
    firmwareSections = []
    with open(args.infile) as hexFile:
        for line in hexFile:
            # strip all excess data and inline everything
            firmwareSections.append(stripLine(line))

    # Get version 
    version = struct.pack(">h",int(args.version))

    # Pack version into finalList first
    finalList.append(version)
    # Pad to page end
    # finalList.append(b'\x00'*254)
    finalList.append(os.urandom(254))
    print(len(b''.join(finalList)))
    
    # Pack firmware message into finalList message
    if len(args.message) < 1024:
        finalList.append(struct.pack(">{}s".format(len(args.message)),args.message))

        # Pad to 4th page
        finalList.append(b'\x00'*(1023-len(args.message)))
        # finalList.append(os.urandom(1023-len(args.message)))
    else:
        print("Message truncated to fit in 1KB")
        tempMSG = (args.message)[:1023]
        finalList.append(struct.pack(">{}s".format(len(args.message)),tempMSG))
    
    # Enforce CSTRING encoding
    finalList.append(b'\x00')
    print("length after msg appended")

    # Pack firmware into finalList
    for line in firmwareSections:
        finalList.append(line)
    # finalList is [version (0x2)][message (1KB)][firmware (30KB)]
    finalBytes = b''.join(finalList)
    print("finalBytes size:",len(finalBytes))
    padSize = 125*256-len(finalBytes)
    if padSize < 0:
        print("firmware size error")
    finalBytes += (b'\x00'*padSize)
    # finalBytes += (os.urandom(padSize))
    
    # Encrypt bytes
    # finalBytes = encryptAES(b'\x00'*32,b'\x00'*16,finalBytes)
    finalBytes = encryptAES(keyMap["FW_KEY"],keyMap["FW_IV"],finalBytes)

    print(len(finalBytes))
    with open("CBC.hash","wb+") as CBCCypherText:
        encryptCBC(keyMap["H_KEY"],b'\x00'*16,finalBytes,CBCCypherText)
        # encryptCBC(b'\x00'*32,b'\x00'*16,finalBytes,CBCCypherText)

    CBCHash = CMACHash(keyMap["H_KEY"],finalBytes)
    # CBCHash = CMACHash(b'\x00'*32,finalBytes)
    finalBytes += CBCHash
    padSize = 126*256 - len(finalBytes)
    with open(args.outfile,'wb+') as outfile:
        outfile.write(finalBytes + b"\x06"*padSize)
#    # Add release message to end of hex (null-terminated).
#    sio = StringIO()
#    firmware.putsz(firmware_size, (args.message + '\0'))
#    firmware.write_hex_file(sio)
#    hex_data = sio.getvalue()
#
#    # Encode the data as json and write to outfile.
#    data = {
#        'firmware_size' : firmware_size,
#        'version' : version,
#        'hex_data' : hex_data
#    }
#
#    with open(args.outfile, 'wb+') as outfile:
#        data = json.dumps(data)
#        data = zlib.compress(data)
#        outfile.write(data) */
