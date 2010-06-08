#!/usr/bin/python

import sys
import subprocess
import time
import socket

def main(zookeeper_addr, craq_nodes):
        if (not socket.gethostname() in craq_nodes):
            return 0

        subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-32 -d meru -p 10333 -z ' + zookeeper_addr, shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
