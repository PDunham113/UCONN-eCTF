
# coding: utf-8

# In[96]:

from Crypto.Cipher import AES
from Crypto.Random.random import StrongRandom
import os, struct

def encryptFileAES(key, iv, input_file, output_file):
    """ Takes in a key, initialization vector, and a file location of the input, and location of the output"""
    encryptor = AES.new(key, AES.MODE_CFB, iv)
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
    decryptAES = AES.new(key, AES.MODE_CFB, iv)
    with open(input_file, 'rb') as infile:
        with open(output_file, 'wb') as outfile:
            while True:
                chunk = infile.read(16)
                if len(chunk)==0:
                    break
                outfile.write(decryptAES.decrypt(chunk))

def generate128Key():
    return os.urandom(16)    

