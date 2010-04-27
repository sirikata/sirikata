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
from bench.flow_fairness import FlowFairness

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

        self._cc = cluster.sim.ClusterConfig()
        # Generate default settings, add user specified settings
        self._cs = cluster.sim.ClusterSimSettings(self._cc, self.space_pool, self.space_layout, self.oh_pool)
        if self.settings:
            for setname, value in self.settings.items():
                setattr(self._cs, str(setname), value)


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
    def __init__(self, name, rate, local_pings=True, remote_pings=True, payload=0, **kwargs):
        """
        name: Name of the test
        rate: Ping rate to test with
        local_pings: if True, pings to objects on the same server are generated
        remote_pings: if True, pings to objects on remote servers are generated
        payload: Size of ping payloads
        Others: see ClusterSimTest.__init__
        """
        assert type(rate) != list
        self.rate = rate

        # pre_sim_func
        if 'pre_sim_func' in kwargs:
            pre_sim_func = kwargs['pre_sim_func']
            del kwargs['pre_sim_func']
        else:
            pre_sim_func = self.__pre_sim_func

        # sim_func
        if 'sim_func' in kwargs:
            sim_func = kwargs['sim_func']
            del kwargs['sim_func']
        else:
            sim_func = self.__sim_func

        # post_sim_func
        if 'post_sim_func' in kwargs:
            post_sim_func = kwargs['post_sim_func']
            del kwargs['post_sim_func']
        else:
            post_sim_func = self.__post_sim_func

        ClusterSimTest.__init__(self, name, pre_sim_func=pre_sim_func, sim_func=sim_func, post_sim_func=post_sim_func, **kwargs)
        self.bench = PacketLatencyByLoad(self._cc, self._cs, local_messages=local_pings, remote_messages=remote_pings, payload=payload)

    def __pre_sim_func(self, cc, cs, io):
        pass

    def __sim_func(self, cc, cs, io):
        self.bench.run(self.rate, io)

    def __post_sim_func(self, cc, cs, io):
        self.bench.analysis(io)
        self.bench.graph(io)



class FlowFairnessTest(ClusterSimTest):
    def __init__(self, name, rate, scheme, payload, local=False, **kwargs):
        """
        name: Name of the test
        rate: Ping rate to test with
        scheme: Fairness scheme to use. ('region', 'csfq')
        payload: Size of ping payloads
        local: Whether only local messages should be generated, False by default
        Others: see ClusterSimTest.__init__
        """
        assert type(rate) != list
        self.rate = rate

        # pre_sim_func
        if 'pre_sim_func' in kwargs:
            pre_sim_func = kwargs['pre_sim_func']
            del kwargs['pre_sim_func']
        else:
            pre_sim_func = self.__pre_sim_func

        # sim_func
        if 'sim_func' in kwargs:
            sim_func = kwargs['sim_func']
            del kwargs['sim_func']
        else:
            sim_func = self.__sim_func

        # post_sim_func
        if 'post_sim_func' in kwargs:
            post_sim_func = kwargs['post_sim_func']
            del kwargs['post_sim_func']
        else:
            post_sim_func = self.__post_sim_func

        ClusterSimTest.__init__(self, name, pre_sim_func=pre_sim_func, sim_func=sim_func, post_sim_func=post_sim_func, **kwargs)
        self.bench = FlowFairness(self._cc, self._cs, scheme=scheme, payload=payload, local=local)

    def __pre_sim_func(self, cc, cs, io):
        pass

    def __sim_func(self, cc, cs, io):
        self.bench.run(self.rate, io)

    def __post_sim_func(self, cc, cs, io):
        self.bench.analysis(io)
        self.bench.graph(io)
