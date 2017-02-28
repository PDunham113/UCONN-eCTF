
# coding: utf-8

# In[2]:

import os
import random
import shutil
import subprocess
import sys

from intelhex import IntelHex

from Crypto.Cipher import AES
from Crypto.Random.random import StrongRandom
import os, struct

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
def AESHash(key,iv,input_file,output_file):
    encryptor = AES.new(key,AES.MODE_CBC,iv,segment_size=128)
    with open(input_file, 'rb') as infile:
        with open(output_file,'wb') as outfile:
            output = None
            while True:
                block = infile.read(16)
                if len(block) == 0:
                    break
                elif len(block) % 16 != 0:
                    block += b'\x00'* (16 - len(block)%16)
                output = encryptor.encrypt(block)
            outfile.write(output)
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
FILE_DIR = os.path.abspath(os.path.dirname(__file__))

def make_bootloader(password=None):
    """
    Build the bootloader from source.
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
    """
    Copy bootloader build artifacts into the host tools directory.
    """
    # Get directory containing this file (host_tools).
    dst_dir = FILE_DIR

    # Get directory containing bootloader output (bootloader).
    src_dir = os.path.join(os.path.dirname(dst_dir), 'bootloader')

    # Copy build artifacts from bootloader directory.
    shutil.copy(os.path.join(src_dir, 'flash.hex'), dst_dir)
    shutil.copy(os.path.join(src_dir, 'eeprom.hex'), dst_dir)

def generate_readback_password():
    """
    Generate secret password for readback tool and store to secret file.
    """
    # Generate 16 character random password.
    pw = ''.join(chr(random.randint(0, 255)) for i in range(8)).encode('hex')

    # Write password to secret file.
    with open('secret_build_output.txt', 'wb+') as secret_build_output:
        secret_build_output.write(pw)

    return pw

def write_fuse_file(fuse_name, fuse_value):
    hex_file = IntelHex()
    hex_file[0] = fuse_value

    with open(os.path.join(FILE_DIR, fuse_name + '.hex'), 'wb+') as outfile:
        hex_file.tofile(outfile, format='hex')

def generate_secrets():
    FW_KEY = generate256Entropy()
    RB_KEY = generate256Entropy()
    H_KEY = generate256Entropy()
    FW_IV = generate128Entropy()
    RB_IV = generate128Entropy()
    H_IV = generate128Entropy()
    RB_PW = os.random(24)
    all_keys_and_ivs = [("FW_KEY", FW_KEY),
                    ("RB_KEY",RB_KEY),
                    ("H_KEY", H_KEY),
                    ("FW_IV", FW_IV),
                    ("RB_IV",RB_IV),
                    ("H_IV",H_IV),
                    ("RB_PW",RB_PW)]
    return all_keys_and_ivs

def make_secrets_file(secrets):
    with open('secret_build_output.txt', 'w') as secret_file:
        for data in secrets:
            secret_file.write("#DEFINE " + data[0] + " \"" + bytesToCString(data[1])+'\"')
            secret_file.write("\n")
    

if __name__ == '__main__':
    secrets = generate_secrets()
    make_secrets_file(secrets)
    password = generate_readback_password()
    if not make_bootloader(password=password):
        print "ERROR: Failed to compile bootloader."
        sys.exit(1)
    copy_artifacts()
    write_fuse_file('lfuse', 0xFF)
    write_fuse_file('hfuse', 0x18)
    write_fuse_file('efuse', 0xFC)


# In[ ]:



