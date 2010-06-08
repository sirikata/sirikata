#!/usr/bin/python
#
# restart_craq_cluster.py - From a workstation, handles all aspects of restarting CRAQ on the cluster.

import sys
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/../..")

import time
from cluster.config import ClusterConfig
from cluster.run import ClusterRun

cc = ClusterConfig()

# FIXME: This is a giant hammer, but currently ok since we're not running any other java processes
ClusterRun(cc, 'killall java')
time.sleep(5)
ClusterRun(cc, 'killall java')
time.sleep(15)
ClusterRun(cc, 'cd %s/scripts/; ./cluster/craq/restart_craq_local.py %s %s %s' % (cc.code_dir, cc.zookeeper, cc.zookeeper_addr, ' '.join(cc.craq_nodes)) )
