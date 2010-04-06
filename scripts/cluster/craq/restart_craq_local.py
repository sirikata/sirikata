#!/usr/bin/python

import sys
import subprocess
import time

def main():
        print "Starting zookeeper"
        subprocess.Popen('./cluster/zookeeper/start_zookeeper.sh', shell=True)
	print "Finished starting zookeeper"

	time.sleep(45)

        print "Starting Craqs"
        subprocess.Popen('./cluster/craq/start_craq_server.py', shell=True)
	print "Finished starting Craqs"

	time.sleep(45)

        print "Starting Router"
        subprocess.Popen('./cluster/craq/start_craq_router.py', shell=True)
	print "Finished starting Router"

        return 0


if __name__ == "__main__":
        sys.exit(main())
