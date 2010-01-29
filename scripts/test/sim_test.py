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
    def __init__(self, name, settings=None, space_pool=4, space_layout=(4,1), oh_pool=1, **kwargs):
        """
        name: Name of the test
        settings: A dict of settings in ClusterSimSettings to override from the defaults, e.g. { 'duration' : '100s' }
        Others: see Test.__init__
        """

        test.Test.__init__(self, name, **kwargs)
        self.settings = settings
        self.space_pool = space_pool
        self.space_layout = space_layout
        self.oh_pool = oh_pool

    def run(self, io):
        intermediate_io = util.stdio.MemoryIO()

        cc = cluster.sim.ClusterConfig()

        # Generate default settings, add user specified settings
        cs = cluster.sim.ClusterSimSettings(cc, self.space_pool, self.space_layout, self.oh_pool)
        if self.settings:
            for setname, value in self.settings.items():
                setattr(cs, str(setname), value)

        cluster_sim = cluster.sim.ClusterSim(cc, cs, io=intermediate_io)
        cluster_sim.run()

        result = 'stdout:\n' + intermediate_io.stdout.getvalue() + '\nstderr:\n' + intermediate_io.stderr.getvalue() + '\n'
        self._store_log(result)

        # now check for warnings and errors
        failed = self._check_errors(io, result)

        return not failed
