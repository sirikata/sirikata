#!/usr/bin/python

# 
#
#
#


def main():
        print "\nStarting zookeeper\n\n"
        subprocess.Popen('sh cluster/zookeeper/runZookeeperScript.sh', shell=True)
	print "\nFinished starting zookeeper\n\n"
	
	time.sleep(45)

        print "\nStarting Craqs\n\n"
        subprocess.Popen('python cluster/zookeeper/runCraq.py', shell=True)
	print "\nFinished starting Craqs\n\n"

	time.sleep(45)

        print "\nStarting Router\n\n"
        subprocess.Popen('python cluster/zookeeper/runRouter.py', shell=True)
	print "\nFinished starting Router\n\n"
	
	
        return 0



if __name__ == "__main__":

        import sys
        import subprocess
	import time
        sys.exit(main())
