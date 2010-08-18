#!/usr/bin/python

# pinto.py
#
# Runs the simulation with objects registering queries to exercise the query handler.

import sys
import subprocess
import os.path

# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.stdio
from cluster.config import ClusterConfig
from cluster.sim import ClusterSimSettings,ClusterSim

def run_trial(cluster_sim):
    cluster_sim.generate_deployment()
    cluster_sim.clean_local_data()
    cluster_sim.clean_remote_data()
    cluster_sim.generate_ip_file()
    cluster_sim.run_cluster_sim()
    cluster_sim.retrieve_data()

class Pinto:
    def __init__(self, cc, cs):
        """
        cc - ClusterConfig
        cs - ClusterSimSettings
        """
        self.cc = cc
        self.cs = cs

    def _setup_cluster_sim(self, io):
        self.cs.scenario = 'null'

        self.cs.object_simple = 'true'
        self.cs.scenario_options = None

        cluster_sim = ClusterSim(self.cc, self.cs, io=io)
        return cluster_sim

    def run(self, io=util.stdio.StdIO()):
        cluster_sim = self._setup_cluster_sim(io)
        run_trial(cluster_sim)

    def analysis(self, io=util.stdio.StdIO()):
        cluster_sim = self._setup_cluster_sim(io)

    def graph(self, io=util.stdio.StdIO()):
        pass
