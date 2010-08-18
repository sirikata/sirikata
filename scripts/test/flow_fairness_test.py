#!/usr/bin/python
#
# sim_test.py - Defines ClusterSimTest, a general interface for creating system
#               tests using the cluster/sim.py script.

import sys
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

from sim_test import ClusterSimTest
from bench.flow_fairness import FlowFairness

class FlowFairnessTest(ClusterSimTest):
    def __init__(self, name, rate, scheme, payload, local=False, oh_pool=1, genpack=False, packname='flow.pack', **kwargs):
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

        if (genpack): oh_pool = 1

        ClusterSimTest.__init__(self, name, pre_sim_func=pre_sim_func, sim_func=sim_func, post_sim_func=post_sim_func, oh_pool=oh_pool, **kwargs)

        nobjects = self._cs.num_random_objects

        if (genpack):
            # Pack generation, run with 1 oh
            assert(self._cs.num_oh == 1)
            self._cs.num_random_objects = nobjects
            self._cs.num_pack_objects = 0
            self._cs.object_pack = ''
            self._cs.pack_dump = packname
        elif (oh_pool > 1):
            # Use pack across multiple ohs
            self._cs.num_random_objects = 0
            self._cs.num_pack_objects = nobjects / self._cs.num_oh
            self._cs.object_pack = packname
            self._cs.pack_dump = ''

        self.bench = FlowFairness(self._cc, self._cs, scheme=scheme, payload=payload, local=local)

    def __pre_sim_func(self, cc, cs, io):
        pass

    def __sim_func(self, cc, cs, io):
        self.bench.run(self.rate, io)

    def __post_sim_func(self, cc, cs, io):
        self.bench.analysis(io)
        self.bench.graph(io)
