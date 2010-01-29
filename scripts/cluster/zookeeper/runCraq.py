#!/usr/bin/python

# 
#
#
#


def main():
        print "Hello World! \n\n  Beginning program"
        subprocess.Popen('/home/meru/bmistree/new-craq-dist/craq-32 -d meru -p 10333 -z 192.168.1.30:9888', shell=True)

        return 0



if __name__ == "__main__":

        import sys
        import subprocess
	import time
        sys.exit(main())
