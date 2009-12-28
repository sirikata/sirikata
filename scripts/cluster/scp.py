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
        scp_command = []
        firstParam=True
        fileName=''
        for param in params:
            subs_param = param % subs_dict
            optionalCommand=''
            command='cat'
            if not firstParam:
                optionalCommand='of='
                command='dd'
            if (subs_param.endswith('/')):
                subs_param+=fileName;
            if (subs_param=='.'):
                subs_param=fileName;
            if (subs_param=='remote:.'):
                subs_param='remote:'+fileName;
            if (subs_param.startswith("remote:")):
                subs_param=subs_param.split(':',1)[1]
                scp_command+=['ssh',cc.headnode,'ssh',node.str(),command,optionalCommand+subs_param]
            else:
                scp_command+=[command,optionalCommand+subs_param]

            if firstParam:
                fileName=subs_param
                where=fileName.rfind('/')
                if where!=-1:
                    fileName=fileName[where+1:]
                firstParam=False
                scp_command+=['|']
        sh_command=['sh','-c',''];
        for arg in scp_command:
            if (len(sh_command[2])):
                sh_command[2]+=' '
            sh_command[2]+=arg
        #print "running ",sh_command
        subprocess.call(sh_command)
        index = index + 1


if __name__ == "__main__":

    deploy_size = int(sys.argv[1])
    file_args = sys.argv[2:]

    cc = ClusterConfig()
    cc.generate_deployment(deploy_size)
    ClusterSCP(cc, file_args)
