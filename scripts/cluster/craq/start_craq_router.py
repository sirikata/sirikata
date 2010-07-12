#!/usr/bin/python

import sys
import subprocess
import time
import socket

import os

def main(zookeeper_addr):
	subprocess.Popen(os.environ.get('HOME')+'/bmistree/new-craq-dist/craq-router-32 -d meru -p 10499 -i ' + socket.gethostbyname(socket.gethostname())  + ' -z ' + zookeeper_addr, shell=True)
	subprocess.Popen(os.environ.get('HOME')+'/bmistree/new-craq-dist/craq-router-32 -d meru -p 10498 -i ' + socket.gethostbyname(socket.gethostname())  + ' -z ' + zookeeper_addr, shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
