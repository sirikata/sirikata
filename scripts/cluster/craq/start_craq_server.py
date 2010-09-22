#!/usr/bin/python

import sys
import subprocess
import time
import socket

import os

def main(zookeeper_addr, craq_nodes):
        if (not socket.gethostname() in craq_nodes):
            return 0

        subprocess.Popen(os.environ.get('HOME')+'/bmistree/new-craq-dist/craq-32 -d meru -i ' + socket.gethostbyname(socket.gethostname())  + ' -p 10333 -z ' + zookeeper_addr, shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
