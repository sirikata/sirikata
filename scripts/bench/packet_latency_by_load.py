#!/usr/bin/python

# packet_latency_by_load.py
#
# Runs a simulation multiple times with varying ping load levels and generates
# a stacked bar chart showing how time is broken down for the various stages
# at different load levels.
#
# In order to isolate issues due to routing, a configuration using only a small
# number of static objects and no querying is used.

import sys

# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

from cluster.config import ClusterConfig
from cluster.sim import ClusterSimSettings,ClusterSim
from graph.message_latency import graph_message_latency

#  8x1 configuration
#  blocksize = 100
#  objects = 200
#  bandwidth = 500000
#  static objects


def run_trial(cluster_sim):
    cluster_sim.generate_deployment()
    cluster_sim.clean_local_data()
    cluster_sim.clean_remote_data()
    cluster_sim.generate_ip_file()
    cluster_sim.run_cluster_sim()
    cluster_sim.retrieve_data()


# Runs MessageLatencyAnalysis and moves data to appropriate location
def run_message_latency_analysis(cluster_sim, log_file):
    cluster_sim.message_latency_analysis(log_file)

def get_logfile_name(trial):
    log_file = 'packet_latency_by_load.log.' + str(trial)
    return log_file

# cc - ClusterConfig
# cs - ClusterSimSettings
# ping_rate - # of pings to try to generate, i.e. load
# allow_same - allow pings to go to objects on the same server
# force_same - force pings to go to objects on the same server
def run_ping_trial(cc, cs, ping_rate, allow_same = True, force_same = False):
    cs.scenario = 'ping'
    cs.scenario_options = ' '.join(
        ['--num-pings-per-tick=' + str(ping_rate),
         '--allow-same-object-host=' + str(allow_same),
         '--force-same-object-host=' + str(force_same),
         ]
        )

    cluster_sim = ClusterSim(cc, cs)
    run_trial(cluster_sim)
    run_message_latency_analysis(cluster_sim, get_logfile_name(ping_rate))


if __name__ == "__main__":
    cc = ClusterConfig()
    cs = ClusterSimSettings(cc, 8, (8,1), 1)

    cs.loc = 'standard'
    cs.blocksize = 100
    cs.tx_bandwidth = 500000
    cs.rx_bandwidth = 500000
    cs.noise = 'false'

    cs.num_random_objects = 200
    cs.num_pack_objects = 0
    cs.object_connect_phase = '0s'

    cs.object_static = 'static'
    cs.object_query_frac = 0.0

    cs.duration = '100s'


    rates = sys.argv[1:]
    for rate in rates:
        run_ping_trial(cc, cs, rate, True, True)

    log_files = [get_logfile_name(x) for x in rates]
    graph_message_latency(log_files, 'latency_stacked_bar.pdf')
