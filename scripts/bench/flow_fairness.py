#!/usr/bin/python

# flow_fairness.py
#
# Runs a simulation with objects continually messaging each other.
# The analysis then generates statistics about the actual rates
# achieved and the weights.  The output can be used to generate
# fairness graphs.

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


# Runs the FlowStats analysis and moves data to a permanent location
def run_flow_stats_analysis(cluster_sim, log_file, flowstats_file):
    cluster_sim.flow_stats_analysis(log_file)
    # individual packetx(stage-stage) samples are always in stage_samples.txt
    if os.path.exists('flow_stats.txt'):
        subprocess.call(['cp', 'flow_stats.txt', flowstats_file])

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
    log_file = 'flow_fairness.log.' + str(trial)
    return log_file

def get_latency_logfile_name(trial):
    log_file = 'flow_fairness.latency.log.' + str(trial)
    return log_file

def get_flowstats_name(trial):
    log_file = 'flow_stats.' + str(trial)
    return log_file

def get_latencyfile_name(trial):
    log_file = 'packet_latency_histogram.' + str(trial)
    return log_file

def get_stage_samples_filename(trial):
    log_file = 'packet_latency_samples.' + str(trial)
    return log_file

class FlowFairness:
    def __init__(self, cc, cs, scheme, payload=0):
        """
        Note that local vs. remote aren't options here.  We only care
        about remote messages, so we only generate remote messages.

        cc - ClusterConfig
        cs - ClusterSimSettings
        scheme - the fairness scheme used.  Current valid values are 'region' and 'csfq'.
        payload - size of ping payloads
        """
        self.cc = cc
        self.cs = cs
        self.scheme = scheme
        self.payload_size = payload
        self._last_rate = None
        self._all_rates = []

    def _setup_cluster_sim(self, rate, io):
        self.cs.scenario = 'deluge'
        self.cs.scenario_options = ' '.join(
            ['--num-pings-per-second=' + str(rate),
             '--ping-size=' + str(self.payload_size),
             ]
            )
        self.cs.odp_flow_scheduler = self.scheme

        if 'object' not in self.cs.traces: self.cs.traces.append('object')
        if 'ping' not in self.cs.traces: self.cs.traces.append('ping')
        if 'message' not in self.cs.traces: self.cs.traces.append('message')

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
        run_flow_stats_analysis(cluster_sim,
                                get_logfile_name(rate),
                                get_flowstats_name(rate)
                                )
        run_message_latency_analysis(cluster_sim,
                                     get_latency_logfile_name(rate),
                                     get_latencyfile_name(rate),
                                     get_stage_samples_filename(rate)
                                     )

    def graph(self, io=util.stdio.StdIO()):
        #log_files = [get_logfile_name(x) for x in self._all_rates]
        #labels = ['%s pps'%(x) for x in self._all_rates]
        #graph_message_latency(log_files, labels, 'latency_stacked_bar.pdf')
        #
        #samples_files = zip(
        #    [get_stage_samples_filename(x) for x in self._all_rates],
        #    [get_stage_samples_filename(x) for x in self._all_rates]
        #    )
        #samples_files = [x for x in samples_files
        #                 if os.path.exists(x[0]) and
        #                 os.path.exists(x[1])]
        #graph_stages_raw_samples(samples_files)
        pass

if __name__ == "__main__":
    cc = ClusterConfig()
    cs = ClusterSimSettings(cc, 4, (4,1), 1)

    cs.debug = False
    cs.valgrind = False
    cs.profile = True

    cs.loc = 'standard'
    cs.blocksize = 100
    cs.tx_bandwidth = 50000000
    cs.rx_bandwidth = 5000000

    cs.num_random_objects = 50
    cs.num_pack_objects = 0
    cs.object_connect_phase = '0s'

    cs.object_static = 'static'
    cs.object_query_frac = 0.0

    cs.duration = '100s'

    rates = sys.argv[1:]
    plan = FlowFairness(cc, cs, scheme='csfq', payload=1024)
    for rate in rates:
        plan.run(rate)
        plan.analysis()
    plan.graph()
