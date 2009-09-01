#!/usr/bin/python

import sys
import threading
import time
import subprocess
import re
import errno

# Settings
ntp_interval = 30.0
heartbeat_interval = 60

# signals if we're done
done = False;
def set_done():
    global done
    done = True

last_heartbeat = time.time()
def heartbeat():
    global last_heartbeat
    last_heartbeat = time.time()

def check_heartbeat():
    global last_heartbeat
    global heartbeat_interval
    if (time.time() - last_heartbeat > heartbeat_interval):
        sys.stderr.write('Heartbeat signal lost\n')
        sys.exit(-1)

# Regex to match the offset info
float_regex_str = '[-+]?[0-9]*\.?[0-9]+'
offset_regex_str = 'offset ' + float_regex_str
float_re = re.compile(float_regex_str)
offset_re = re.compile(offset_regex_str)


# monitors stdin, watching for a signal to stop
class MonitorStdInThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.daemon = True

    def run(self):
        try:
            data = sys.stdin.read(1)
            if len(data) == 0:
                sys.stderr.write('Monitor read error\n')
                sys.exit(-1)
            elif (data == 'h'):
                heartbeat()
            elif (data == 'k'):
                set_done()
        except IOError, e:
            if e.errno == errno.EPIPE:
                sys.stderr.write('Broken monitor pipe\n')
                sys.exit(-1)

class MonitorHeartbeatThread(threading.Thread):
    def __init__(self):
        threading.Thread.__init__(self)
        self.daemon = True

    def run(self):
        while(True):
            check_heartbeat()
            time.sleep(heartbeat_interval)

# Main code
server_name = sys.argv[1]

# Monitor std in waiting for signal to stop
monitor = MonitorStdInThread()
monitor.start()

heartbeat()
monitor_heartbeat = MonitorHeartbeatThread()
monitor_heartbeat.start()

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
        if e.errno == errno.EPIPE:
            set_done()
            break
        elif e.errno != errno.EPIPE:
            # if its EPIPE, the other process closed the pipe, ignore
            # otherwise, rethrow
            raise

    time.sleep(ntp_interval)

sys.exit(0)
