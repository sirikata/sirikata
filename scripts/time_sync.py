#!/usr/bin/python

import sys
import threading
import time
import subprocess
import re
import errno

# signals if we're done
done = False;
def set_done():
    global done
    done = True

# Regex to match the offset info
float_regex_str = '[-+]?[0-9]*\.?[0-9]+'
offset_regex_str = 'offset ' + float_regex_str
float_re = re.compile(float_regex_str)
offset_re = re.compile(offset_regex_str)


# monitors stdin, watching for a signal to stop
class MonitorStdInThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)

    def run(self):
        data = sys.stdin.read(1)
        if len(data) > 0:
            set_done()
        time.sleep(1.0)


# Main code
server_name = sys.argv[1]

# Monitor std in waiting for signal to stop
monitor = MonitorStdInThread()
monitor.start()

# Just loop checking and transmiting values
while (done == False):
    ntp_process = subprocess.Popen(["ntpdate", "-q", "-p", "8", server_name], stdout=subprocess.PIPE)
    output = ntp_process.communicate()[0]
    results = ntp_process.returncode

    match = offset_re.search(output)
    offset_match = float_re.search(match.group())
    try:
        print offset_match.group()
        sys.stdout.flush()
    except IOError, e:
        if e.errno != errno.EPIPE:
            # if its EPIPE, the other process closed the pipe, ignore
            # otherwise, rethrow
            raise

    time.sleep(5.0)
