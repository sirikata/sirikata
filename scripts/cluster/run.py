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
from config import ClusterConfig
#xforward='-Y'
xforward='-T'
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

        buf = buf + self.sp.stdout.read(1024*1024)
        for line in buf.splitlines():
            print self.node, ': ', line
        self.sp.wait()

    def returncode(self):
        return self.sp.returncode


# Performs substitution of per-node parameters.
# command - may be either string or array of strings containing commands with substitution string
# host - the host the command will be run on
# user - the user to run as
# index - the index of the node in the cluster deployment config
# user_params - a dict of user specified substitutions
# return value - same type and form as command (string or array), with substitutions performed
def ClusterSubstitute(command, host, user, index=0, user_params=None):
    # Construct substitution dictionary
    subs = {'host' : host, 'user' : user, 'node' : index}
    if (user_params != None):
        for key,value in user_params.items():
            subs[key] = value[index-1]

    # Perform substituion
    if (isinstance(command, str)):
        return command % subs
    elif (isinstance(command, list)):
        return [(x % subs) for x in command]
    return None


# Runs the given command on all nodes of the cluster, dumps output to stdout, and waits until all connections to nodes have
# been closed.
def ClusterRun(cc, command):
    new_environ = os.environ
    new_environ['DISPLAY'] = ':0.0'
    new_environ['XAUTHORITY'] = new_environ['HOME'] + '/.Xauthority'

    mts = []
    for node in cc.nodes:
        node_command = ClusterSubstitute(command, host=node.node, user=node.user)
        (outp,inp)=os.pipe();
        os.write(inp,node_command)
        os.close(inp)
        #f.seek(0);
        sp = subprocess.Popen(['ssh',xforward,cc.headnode,'ssh '+xforward+' '+node.str()+' sh'], 0, None, outp, subprocess.PIPE, subprocess.STDOUT, None, False, False, None, new_environ)
        mt = NodeMonitorThread(node.str(), sp)
        mt.start()
        mts.append(mt)
        time.sleep(.1)
    for mt in mts:
        mt.join()

    return [mt.returncode() for mt in mts]

# Runs the given command on all nodes of the cluster, dumps output to stdout, and waits until all connections to nodes have
# been closed.
# The user_params dictionary is a user specified set of substitutions to make, in the form of a dictionary whose values are arrays, each with a number of elements equal to the number of deployment node
def ClusterDeploymentRun(cc, command, user_params = None):
    new_environ = os.environ
    new_environ['DISPLAY'] = ':0.0'
    new_environ['XAUTHORITY'] = new_environ['HOME'] + '/.Xauthority'

    mts = []
    index = 1
    for node in cc.deploy_nodes:
        node_command = ClusterSubstitute(command, host=node.node, user=node.user, index=index, user_params=user_params)
        (outp,inp)=os.pipe();
        os.write(inp,node_command)
        os.close(inp)
        
        sp = subprocess.Popen(['ssh',xforward,cc.headnode,'ssh '+xforward+' '+node.str()+' sh'], 0, None, outp, subprocess.PIPE, subprocess.STDOUT, None, False, False, None, new_environ)
        mt = NodeMonitorThread(str(index) + " (" + node.str() + ")", sp)
        mt.start()
        mts.append(mt)
        time.sleep(.1)
        index = index + 1

    for mt in mts:
        mt.join()

    return [mt.returncode() for mt in mts]


def ClusterRunConcatCommands(commands):
    filtered = [x for x in commands if len(x.strip()) > 0]
    return " && ".join(filtered)

def ClusterRunFailed(returned):
    for rec in returned:
        if (rec != 0):
            return True
    return False

def ClusterRunSummaryCode(returned):
    if (ClusterRunFailed(returned)):
        return -1
    return 0

def main():
    if (len(sys.argv) != 2):
        print "Syntax: cluster_run.py \"my; commands;\""
        exit(-1)

    command = sys.argv[1]

    cc = ClusterConfig()
    results = ClusterRun(cc, command)
    if (ClusterRunFailed(results)):
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(main())
