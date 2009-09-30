#!/usr/bin/python

import sys
import math
import subprocess
import datetime
from cluster_config import ClusterConfig
from cluster_run import ClusterSubstitute,ClusterRun,ClusterDeploymentRun
from cluster_scp import ClusterSCP

CBR_WRAPPER = "./cbr_wrapper.sh"

# User parameters
class ClusterSimSettings:
    def __init__(self, config, space_svr_pool, layout, num_object_hosts):
        self.config = config

        self.layout_x = layout[0]
        self.layout_y = layout[1]
        self.duration = '150s'
        self.tx_bandwidth = 1000000
        self.rx_bandwidth = 1000000
        self.flatness = 500
        self.server_queue = 'fair'
        self.server_queue_length = 8192
        self.object_queue = 'fairfifo'
        self.object_queue_length = 8192

        # OH: basic oh settings
        self.num_oh = num_object_hosts
        self.object_connect_phase = '0s'

        # OH: random object generation settings
        self.num_random_objects = 100
        self.object_static = 'random'
        self.object_drift_x = '-10'
        self.object_drift_y = '0'
        self.object_drift_z = '0'
        self.object_simple = 'true'
        self.object_2d = 'true'
        self.object_global = 'false'

        # OH: pack object generation settings
        self.num_pack_objects = 0
        self.object_pack = '/home/meru/data/objects.pack'


        self.blocksize = 200
        self.center = (0, 0, 0)

        self.noise = 'true'
        self.debug = True
        self.valgrind = False
        self.profile = True
        self.loc = 'standard'
        self.space_server_pool = space_svr_pool
        self.cseg = 'uniform'
        self.cseg_service_host = 'indus'
        self.oseg = 'oseg_craq'
        self.oseg_unique_craq_prefix = 'M' # NOTE: this is really a default, you should set unique = x in your .cluster
        self.oseg_analyze_after = '100' #Will perform oseg analysis after this many seconds of the run.

        self.vis_mode = 'object'
        self.vis_seed = 1

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
        neg = [ -x for x in half_extents ]
        pos = [ x for x in half_extents ]
        return "<<%f,%f,%f>,<%f,%f,%f>>" % (neg[0]+self.center[0], neg[1]+self.center[1], neg[2]+self.center[2], pos[0]+self.center[0], pos[1]+self.center[1], pos[2]+self.center[2])

    def unique(self):
        if (self.config.unique == None):
            print 'Using the default CRAQ prefix.  You should set unique in your .cluster file.'
            return self.oseg_unique_craq_prefix
        return self.config.unique


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

    # x_parameters get parameters which affect x category of options
    # Will be returned as a tuple (params, class_params) where
    # params is an array of strings containing the parameters
    # and class_params is a dict of class (space, oh, cseg) => ( dict of
    #  param => function(index) ) which generates the per-node parameter
    # to be passed.

    def debug_parameters(self):
        params = []
        if (self.settings.debug):
            params.append("--debug")
        if (self.settings.valgrind):
            params.append("--valgrind")
        if (self.settings.profile):
            params.append("--profile=true")
        class_params = {}
        return (params, class_params)

    def oh_objects_per_server(self):
        return self.settings.num_random_objects + self.settings.num_pack_objects

    def oh_parameters(self):
        params = [
            '--ohid=%(node)d ',
            '--object.connect=' + self.settings.object_connect_phase,
            '--object.num.random=' + str(self.settings.num_random_objects),
            '--object.static=' + self.settings.object_static,
            '--object.simple=' + self.settings.object_simple,
            '--object.2d=' + self.settings.object_2d,
            '--object.global=' + self.settings.object_global,
            '--object.num.pack=' + str(self.settings.num_pack_objects),
            '--object.pack=' + self.settings.object_pack,
            '%(packoffset)s',
            ]
        class_params = {
            'packoffset' : {
                'oh' : lambda index : ['--object.pack-offset=' + str(index*self.oh_objects_per_server())]
                }
            }
        return (params, class_params)


    def vis_parameters(self):
        params = ['%(vis)s']
        class_params = {
            'vis' : {
                'vis' : lambda index : ['--analysis.locvis=' + self.settings.vis_mode, '--analysis.locvis.seed=' + str(self.settings.vis_seed)]
                }
            }
        return (params, class_params)


    def run(self):
        self.generate_deployment()

        self.clean_local_data()
        self.clean_remote_data()
        self.generate_ip_file()
        self.run_cluster_sim()
        self.retrieve_data()

        self.bandwidth_analysis()
        self.latency_analysis()
        self.oseg_analysis()
        self.object_latency_analysis()

        self.loc_latency_analysis()
        self.prox_dump_analysis()


    def generate_deployment(self):
        self.config.generate_deployment(self.num_servers())

    def clean_local_data(self):
        subprocess.call(['rm -f trace*'], shell=True)
        subprocess.call(['rm -f serverip*'], shell=True)
        subprocess.call(['rm -f *.ps'], shell=True)
        subprocess.call(['rm -f *.dat'], shell=True)
        subprocess.call(['rm -f distance_latency_histogram.csv'], shell=True)

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

    def fill_parameters(self, node_params, param_dict, node_class, idx):
        for parm,parm_map in param_dict.items():
            if (not parm in node_params):
                node_params[parm] = []
            if not node_class in parm_map:
                node_params[parm].append('')
            else:
                node_params[parm].extend(parm_map[node_class](idx))


    # instance_types: [] of tuples (type, count) - types of nodes, e.g.
    #                 'space', 'oh', 'cseg' and how many of each should
    #                 be deployed
    # local: bool indicating whether this should all be run locally or remotely
    def run_instances(self, instance_types, local):
        # get various types of parameters, along with node-specific dict-functors
        debug_params, debug_param_functor_dict = self.debug_parameters()
        oh_params, oh_param_functor_dict = self.oh_parameters()
        vis_params, vis_param_functor_dict = self.vis_parameters()

        # Construct the node-specific parameter lists
        node_params = {}
        node_params['binary'] = []

        for node_class in instance_types:
            node_type = node_class[0]
            node_count = node_class[1]

            for x in range(0, node_count):
                # FIXME should just map directly to binaries
                if (node_type == 'space'):
                    node_params['binary'].append('cbr')
                elif (node_type == 'oh'):
                    node_params['binary'].append('oh')
                elif (node_type == 'vis'):
                    node_params['binary'].append('cbr')
                else:
                    node_params['binary'].append('')
                self.fill_parameters(node_params, debug_param_functor_dict, node_type, x)
                self.fill_parameters(node_params, oh_param_functor_dict, node_type, x)
                self.fill_parameters(node_params, vis_param_functor_dict, node_type, x)

        print node_params

        wait_until_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S%z")

        # Construct the command lines to do the actual run and dispatch them
        cmd_seq = []
        cmd_seq.extend( [
            CBR_WRAPPER,
            "%(binary)s",
            ] )
        cmd_seq.extend(debug_params)
        cmd_seq.extend( [
                "--id=%(node)d",
                "--layout=" + self.settings.layout(),
                "--num-oh=" + str(self.settings.num_oh),
                "--region=" + self.settings.region(),
                "--serverips=" + self.ip_file(),
                "--duration=" + self.settings.duration,
                "--send-bandwidth=" + str(self.settings.tx_bandwidth),
                "--receive-bandwidth=" + str(self.settings.rx_bandwidth),
                "--wait-until=" + wait_until_time,
                "--wait-additional=10s",
                "--flatness=" + str(self.settings.flatness),
                "--capexcessbandwidth=false",
                "--server.queue=" + self.settings.server_queue,
                "--server.queue.length=" + str(self.settings.server_queue_length),
                "--object.queue=" + self.settings.object_queue,
                "--object.queue.length=" + str(self.settings.object_queue_length),
                "--noise=" + self.settings.noise,
                "--loc=" + self.settings.loc,
                "--cseg=" + self.settings.cseg,
                "--cseg-service-host=" + self.settings.cseg_service_host,
                "--oseg=" + self.settings.oseg,
                "--oseg_unique_craq_prefix=" + self.settings.unique(),
                "--object_drift_x=" + self.settings.object_drift_x,
                "--object_drift_y=" + self.settings.object_drift_y,
                "--object_drift_z=" + self.settings.object_drift_z,
                "--oseg_analyze_after=" + self.settings.oseg_analyze_after,

                ])
        cmd_seq.extend(oh_params)
        cmd_seq.extend(vis_params)

        # Add a param for loglevel if necessary
        if len(self.settings.loglevels) > 0:
            loglevel_cmd = "--moduleloglevel="
            mods_count = 0
            for mod,level in self.settings.loglevels.items():
                if mods_count > 1:
                    loglevel_cmd += ","
                loglevel_cmd += mod + "=" + level
                mods_count = mods_count + 1
            cmd_seq.append(loglevel_cmd)

        if local:
            # FIXME do we ever need to handle multiple local runs
            # FIXME what do we do about index? ideally it shouldn't matter, but analysis code cares about index=0
            cmd = ClusterSubstitute(cmd_seq, host='localhost', user='xxx', index=1, user_params=node_params)
            print cmd
            subprocess.call(cmd)
        else: # cluster deployment
            # Generate the command string itself
            #  Note: we need to quote the entire command, so we we quote each parameter with escaped quotes,
            #        then separate with spaces, then wrap in regular quotes
            cmd = ' '.join([ '"' + x + '"' for x in cmd_seq])
            # FIXME this gets prepended here because I can't get all the quoting to work out with ssh properly
            # if it is included as part of the normal process
            cmd = 'cd ' + self.scripts_dir() + ' ; ' + cmd
            # And deploy
            ClusterDeploymentRun(self.config, cmd, node_params)


    def run_cluster_sim(self):
        instances = [
            ('space', self.settings.space_server_pool),
            ('oh', self.settings.num_oh)
            ]
        self.run_instances(instances, False)

    def vis(self):
        instances = [
            ('vis', 1)
            ]
        self.run_instances(instances, True)


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

    def loc_latency_analysis(self):
        subprocess.call([CBR_WRAPPER, '--debug', '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.loc.latency=true', '--max-servers=' + str(self.max_space_servers()) ])

    def prox_dump_analysis(self):
        subprocess.call([CBR_WRAPPER, '--id=1', "--layout=" + self.settings.layout(), "--num-oh=" + str(self.settings.num_oh), "--serverips=" + self.ip_file(), "--duration=" + self.settings.duration, '--analysis.prox.dump=prox.log', '--max-servers=' + str(self.max_space_servers()) ])



if __name__ == "__main__":
    cc = ClusterConfig()
#    cs = ClusterSimSettings(cc, 3, (3,1), 1)
    cs = ClusterSimSettings(cc, 4, (2,2), 1)
#    cs = ClusterSimSettings(cc, 8, (8,1), 1)
#    cs = ClusterSimSettings(cc, 8, (8,1), 1)
#    cs = ClusterSimSettings(cc, 14, (2,2), 1)
#    cs = ClusterSimSettings(cc, 4, (2,2), 1)
#    cs = ClusterSimSettings(cc, 4, (4,1), 1)
#    cs = ClusterSimSettings(cc, 16, (4,4), 1)


    cluster_sim = ClusterSim(cc, cs)

    if len(sys.argv) < 2:
        cluster_sim.run()
    else:
        if sys.argv[1] == 'vis':
            cluster_sim.vis()
