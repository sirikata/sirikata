#!/usr/bin/python
#
# restart_craq_cluster.py - From a workstation, handles all aspects of restarting CRAQ on the cluster.

import time
import cluster.config
import cluster.run

cc = ClusterConfig()

# FIXME: This is a giant hammer, but currently ok since we're not running any other java processes
ClusterRun('killall java')
time.sleep(5)
ClusterRun('killall java')
time.sleep(15)
ClusterRun(cc, 'cd %s/scripts/; ./cluster/craq/restart_craq_local.py'%cc.code_dir)
