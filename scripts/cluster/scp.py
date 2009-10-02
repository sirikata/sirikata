#!/usr/bin/python

# Copies files to/from all nodes of a deployment via scp.
# Syntax: cluster_scp num_nodes file1 file2 dest
#
# For remote files, prefix with remote:, i.e.
#   cluster_scp 5 remote:file .
#
# You may use string formatting with the the field names 'host' and 'node'
# for the physical host (meru00) and the deployment node index (an integer)
# respectively.  All the formatting rules apply, so you could do something like
#   %(node)04d
# to get node indices with 4 places, like trace-0001.txt.
#
# Note that you can, of course, simply use a directory for the destination
# instead of a formatted filename.

import sys
import subprocess
from config import ClusterConfig

# Takes a cluster config and the source and destination filename formats
def ClusterSCP(cc, params):
    deployment_nodes = cc.deploy_nodes
    if (deployment_nodes == None or len(deployment_nodes) == 0):
        deployment_nodes = cc.nodes

    index = 1
    for node in deployment_nodes:
        subs_dict = { 'host' : node.node, 'user' : node.user, 'node' : index }
        scp_command = ['scp']
        for param in params:
            subs_param = param % subs_dict
            if (subs_param.startswith("remote:")):
                subs_param = node.str() + ":" + subs_param.split(':',1)[1]
            scp_command.append(subs_param)
        subprocess.call(scp_command)
        index = index + 1


if __name__ == "__main__":

    deploy_size = int(sys.argv[1])
    file_args = sys.argv[2:]

    cc = ClusterConfig()
    cc.generate_deployment(deploy_size)
    ClusterSCP(cc, file_args)
