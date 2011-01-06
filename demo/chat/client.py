#!/usr/bin/env python

import sys
import subprocess
import signal
import time
import chatoptions

chatoptions.parse(sys.argv[1:])

oh = chatoptions.run(['../../build/cmake/cppoh_d', '--cfg=chat.cfg', '--servermap-options=--host=%s --port=%d' % (chatoptions.host, chatoptions.port)], chatoptions.debug)
signalled = False

def handle_int(signum, frame):
    global signalled
    global oh

    if signalled: return

    signalled = True
    oh.send_signal(signal.SIGINT)
    oh.wait()
    sys.exit(0)

signal.signal(signal.SIGINT, handle_int)
signal.signal(signal.SIGTERM, handle_int)
signal.signal(signal.SIGABRT, handle_int)
signal.signal(signal.SIGHUP, handle_int)

oh.wait()
