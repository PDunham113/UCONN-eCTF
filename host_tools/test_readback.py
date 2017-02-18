#!/usr/bin/env python

import readback
import unittest

class TestReadback(unittest.TestCase):
    def test_construct_request(self):
        output = ":".join(['%0X' % ord(b) for b in
        readback.construct_request(0x11, 22)])
        print(output)

if __name__ == '__main__':
    unittest.main()
