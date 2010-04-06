#!/usr/bin/python

import sys
import subprocess
import time
import settings # CRAQ Settings

def main():
	subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-router-32 -d meru -p 10499 -z ' + settings.CRAQ_ZOOKEEPER_ADDR, shell=True)
	subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-router-32 -d meru -p 10498 -z ' + settings.CRAQ_ZOOKEEPER_ADDR, shell=True)
        return 0

if __name__ == "__main__":
        sys.exit(main())
