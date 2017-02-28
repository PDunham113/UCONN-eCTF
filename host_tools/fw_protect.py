#!/usr/bin/env python
"""
Firmware Bundle-and-Protect Tool

"""
import argparse
import shutil
import struct
import json
import zlib

from cStringIO import StringIO
from intelhex import IntelHex

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

    # Parse Intel hex file.
    firmware = IntelHex(args.infile)

    # Get version and size.
    firmware_size = firmware.maxaddr() + 1
    version = int(args.version)

    # Add release message to end of hex (null-terminated).
    sio = StringIO()
    firmware.putsz(firmware_size, (args.message + '\0'))
    firmware.write_hex_file(sio)
    hex_data = sio.getvalue()

    # Encode the data as json and write to outfile.
    data = {
        'firmware_size' : firmware_size,
        'version' : version,
        'hex_data' : hex_data
    }

    with open(args.outfile, 'wb+') as outfile:
        data = json.dumps(data)
        data = zlib.compress(data)
        outfile.write(data)

