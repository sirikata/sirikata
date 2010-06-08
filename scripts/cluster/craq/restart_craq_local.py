#!/usr/bin/python

import sys
import subprocess
import time

import start_craq_server
import start_craq_router

def main(args):
        zookeeper_node = args[1]
        zookeeper_addr = args[2]
        craq_nodes = args[3:]

        print "Starting zookeeper"
        subprocess.Popen(['./cluster/zookeeper/start_zookeeper.sh', zookeeper_node])
	print "Finished starting zookeeper"

	time.sleep(45)

        print "Starting Craqs"
        start_craq_server.main(zookeeper_addr, craq_nodes)
	print "Finished starting Craqs"

	time.sleep(45)

        print "Starting Router"
        start_craq_router.main(zookeeper_addr)
	print "Finished starting Router"

        return 0


if __name__ == "__main__":
        sys.exit(main(sys.argv))
