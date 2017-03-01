#!/usr/bin/env python
"""
Bootloader Configuration Tool
This tool is responsible for configuring the bootloader after it is programmed.
"""
import argparse
import os
import serial
import shutil

FILE_PATH = os.path.abspath(__file__)

def generate_secret_file():
    """
    Compile all secrets from build and configuration and store to secret file.
    """
    # Get directory containing this file (host_tools).
    directory = os.path.dirname(FILE_PATH)

    # Copy secret build output to secret configure output.
    shutil.copyfile(os.path.join(directory, 'secret_build_output.txt'),
                    os.path.join(directory, 'secret_configure_output.txt'))

    # If there were additional secret parameters to output, the file could be
    # edited here.



def configure_bootloader(serial_port):
    """
    Configure bootloader using serial connection.
    """
    # If there were online configuration or checking of the bootloader using
    # the serial port it could be added here.

    while serial_port.read(1) != 'C':
        pass

    keyValPairs = grabKeys()
    # Send an ACK to the device.
    serial_port.write('\x06') # return ACK
    
    # Waits and reads Hash into object buf.
    buf = serial_port.read(16)

    comp = keyValPairs["flashHash"] 
    check = True;
    for i in range(0, len(buf)):
        if buf[i] != comp[i]:
            check = False;
    print(''.join(['{}'.format(x) for x in buf]))
    if check == True:
        serial_port.write('\x06')
    else:
        serial_port.write('\x15')
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
if __name__ == '__main__':
    # Argument parser setup.
    parser = argparse.ArgumentParser(description='Bootloader Config Tool')
    parser.add_argument('--port', help='Serial port to use for configuration.',
                        required=True)
    args = parser.parse_args()
    # Create serial connection using specified port.
    serial_port = serial.Serial(args.port)

    # Do configuration and then close port.
    try:
        configure_bootloader(serial_port)
    finally:
        serial_port.close()

    # Generate secret file.
    generate_secret_file()
