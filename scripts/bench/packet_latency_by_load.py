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
import subprocess
import os.path

# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

import util.stdio
from cluster.config import ClusterConfig
from cluster.sim import ClusterSimSettings,ClusterSim
from graph.message_latency import graph_message_latency
from graph.stages import graph_stages_raw_samples

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
def run_message_latency_analysis(cluster_sim, log_file, histogram_file, samples_file):
    cluster_sim.message_latency_analysis(log_file)
    # individual packetx(stage-stage) samples are always in stage_samples.txt
    if os.path.exists('stage_samples.txt'):
        subprocess.call(['cp', 'stage_samples.txt', samples_file])

    cluster_sim.object_latency_analysis()
    # object latency histogram always goes to 'distance_latency_histogram.csv'
    if os.path.exists('distance_latency_histogram.csv'):
        subprocess.call(['cp', 'distance_latency_histogram.csv', histogram_file])

def get_logfile_name(trial):
    log_file = 'packet_latency_by_load.log.' + str(trial)
    return log_file

def get_latencyfile_name(trial):
    log_file = 'packet_latency_histogram.' + str(trial)
    return log_file

def get_stage_samples_filename(trial):
    log_file = 'packet_latency_samples.' + str(trial)
    return log_file

# cc - ClusterConfig
# cs - ClusterSimSettings
# ping_rate - # of pings to try to generate, i.e. load
# local_messages - if True, generate messages to objects connected to the same space server
# remote_messages - if True, generate messages to objects connected to other space servers
def run_ping_trial(cc, cs, ping_rate, local_messages=True, remote_messages=True, io=util.stdio.StdIO()):
    cs.scenario = 'ping'
    cs.scenario_options = ' '.join(
        ['--num-pings-per-second=' + str(ping_rate),
         '--allow-same-object-host=' + str(local_messages),
         '--force-same-object-host=' + str(local_messages and not remote_messages),
         ]
        )

    cluster_sim = ClusterSim(cc, cs, io=io)
    run_trial(cluster_sim)
    run_message_latency_analysis(cluster_sim,
                                 get_logfile_name(ping_rate),
                                 get_latencyfile_name(ping_rate),
                                 get_stage_samples_filename(ping_rate)
                                 )


# cc - ClusterConfig
# cs - ClusterSimSettings
# rates - array of ping rates to test
# local_messages - if True, generate messages to objects connected to the same space server
# remote_messages - if True, generate messages to objects connected to other space servers
def PacketLatencyByLoad(cc, cs, rates, local_messages=True, remote_messages=True, io=util.stdio.StdIO()):
    for rate in rates:
        run_ping_trial(cc, cs, rate, local_messages=local_messages, remote_messages=remote_messages, io=io)

    log_files = [get_logfile_name(x) for x in rates]
    labels = ['%s pps'%(x) for x in rates]
    graph_message_latency(log_files, labels, 'latency_stacked_bar.pdf')

    samples_files = zip(
        [get_stage_samples_filename(x) for x in rates],
        [get_stage_samples_filename(x) for x in rates]
        )
    samples_files = [x for x in samples_files
                     if os.path.exists(x[0]) and
                     os.path.exists(x[1])]
    graph_stages_raw_samples(samples_files)


if __name__ == "__main__":
    cc = ClusterConfig()
    cs = ClusterSimSettings(cc, 8, (8,1), 1)

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

    cs.duration = '100s'


    rates = sys.argv[1:]
    PacketLatencyByLoad(cc, cs, rates, local_messages=True, remote_messages=True)
