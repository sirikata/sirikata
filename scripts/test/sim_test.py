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
from bench.packet_latency_by_load import PacketLatencyByLoad

def __standard_pre_sim_func__(cc, cs, io):
    cluster_sim = cluster.sim.ClusterSim(cc, cs, io)
    cluster_sim.run_pre()

def __standard_sim_func__(cc, cs, io):
    cluster_sim = cluster.sim.ClusterSim(cc, cs, io)
    cluster_sim.run_main()

def __standard_post_sim_func__(cc, cs, io):
    cluster_sim = cluster.sim.ClusterSim(cc, cs, io)
    cluster_sim.run_analysis()

class ClusterSimTest(test.Test):
    def __init__(self, name, settings=None, space_pool=None, space_layout=(4,1), oh_pool=1, pre_sim_func=__standard_pre_sim_func__, sim_func=__standard_sim_func__, post_sim_func=__standard_post_sim_func__, **kwargs):
        """
        name: Name of the test
        settings: A dict of settings in ClusterSimSettings to override from the defaults, e.g. { 'duration' : '100s' }
        space_pool: Number of space servers to allocate and run, if None it is set to match the number needed by space_layout
        space_layout: 2-tuple specifying 2D layout of servers
        oh_pool: Number of object hosts to allocate and run
        pre_sim_func: Function to invoke to run the pre-simulation setup, of the form f(cluster_config, cluster_settings, io=StdIO_obj)
        sim_func: Function to invoke to run the simulation, of the form f(cluster_config, cluster_settings, io=StdIO_obj)
        post_sim_func: Function to invoke to run the post-simulation cleanup and analysis, of the form f(cluster_config, cluster_settings, io=StdIO_obj)
        Others: see Test.__init__
        """

        test.Test.__init__(self, name, **kwargs)
        self.settings = settings
        self.space_pool = space_pool
        self.space_layout = space_layout
        self.oh_pool = oh_pool
        self.pre_sim_func = pre_sim_func
        self.sim_func = sim_func
        self.post_sim_func = post_sim_func

        self._cc = None
        self._cs = None

        if not self.space_pool:
            self.space_pool = int(self.space_layout[0]) * int(self.space_layout[1])

    def __collect_output_and_check(self, io, store):
        """
        Collect intermediate IO and check it for errors, returning
        True if it has no flagged errors, False if it does.  If store
        is true, the output will be stored to the log file.
        """

        result = 'stdout:\n' + self._intermediate_io.stdout.getvalue() + '\nstderr:\n' + self._intermediate_io.stderr.getvalue() + '\n'
        if store: self._store_log(result)

        # now check for warnings and errors
        failed = self._check_errors(io, result)
        return not failed

    def pre_run(self, io):
        self._intermediate_io = util.stdio.MemoryIO()

        self._cc = cluster.sim.ClusterConfig()
        # Generate default settings, add user specified settings
        self._cs = cluster.sim.ClusterSimSettings(self._cc, self.space_pool, self.space_layout, self.oh_pool)
        if self.settings:
            for setname, value in self.settings.items():
                setattr(self._cs, str(setname), value)

        self.pre_sim_func(self._cc, self._cs, io=self._intermediate_io)
        return self.__collect_output_and_check(io, store=True)

    def run(self, io):
        target_func = lambda: self.sim_func(self._cc, self._cs, io=self._intermediate_io)
        sim_result = True
        if self.time_limit:
            sim_thread = threading.Thread(target=target_func)
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
            target_func()

        return self.__collect_output_and_check(io, store=True) or sim_result


    def post_run(self, io):
        self.post_sim_func(self._cc, self._cs, io=self._intermediate_io)
        return self.__collect_output_and_check(io, store=True)



class PacketLatencyByLoadTest(ClusterSimTest):
    def __init__(self, name, rates, local_pings=True, remote_pings=True, **kwargs):
        """
        name: Name of the test
        rates: List of ping rates to test with
        local_pings: if True, pings to objects on the same server are generated
        remote_pings: if True, pings to objects on remote servers are generated
        Others: see ClusterSimTest.__init__
        """
        if 'sim_func' in kwargs:
            ClusterSimTest.__init__(self, name, **kwargs)
        else:
            sim_func = (lambda cc,cs,io: PacketLatencyByLoad(cc,cs,rates,local_messages=local_pings,remote_messages=remote_pings,io=io))
            ClusterSimTest.__init__(self, name, sim_func=sim_func, **kwargs)
