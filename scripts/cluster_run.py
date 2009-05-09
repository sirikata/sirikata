#!/usr/bin/python

# Runs a command (or series of commands) on all cluster nodes via ssh.
# This is a maintainence command, not intended for use over a deployment.
# This means that the command will only be run once per node, not once
# per deployment node.
#
# The syntax of the command is simply
#   cluster_run.py "My; set; of; commands"
#
# You can use string formatting with the field 'host' in the command to insert
# the node name into the command.  For example, the command could be
# "run_something.py %(node)" to pass the node name as a parameter.

import sys
import subprocess
import threading
import os
import time
from cluster_config import ClusterConfig


class NodeMonitorThread(threading.Thread):
    def __init__(self, node, sp):
        threading.Thread.__init__(self)
        self.node = node
        self.sp = sp

    def run(self):
        buf = ""
        while( self.sp.returncode == None ):
            self.sp.poll()
            buf = buf + self.sp.stdout.read(16)

            buflines = buf.splitlines(True)
            for line in buflines[:-1]:
                print self.node, ': ', line.strip()
            if (len(buflines) > 0):
                buf = buflines[-1]

            sys.stdout.flush()

        print self.node, ': ', buf.strip()
        self.sp.wait()


# Runs the given command on all nodes of the cluster, dumps output to stdout, and waits until all connections to nodes have
# been closed.
def ClusterRun(cc, command):
    new_environ = os.environ
    new_environ['DISPLAY'] = ':0.0'
    new_environ['XAUTHORITY'] = new_environ['HOME'] + '/.Xauthority'

    mts = []
    for node in cc.nodes:
        node_command = command % {'host' : node}
        sp = subprocess.Popen(['ssh', '-Y', '-l' , cc.user, node, node_command], 0, None, None, subprocess.PIPE, subprocess.STDOUT)
        mt = NodeMonitorThread(node, sp)
        mt.start()
        mts.append(mt)
        time.sleep(.1)

    for mt in mts:
        mt.join()

# Runs the given command on all nodes of the cluster, dumps output to stdout, and waits until all connections to nodes have
# been closed.
def ClusterDeploymentRun(cc, command):
    new_environ = os.environ
    new_environ['DISPLAY'] = ':0.0'
    new_environ['XAUTHORITY'] = new_environ['HOME'] + '/.Xauthority'

    mts = []
    index = 1
    for node in cc.deploy_nodes:
        node_command = command % {'node' : index}
        print node_command
        sp = subprocess.Popen(['ssh', '-Y', '-l' , cc.user, node, node_command], 0, None, None, subprocess.PIPE, subprocess.STDOUT, None, False, False, None, new_environ)
        mt = NodeMonitorThread(str(index) + " (" + node + ")", sp)
        mt.start()
        mts.append(mt)
        time.sleep(.1)
        index = index + 1

    for mt in mts:
        mt.join()



if __name__ == "__main__":
    if (len(sys.argv) != 2):
        print "Syntax: cluster_run.py \"my; commands;\""
        exit(-1)

    command = sys.argv[1]

    cc = ClusterConfig()
    ClusterRun(cc, command)
