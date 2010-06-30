#!/usr/bin/python

# bandwidth_by_load.py
#
# Runs a simulation multiple times with varying ping load levels and
# generates graphs showing the bandwidth between pairs of servers.
#
# In order to isolate issues due to routing, a configuration using only a small
# number of static objects and no querying is used.

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
    cluster_sim.run_pre()
    cluster_sim.run_main()

# Runs bandwidth analysis and moves data for storage
def run_bandwidth_analysis(cluster_sim, histogram_file):
    cluster_sim.bandwidth_analysis()

    cluster_sim.object_latency_analysis()
    # object latency histogram always goes to 'distance_latency_histogram.csv'
    if os.path.exists('distance_latency_histogram.csv'):
        subprocess.call(['cp', 'distance_latency_histogram.csv', histogram_file])

def get_logfile_name(trial):
    log_file = 'bandwidth_by_load.log.' + str(trial)
    return log_file

def get_histogram_file_name(trial):
    log_file = 'bandwidth_by_load.histogram.' + str(trial)
    return log_file

class BandwidthByLoad:
    def __init__(self, cc, cs, local_messages=True, remote_messages=True):
        """
        cc - ClusterConfig
        cs - ClusterSimSettings
        local_messages - if True, generate messages to objects connected to the same space server
        remote_messages - if True, generate messages to objects connected to other space servers
        """
        self.cc = cc
        self.cs = cs
        self.local_messages = local_messages
        self.remote_messages = remote_messages
        self._last_rate = None
        self._all_rates = []

    def _setup_cluster_sim(self, rate, io):
        self.cs.scenario = 'ping'
        self.cs.scenario_options = ' '.join(
            ['--num-pings-per-second=' + str(rate),
             '--allow-same-object-host=' + str(self.local_messages),
             '--force-same-object-host=' + str(self.local_messages and not self.remote_messages),
             ]
            )

        if 'datagram' not in self.cs.traces['all']: self.cs.traces['all'].append('datagram')
        if 'ping' not in self.cs.traces['all']: self.cs.traces['all'].append('ping')

        cluster_sim = ClusterSim(self.cc, self.cs, io=io)
        return cluster_sim

    def run(self, rate, io=util.stdio.StdIO()):
        self._last_rate = rate
        self._all_rates.append(rate)

        cluster_sim = self._setup_cluster_sim(rate, io)
        run_trial(cluster_sim)

    def analysis(self, io=util.stdio.StdIO()):
        rate = self._last_rate
        cluster_sim = self._setup_cluster_sim(rate, io)
        run_bandwidth_analysis(cluster_sim,
                               get_histogram_file_name(rate)
                               )

if __name__ == "__main__":
    cc = ClusterConfig()
    cs = ClusterSimSettings(cc, 4, (4,1), 1)

    cs.debug = False
    cs.valgrind = False
    cs.profile = True

    cs.loc = 'standard'
    cs.blocksize = 100
    cs.tx_bandwidth = 500000
    cs.rx_bandwidth = 500000

    cs.num_random_objects = 50
    cs.num_pack_objects = 0
    cs.object_connect_phase = '0s'

    cs.object_static = 'static'
    cs.object_query_frac = 0.0

    cs.oseg_cache_entry_lifetime = '300s'

    cs.duration = '100s'

    rates = sys.argv[1:]
    plan = BandwidthByLoad(cc, cs, local_messages=False, remote_messages=True)
    for rate in rates:
        plan.run(rate)
        plan.analysis()
