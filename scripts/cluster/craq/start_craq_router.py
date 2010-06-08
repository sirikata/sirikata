#!/usr/bin/python

import sys
import subprocess
import time

def main(zookeeper_addr):
	subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-router-32 -d meru -p 10499 -z ' + zookeeper_addr, shell=True)
	subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-router-32 -d meru -p 10498 -z ' + zookeeper_addr, shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
