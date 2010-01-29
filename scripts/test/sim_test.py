#!/usr/bin/python
#
# sim_test.py - Defines ClusterSimTest, a general interface for creating system
#               tests using the cluster/sim.py script.

import test

import sys
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import threading
import time

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

        sim_result = True
        if self.time_limit:
            sim_thread = threading.Thread(target=cluster_sim.run)
            sim_thread.daemon = True # allow program to exit without sim thread exiting since we can't kill it currently
            sim_thread.setDaemon(True) # Alternate version for old versions of Python
            sim_thread.start()
            time_limit_as_secs = self.time_limit.days*86400 + self.time_limit.seconds
            start_time = time.time()
            while sim_thread.isAlive():
                time.sleep(1)

                if ((time.time() - start_time) > time_limit_as_secs):
                    sim_result = False
                    print >>io.stderr, "(Time limit exceeded)",
                    io.stderr.flush()
                    break
        else:
            cluster_sim.run()

        result = 'stdout:\n' + intermediate_io.stdout.getvalue() + '\nstderr:\n' + intermediate_io.stderr.getvalue() + '\n'
        self._store_log(result)

        # now check for warnings and errors
        failed = self._check_errors(io, result) or not sim_result

        return not failed
