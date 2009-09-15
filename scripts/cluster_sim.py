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
    def __init__(self, space_svr_pool, layout, num_object_hosts):
        self.num_oh = num_object_hosts
        self.layout_x = layout[0]
        self.layout_y = layout[1]
        self.duration = '50s'
        self.tx_bandwidth = 10000
        self.rx_bandwidth = 10000
        self.flatness = 100
        self.num_objects = 100
        self.server_queue = 'fair'
        self.server_queue_length = 8192
        self.object_queue = 'fairfifo'
        self.object_queue_length = 8192
        self.object_static = 'false'
        self.object_simple = 'true'
        self.object_2d = 'true'
        self.object_global = 'false'
        self.noise = 'true'
        self.debug = True
        self.valgrind = False
        self.profile = False
        self.loc = 'oracle'
        self.blocksize = 200
        self.space_server_pool = space_svr_pool
        self.cseg = 'uniform'
        self.cseg_service_host = 'indus'
        self.oseg = 'oseg_loc'
        self.oseg_unique_craq_prefix = 'M'

        self.loglevels = {
            "prox" : "warn",
            }

        # sanity checks
        if (self.space_server_pool < self.layout_x * self.layout_y):
            print "Space server pool not large enough for desired layout."
            sys.exit(-1)


    def layout(self):
        return "<" + str(self.layout_x) + "," + str(self.layout_y) + ",1>"

    def region(self):
        half_extents = [ self.blocksize * x / 2 for x in (self.layout_x, self.layout_y, 1)]
        return "<<" + str(-half_extents[0]) + "," + str(-half_extents[1]) + "," + str(-half_extents[2]) + ">,<" + str(half_extents[0]) + "," + str(half_extents[1]) + "," + str(half_extents[2]) + ">>"

class ClusterSim:
    def __init__(self, config, settings):
        self.config = config
        self.settings = settings

    def num_servers(self):
        return self.settings.space_server_pool + self.settings.num_oh

    def max_space_servers(self):
        return self.settings.space_server_pool

    def ip_file(self):
        return "serverip.txt"

    def scripts_dir(self):
        return self.config.code_dir + "/scripts/"

    def run(self):
        self.config.generate_deployment(self.num_servers())
        self.clean_local_data()
        self.clean_remote_data()
        self.generate_ip_file()
        self.run_instances()
        self.retrieve_data()
        self.bandwidth_analysis()
        self.latency_analysis()
        self.oseg_analysis()
        self.object_latency_analysis()

    def vis(self):
        subprocess.call([CBR_WRAPPER,
                         '--debug',
                         '--id=1',
                         "--layout=" + self.settings.layout(),
                         "--num-oh=" + str(self.settings.num_oh),
                         '--max-servers=' + str(self.max_space_servers()),
                         "--region=" + self.settings.region(),
                         "--serverips=" + self.ip_file(),
                         "--duration=" + self.settings.duration,
                         '--analysis.locvis=object',
                         '--analysis.locvis.seed=1',
                         "--object.static=" + self.settings.object_static,
                         "--object.simple=" + self.settings.object_simple,
                         "--object.2d=" + self.settings.object_2d
                         ])

    def clean_local_data(self):
        subprocess.call(['rm -f trace*'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f serverip*'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f *.ps'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f *.dat'], 0, None, None, None, None, None, False, True)
        subprocess.call(['rm -f distance_latency_histogram.csv'], 0, None, None, None, None, None, False, True)

    def clean_remote_data(self):
        clean_cmd = "cd " + self.scripts_dir() + "; rm trace*; rm serverip*;"
        ClusterRun(self.config, clean_cmd)

    def generate_ip_file(self):
        # Generate the serverip file, copy it to all nodes
        serveripfile = self.ip_file()
        fp = open(serveripfile,'w')
        port = self.config.port_base
        server_index = 0;
        for i in range(0, self.settings.space_server_pool):
            fp.write(self.config.deploy_nodes[server_index].node+":"+str(port)+":"+str(port+1)+'\n')
            port += 2
            server_index += 1

        fp.close()
        ClusterSCP(self.config, [serveripfile, "remote:" + self.scripts_dir()])

    def run_instances(self):
        # Construct the list of binaries to be run
        binaries = []
        for x in range(0, self.settings.space_server_pool):
            binaries.append("cbr");
        for x in range(0, self.settings.num_oh):
            binaries.append("oh");
        print binaries

        wait_until_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S%z")
        # Construct the command lines to do the actual run and dispatch them
        cmd = "cd " + self.scripts_dir() + "; "
        cmd += CBR_WRAPPER + " "
        cmd += "%(binary)s "
        if (self.settings.debug):
            cmd += "--debug "
        if (self.settings.valgrind):
            cmd += "--valgrind "
        if (self.settings.profile):
            cmd += "--profile=true "
        cmd += "--id=%(node)d "
        cmd += "--ohid=%(node)d "
        cmd += "\"--layout=" + self.settings.layout() + "\" "
        cmd += "--num-oh=" + str(self.settings.num_oh) + " "
        cmd += "\"--region=" + self.settings.region() + "\" "
        cmd += "--serverips=" + self.ip_file() + " "
        cmd += "--duration=" + self.settings.duration + " "
        cmd += "--send-bandwidth=" + str(self.settings.tx_bandwidth) + " "
        cmd += "--receive-bandwidth=" + str(self.settings.rx_bandwidth) + " "
        cmd += "--wait-until=" + "\"" + wait_until_time + "\" "
        cmd += "--wait-additional=6s "
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
        cmd += "--cseg=" + self.settings.cseg + " "
        cmd += "--cseg-service-host=" + self.settings.cseg_service_host + " "
        cmd += "--oseg=" + self.settings.oseg + " "
        cmd += "--oseg_unique_craq_prefix=" + self.settings.oseg_unique_craq_prefix + "  "

        if len(self.settings.loglevels) > 0:
            cmd += "\""
            cmd += "--moduleloglevel="
            mods_count = 0
            for mod,level in self.settings.loglevels.items():
                if mods_count > 1:
                    cmd += ","
                cmd += mod + "=" + level
                mods_count = mods_count + 1
            cmd += "\""
        cmd += " "

        ClusterDeploymentRun(self.config, cmd, { 'binary' : binaries } )

    def retrieve_data(self):
        # Copy the trace and sync data back here
        trace_file_pattern = "remote:" + self.scripts_dir() + "trace-%(node)04d.txt"
        ClusterSCP(self.config, [trace_file_pattern, "."])

    def bandwidth_analysis(self):
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.windowed-bandwidth=packet', '--analysis.windowed-bandwidth.rate=100ms', '--max-servers=' + str(self.max_space_servers()) ])
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.windowed-bandwidth=datagram', '--analysis.windowed-bandwidth.rate=100ms', '--max-servers=' + str(self.max_space_servers()) ])

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


    def latency_analysis(self):
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.latency=true', '--max-servers=' + str(self.max_space_servers())])

    def object_latency_analysis(self):
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.object.latency=true', '--max-servers=' + str(self.max_space_servers())])


    def oseg_analysis(self):
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.oseg=true' ])



if __name__ == "__main__":
    cc = ClusterConfig()
#    cs = ClusterSimSettings(14, (2,2), 1)
    cs = ClusterSimSettings(4, (2,2), 1)
    cluster_sim = ClusterSim(cc, cs)

    if len(sys.argv) < 2:
        cluster_sim.run()
    else:
        if sys.argv[1] == 'vis':
            cluster_sim.vis()
