#!/usr/bin/env python

import sys
import subprocess
import signal
import time
import chatoptions

chatoptions.parse(sys.argv[1:])

space = subprocess.Popen(['../../build/cmake/space_d', '--servermap-options=--host=%s --port=%d' % (chatoptions.host, chatoptions.port)])
time.sleep(2) # Ensure the space server has time to get up and running
oh = subprocess.Popen(['../../build/cmake/cppoh_d', '--cfg=chat-static.cfg', '--servermap-options=--host=%s --port=%d' % (chatoptions.host, chatoptions.port)])

def handle_int(signum, frame):
    oh.send_signal(signal.SIGINT)
    space.send_signal(signal.SIGINT)

    sys.exit(0)

signal.signal(signal.SIGINT, handle_int)
signal.signal(signal.SIGTERM, handle_int)
signal.signal(signal.SIGABRT, handle_int)
signal.signal(signal.SIGHUP, handle_int)

while True:
    time.sleep(100)
