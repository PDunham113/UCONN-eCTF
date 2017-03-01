#!/usr/bin/env python2

import os
import struct
import random
import shutil
import subprocess
import sys
from intelhex import IntelHex
from Crypto.Cipher import AES
from Crypto.Random.random import StrongRandom

def grabKeys():
    with open("secret_build_output.txt",'r') as keyFile:
        keyDefinition = keyFile.readline()
        keyValues = {}
        while len(keyDefinition) > 0:
            keyDefinition = keyDefinition.split(" ")
            z = keyDefinition[2]
            z=z[1:-2]
            key = []
            for i in range(len(z)//4):
                    key.append(struct.pack(">B",int(z[4*i+2:4*i+4],16)))
            key = b''.join(key)
            keyValues[keyDefinition[1]] = key 
            keyDefinition = keyFile.readline()
        return keyValues

def CMACHash(key,inBytes):
    encryptor = AES.new(key,AES.MODE_CBC,b'\x00'*16,segment_size=128)
    if len(inBytes) % 16 != 0:
        block = inBytes + b'\x00'*(len(inBytes)%16)
    else:
        block = inBytes
    output = (encryptor.encrypt(block))
    return output[-16:]

def bytesToHexList(data):
    #data_new = ["{:x}".format(x) for x in data]
    data_new = ["{:02x}".format(ord(x)) for x in data]
    return ["\\x{}".format(b) for b in data_new]

def bytesToCString(data):
    return "".join(bytesToHexList(data))

def generate128Entropy():
    return os.urandom(16) 

def generate256Entropy():
    return os.urandom(32) 

FILE_DIR = os.path.abspath(os.path.dirname(__file__))

def make_bootloader(password=None):
    """Build the bootloader from source.
    Return:
        True if successful, False otherwise.
    """
    # Change into directory containing bootloader.
    os.chdir('../bootloader')

    subprocess.call('make clean', shell=True)
    # Call make in subprocess to build bootloader.
    if password is not None:
        status = subprocess.call('make PASSWORD="%s"' % password, shell=True)
    else:
        status = subprocess.call('make')

    # Return True if make returned 0, otherwise return False.
    return (status == 0)

def copy_artifacts():
    """Copy bootloader build artifacts into the host tools directory.
    """
    # Get directory containing this file (host_tools).
    dst_dir = FILE_DIR

    # Get directory containing bootloader output (bootloader).
    src_dir = os.path.join(os.path.dirname(dst_dir), 'bootloader')

    # Copy build artifacts from bootloader directory.
    shutil.copy(os.path.join(src_dir, 'flash.hex'), dst_dir)
    shutil.copy(os.path.join(src_dir, 'eeprom.hex'), dst_dir)

def write_fuse_file(fuse_name, fuse_value):
    hex_file = IntelHex()
    hex_file[0] = fuse_value

    with open(os.path.join(FILE_DIR, fuse_name + '.hex'), 'wb+') as outfile:
        hex_file.tofile(outfile, format='hex')

def generate_secrets():
    """Generate the keys and Password.
    """
    FW_KEY = generate256Entropy()
    RB_KEY = generate256Entropy()
    H_KEY = generate256Entropy()
    RBH_KEY = generate256Entropy()
    VERIFY_KEY = generate256Entropy()
    FW_IV = generate128Entropy()
    RB_IV = generate128Entropy()
    RB_PW = os.urandom(24)
    all_keys_and_ivs = [("FW_KEY", FW_KEY),
                    ("RB_KEY",RB_KEY),
                    ("H_KEY", H_KEY),
                    ("FW_IV", FW_IV),
                    ("RB_IV",RB_IV),
                    ("RBH_KEY",RBH_KEY),
                    ("VERIFY_KEY",VERIFY_KEY),
                    ("RB_PW",RB_PW)]
    return all_keys_and_ivs

def make_secrets_file(secrets):
    """Construct secret_build_output.txt with all of the necessary keys and
    passwords.
    """
    with open('secret_build_output.txt', 'w') as secret_file:
        for data in secrets:
            secret_file.write("#define " + data[0] + " \"" + bytesToCString(data[1])+'\"')
            secret_file.write("\n")

def stripLine(intelLine):
    """Pulls the data out of a line of Intel hex and packs it as bytes.
    """
    intLine = intelLine[9:-3]
    if len(intLine) == 0:
        return b'' 
    # if it is a data line
    if intelLine[7:9] == "00":
        final = b'' 
        for i in range(len(intLine)//2):
            final += struct.pack(">B",int(intLine[(2*i):(2*i+2)],16))
        return struct.pack(">{}s".format(len(intLine)//2),final)
    # if it is not a data line
    else:
        return b''

def addrOf(intHexLine):
    """Extract the address bytes from an intel hex line.
    """
    return int(intHexLine[3:7],16)

def dataLen(intHexLine):
    """Extract the length of the true data section to be safe.
    """
    return len(intHexLine[9:-3])//2

def genMem(intHex):
    with open(intHex,'r') as hexFile:
        dump = []
        prevAddr = -16
        prevLen = 16
        bytesADDED = 0
        for currentLine in hexFile:
            recordType = currentLine[7:9]
            if recordType != '00':
                pass
            else:
                # Extend the line to its appropriate length.
                # this won't append any lines if the lines line up well
                # for i in range(addrOf(currentLine)-prevLen-prevAddr):
                #     # ff is default value of eeprom and flash
                #     dump.append(b'\xff')
                #     bytesADDED =+ 1
                prevLen = addrOf(currentLine)-prevAddr
            
                if prevAddr + prevLen == addrOf(currentLine):
                    # If the last line is now continuos append it's data as is
                    dump.append(stripLine(currentLine))
                    # Initialize values for previous line as current line
                    prevAddr = addrOf(currentLine)
                    prevLen = dataLen(currentLine)
            prevLine = currentLine
        return b''.join(dump)

if __name__ == '__main__':
#     secrets = generate_secrets()
#     make_secrets_file(secrets)
#     if not make_bootloader():
#         print "ERROR: Failed to compile bootloader."
#         sys.exit(1)
    secrets = grabKeys()
    bootFlash = genMem("flash.hex")
    print(len(bootFlash))
    bootFlash += b'\xff'*((7936) - len(bootFlash))
    print("\n")
    print(len(bootFlash))
    flashHash = CMACHash(secrets["H_KEY"],bootFlash)
    # flashHash = CMACHash(secrets[2][1],bootFlash)
#     bootEeprom = genMen("eeprom.hex")
#     bootEeprom += b'\xff' * ((3584) - len(bootFlash))
#     eepromHash = CMACHash(secrets,secrets[2][1])
    with open("secret_build_output.txt","a") as secFile:
        secFile.write("#define flashHash " + "\"" + bytesToCString(flashHash) + "\"")
#         secFile.write("#define eepromHash " + "\"" + eepromHash + "\"")
    copy_artifacts()
    write_fuse_file('lfuse', 0xFF)
    write_fuse_file('hfuse', 0x18)
    write_fuse_file('efuse', 0xFC)
