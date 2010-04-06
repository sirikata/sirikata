#!/usr/bin/python

import sys
import subprocess
import time
import socket

def main():
        if (not socket.gethostname() in ['meru27', 'meru29', 'meru30']):
            return 0

        subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-32 -d meru -p 10333 -z 192.168.1.7:9888', shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
