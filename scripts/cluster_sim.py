#!/usr/bin/python

import sys
import math
import subprocess
from cluster_config import ClusterConfig
from cluster_run import ClusterRun
from cluster_run import ClusterDeploymentRun
from cluster_scp import ClusterSCP

# User parameters
layout_x = 3 # FIXME
layout_y = 3 # FIXME

duration = '100s'
tx_bandwidth = 10000
rx_bandwidth = 10000
flatness = .001
num_objects = 100
server_queue = 'fair'
object_queue = 'fairfifo'


nservers = layout_x * layout_y

cc = ClusterConfig()
cc.generate_deployment(nservers)

#ClusterRun(cc, "cd ewencp/cbr.git/build/cmake; cmake .; make clean; make -j2")


# Generate the serverip file, copy it to all nodes
serveripfile = "serverip-" + str(layout_x) + '-' + str(layout_y) + '.txt'
fp = open(serveripfile,'w')
port = 6666
server_index = 0;
for i in range(0, layout_x):
    for j in range(0, layout_y):
        fp.write(cc.deploy_nodes[server_index].node+":"+str(port)+'\n')
        port += 1
        server_index += 1
fp.close()
ClusterSCP(cc, [serveripfile, "remote:ewencp/cbr.git/build/cmake/"])


# Construct the command lines to do the actual run and dispatch them
cmd_fmt = "cd ewencp/cbr.git/build/cmake; "
cmd_fmt += "./cbr "
cmd_fmt += "\"--layout=<" + str(layout_x)+","+str(layout_y)+",1>\" "
cmd_fmt += "--serverips=" + serveripfile + " "
cmd_fmt += " --duration=%(duration)s --send-bandwidth=%(sbandwidth)d --receive-bandwidth=%(rbandwidth)d --wait-additional=6 --flatness=%(flatness)f --capexcessbandwidth=false --objects=%(numobjects)d --server.queue=%(squeue)s --object.queue=%(oqueue)s "
cmd = cmd_fmt % { 'numsq' : nservers, 'duration' : duration, 'sbandwidth' : tx_bandwidth, 'rbandwidth' : rx_bandwidth, 'flatness' : flatness, 'numobjects' : num_objects, 'squeue' : server_queue, 'oqueue' : object_queue}
cmd = cmd + "--id=%(node)d "
print cmd
ClusterDeploymentRun(cc, cmd)

# Copy the trace and sync data back here
ClusterSCP(cc, ["remote:ewencp/cbr.git/build/cmake/trace-%(node)04d.txt", "remote:ewencp/cbr.git/build/cmake/sync-%(node)04d.txt", "."])

# Run analysis
subprocess.call(['../build/cmake/cbr', '--id=1', "--layout=<" + str(layout_x)+","+str(layout_y)+",1>", "--serverips=" + serveripfile, "--duration=100s", '--analysis.windowed-bandwidth=packet', '--analysis.windowed-bandwidth.rate=100ms'])
subprocess.call(['../build/cmake/cbr', '--id=1', "--layout=<" + str(layout_x)+","+str(layout_y)+",1>", "--serverips=" + serveripfile, "--duration=100s", '--analysis.windowed-bandwidth=datagram', '--analysis.windowed-bandwidth.rate=100ms'])

subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_packet_send.dat'])
subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_packet_receive.dat'])
subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_datagram_send.dat'])
subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_datagram_receive.dat'])
