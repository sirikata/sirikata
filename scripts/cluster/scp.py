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
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.stdio
import util.invoke
from config import ClusterConfig

def append_file_if_directory(dest, source_filename):
    source_dest = dest
    if (source_dest.endswith('/')):
        return source_dest + source_filename
    if (source_dest == '.' or source_dest == '..'):
        return source_dest + '/' + source_filename
    return source_dest

# Takes a cluster config and the source and destination filename formats
def ClusterSCP(cc, params, io=util.stdio.StdIO()):
    deployment_nodes = cc.deploy_nodes
    if (deployment_nodes == None or len(deployment_nodes) == 0):
        deployment_nodes = cc.nodes

    index = 1
    for node in deployment_nodes:
        subs_dict = { 'host' : node.node, 'user' : node.user, 'node' : index }
        # Since we need to pipe through another server we need to construct a
        # sequence of commands that look like:
        #  cat src_file | ssh gateway ssh dest_server dd of=dest_file_or_dir
        # This loop does that, and collects them into a single large command
        # chained with &&'s to allow for early failure.
        scp_command = []

        sources = params[:-1]
        dest = params[-1]

        dest_remote = dest.startswith("remote:")
        if dest_remote:
            dest = dest.split(':',1)[1]

        for source in sources:
            source_remote = source.startswith("remote:")
            if source_remote:
                source = source.split(':',1)[1]

            if (source_remote and dest_remote):
                print "Source and dest cannot both be remote"
                return -1

            # extract just the filename, without preceding dirs
            source_filename = source
            where = source_filename.rfind('/')
            if where != -1:
                source_filename = source_filename[where+1:]

            # construct the destination filename
            source_dest = append_file_if_directory(dest, source_filename)

            # and generate the actual command to execute
            if (source_remote):
                # In this case we cat on the remote server and dd locally
                cmd = '(ssh ' + cc.headnode + ' ssh ' + node.str() + ' cat ' + source + ') | dd of=' + source_dest
            elif (dest_remote):
                # And here we cat locally, dd remotely
                cmd = 'cat ' + source + ' | ssh ' + cc.headnode + ' ssh ' + node.str() + ' dd of=' + source_dest
            else: # both local, just cp
                cmd = 'cp ' + source + ' ' + source_dest

            sh_command = ['sh', '-c', cmd % subs_dict]
            util.invoke.invoke(sh_command, io)

        index = index + 1


if __name__ == "__main__":

    deploy_size = int(sys.argv[1])
    file_args = sys.argv[2:]

    cc = ClusterConfig()
    cc.generate_deployment(deploy_size)
    ClusterSCP(cc, file_args)
