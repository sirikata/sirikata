#!/usr/bin/python

import sys
import math
import subprocess
import datetime
from cluster_config import ClusterConfig
from cluster_run import ClusterRun
from cluster_run import ClusterDeploymentRun
from cluster_scp import ClusterSCP

CBR_WRAPPER = "./cbr_wrapper.sh"

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
        self.server_queue_length = 8192
        self.object_queue = 'fairfifo'
        self.object_queue_length = 8192
        self.object_static = 'false'
        self.object_simple = 'true'
        self.object_2d = 'true'
        self.object_global = 'false'
        self.noise = 'false'
        self.debug = True
        self.loc = 'oracle'
        self.blocksize = 100

    def layout(self):
        return "<" + str(self.layout_x) + "," + str(self.layout_y) + ",1>"

    def region(self):
        half_extents = [ self.blocksize * x / 2 for x in (self.layout_x, self.layout_y, 1)]
        return "<<" + str(-half_extents[0]) + "," + str(-half_extents[1]) + "," + str(-half_extents[2]) + ">,<" + str(half_extents[0]) + "," + str(half_extents[1]) + "," + str(half_extents[2]) + ">>"

class ClusterSim:
    def __init__(self, config, x, y):
        self.config = config
        self.settings = ClusterSimSettings(x, y)

    def num_servers(self):
        return self.settings.layout_x * self.settings.layout_y

    def ip_file(self):
        return "serverip-" + str(self.settings.layout_x) + '-' + str(self.settings.layout_y) + '.txt'

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

    def vis(self):
        subprocess.call([CBR_WRAPPER,
                         '--id=1',
                         "--layout=" + self.settings.layout(),
                         "--region=" + self.settings.region(),
                         "--serverips=" + self.ip_file(),
                         "--duration=" + self.settings.duration,
                         '--analysis.locvis=server',
                         "--object.static=" + self.settings.object_static,
                         "--object.simple=" + self.settings.object_simple,
                         "--object.2d=" + self.settings.object_2d
                         ])

    def clean_local_data(self):
        subprocess.call(['rm -f trace*'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f sync*'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f serverip*'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f *.ps'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f *.dat'], 0, None, None, None, None, None, False, True)

    def clean_remote_data(self):
        clean_cmd = "cd " + self.scripts_dir() + "; rm trace*; rm sync*; rm serverip*;"
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
        ClusterSCP(self.config, [serveripfile, "remote:" + self.scripts_dir()])

    def run_instances(self):
        wait_until_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S%z")
        # Construct the command lines to do the actual run and dispatch them
        cmd = "cd " + self.scripts_dir() + "; "
        cmd += CBR_WRAPPER + " "
        if (self.settings.debug):
            cmd += "--debug "
        cmd += "--id=%(node)d "
        cmd += "\"--layout=" + self.settings.layout() + "\" "
        cmd += "\"--region=" + self.settings.region() + "\" "
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
        cmd += "--server.queue.length=" + str(self.settings.server_queue_length) + " "
        cmd += "--object.queue=" + self.settings.object_queue + " "
        cmd += "--object.queue.length=" + str(self.settings.object_queue_length) + " "
        cmd += "--object.static=" + self.settings.object_static + " "
        cmd += "--object.simple=" + self.settings.object_simple + " "
        cmd += "--object.2d=" + self.settings.object_2d + " "
        cmd += "--object.global=" + self.settings.object_global + " "
        cmd += "--noise=" + self.settings.noise + " "
        cmd += "--loc=" + self.settings.loc + " "
        ClusterDeploymentRun(self.config, cmd)

    def retrieve_data(self):
        # Copy the trace and sync data back here
        trace_file_pattern = "remote:" + self.scripts_dir() + "trace-%(node)04d.txt"
        sync_file_pattern = "remote:" + self.scripts_dir() + "sync-%(node)04d.txt"
        ClusterSCP(self.config, [trace_file_pattern, sync_file_pattern, "."])

    def run_analysis(self):
        # Run analysis
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.windowed-bandwidth=packet', '--analysis.windowed-bandwidth.rate=100ms'])
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.windowed-bandwidth=datagram', '--analysis.windowed-bandwidth.rate=100ms'])

        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_packet_send.dat'])
        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_packet_receive.dat'])
        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_datagram_send.dat'])
        subprocess.call(['python', './graph_windowed_bandwidth.py', 'windowed_bandwidth_datagram_receive.dat'])

        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_send_packet.dat'])
        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_receive_packet.dat'])
        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_send_datagram.dat'])
        subprocess.call(['python', './graph_windowed_queues.py', 'windowed_queue_info_receive_datagram.dat'])


        subprocess.call(['python', './graph_windowed_jfi.py', 'windowed_bandwidth_packet_send.dat', 'windowed_queue_info_send_packet.dat', 'jfi_packet_send'])
        subprocess.call(['python', './graph_windowed_jfi.py', 'windowed_bandwidth_packet_receive.dat', 'windowed_queue_info_receive_packet.dat', 'jfi_packet_receive'])
        subprocess.call(['python', './graph_windowed_jfi.py', 'windowed_bandwidth_datagram_send.dat', 'windowed_queue_info_send_datagram.dat', 'jfi_datagram_send'])
        subprocess.call(['python', './graph_windowed_jfi.py', 'windowed_bandwidth_datagram_receive.dat', 'windowed_queue_info_receive_datagram.dat', 'jfi_datagram_receive'])


if __name__ == "__main__":
    cc = ClusterConfig()
    cluster_sim = ClusterSim(cc, 2, 2)

    if len(sys.argv) < 2:
        cluster_sim.run()
    else:
        if sys.argv[1] == 'vis':
            cluster_sim.vis()
