#!/usr/bin/python

import sys
# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

from sim_test import ClusterSimTest
from bench.pinto import Pinto

class PintoTest(ClusterSimTest):
    def __init__(self, name, **kwargs):
        """
        name: Name of the test
        Others: see ClusterSimTest.__init__
        """
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
        self.bench = Pinto(self._cc, self._cs)

    def __pre_sim_func(self, cc, cs, io):
        pass

    def __sim_func(self, cc, cs, io):
        self.bench.run(io)

    def __post_sim_func(self, cc, cs, io):
        self.bench.analysis(io)
        self.bench.graph(io)
