#!/usr/bin/python

import sys
import subprocess
import time
import socket
import settings # CRAQ Settings

def main():
        if (not socket.gethostname() in settings.CRAQ_CHAIN_NODES):
            return 0

        subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-32 -d meru -p 10333 -z ' + settings.CRAQ_ZOOKEEPER_ADDR, shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
