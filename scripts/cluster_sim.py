#!/usr/bin/python

import sys
import math
import subprocess
import datetime
from cluster_config import ClusterConfig
from cluster_run import ClusterRun
from cluster_run import ClusterDeploymentRun
from cluster_scp import ClusterSCP


# User parameters
class ClusterSimSettings:
    def __init__(self, x, y):
        self.layout_x = x
        self.layout_y = y
        self.duration = '100s'
        self.tx_bandwidth = 10000
        self.rx_bandwidth = 10000
        self.flatness = .001
        self.num_objects = 100
        self.server_queue = 'fair'
        self.object_queue = 'fairfifo'

    def layout(self):
        return "<" + str(self.layout_x) + "," + str(self.layout_y) + ",1>"

class ClusterSim:
    def __init__(self, config, x, y):
        self.config = config
        self.settings = ClusterSimSettings(x, y)

    def num_servers(self):
        return self.settings.layout_x * self.settings.layout_y

    def ip_file(self):
        return "serverip-" + str(self.settings.layout_x) + '-' + str(self.settings.layout_y) + '.txt'

    def build_dir(self):
        return self.config.code_dir + "/build/cmake/"

    def scripts_dir(self):
        return self.config.code_dir + "/scripts/"

    def run(self):
        self.config.generate_deployment(self.num_servers())
        self.clean_local_data()
        self.clean_remote_data()
        self.generate_ip_file()
        self.run_instances()
        self.retrieve_data()
        self.run_analysis()

    def clean_local_data(self):
        subprocess.call(['rm', '-rf', 'trace*'])
        subprocess.call(['rm', '-rf', 'sync*'])
        subprocess.call(['rm', '-rf', 'serverip*'])

    def clean_remote_data(self):
        clean_cmd = "cd " + self.build_dir() + "; rm trace*; rm sync*; rm serverip*;"
        ClusterRun(self.config, clean_cmd)

    def generate_ip_file(self):
        # Generate the serverip file, copy it to all nodes
        serveripfile = self.ip_file()
        fp = open(serveripfile,'w')
        port = self.config.port_base
        server_index = 0;
        for i in range(0, self.settings.layout_x):
            for j in range(0, self.settings.layout_y):
                fp.write(self.config.deploy_nodes[server_index].node+":"+str(port)+'\n')
                port += 1
                server_index += 1
        fp.close()
        ClusterSCP(self.config, [serveripfile, "remote:" + self.build_dir()])

    def run_instances(self):
        wait_until_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S%z")
        # Construct the command lines to do the actual run and dispatch them
        cmd = "cd " + self.build_dir() + "; "
        cmd += "./cbr "
        cmd += "--id=%(node)d "
        cmd += "\"--layout=" + self.settings.layout() + "\" "
        cmd += "--serverips=" + self.ip_file() + " "
        cmd += "--duration=" + self.settings.duration + " "
        cmd += "--send-bandwidth=" + str(self.settings.tx_bandwidth) + " "
        cmd += "--receive-bandwidth=" + str(self.settings.rx_bandwidth) + " "
        cmd += "--wait-until=" + "\"" + wait_until_time + "\" "
        cmd += "--wait-additional=6 "
        cmd += "--flatness=" + str(self.settings.flatness) + " "
        cmd += "--capexcessbandwidth=false "
        cmd += "--objects=" + str(self.settings.num_objects) + " "
        cmd += "--server.queue=" + self.settings.server_queue + " "
        cmd += "--object.queue=" + self.settings.object_queue + " "
        ClusterDeploymentRun(self.config, cmd)

    def retrieve_data(self):
        # Copy the trace and sync data back here
        trace_file_pattern = "remote:" + self.build_dir() + "trace-%(node)04d.txt"
        sync_file_pattern = "remote:" + self.build_dir() + "sync-%(node)04d.txt"
        ClusterSCP(self.config, [trace_file_pattern, sync_file_pattern, "."])

    def run_analysis(self):
        # Run analysis
        subprocess.call(['../build/cmake/cbr', '--id=1', "--layout=" + self.settings.layout(), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.windowed-bandwidth=packet', '--analysis.windowed-bandwidth.rate=100ms'])
        subprocess.call(['../build/cmake/cbr', '--id=1', "--layout=" + self.settings.layout(), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.windowed-bandwidth=datagram', '--analysis.windowed-bandwidth.rate=100ms'])

        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_packet_send.dat'])
        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_packet_receive.dat'])
        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_datagram_send.dat'])
        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_datagram_receive.dat'])

        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_send_packet.dat'])
        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_receive_packet.dat'])
        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_send_datagram.dat'])
        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_receive_datagram.dat'])



if __name__ == "__main__":
    cc = ClusterConfig()
    cluster_sim = ClusterSim(cc, 2, 2)
    cluster_sim.run()
