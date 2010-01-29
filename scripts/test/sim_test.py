#!/usr/bin/python
#
# sim_test.py - Defines ClusterSimTest, a general interface for creating system
#               tests using the cluster/sim.py script.

import test

import sys
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.stdio
import cluster.sim

class ClusterSimTest(test.Test):
    def __init__(self, name, **kwargs):
        """
        name: Name of the test
        Others: see Test.__init__
        """

        test.Test.__init__(self, name, **kwargs)

    def run(self, io):
        intermediate_io = util.stdio.MemoryIO()

        cc = cluster.sim.ClusterConfig()
        cs = cluster.sim.ClusterSimSettings(cc, 4, (2,2), 1)
        cluster_sim = cluster.sim.ClusterSim(cc, cs, io=intermediate_io)
        cluster_sim.run()

        result = 'stdout:\n' + intermediate_io.stdout.getvalue() + '\nstderr:\n' + intermediate_io.stderr.getvalue() + '\n'
        self._store_log(result)

        # now check for warnings and errors
        failed = self._check_errors(io, result)

        return not failed
