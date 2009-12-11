#!/usr/bin/python
import sys
sys.path.insert(0, sys.path[0]+"/../cluster")
import sim
cc = sim.ClusterConfig()
csim = sim.ClusterSimSettings(cc, 8, (8,1), 1)



csim.duration = '100s'
csim.tx_bandwidth = 500000
csim.rx_bandwidth = 500000
csim.flatness = 256
csim.server_queue = 'fair'
csim.server_queue_length = 65536
csim.object_queue = 'fairfifo'
csim.object_queue_length = 65536

# OH: random object generation settings
csim.num_random_objects = 50
csim.object_static = 'random'
csim.object_drift_x = '-10'
csim.object_drift_y = '0'
csim.object_drift_z = '0'
csim.object_simple = 'true'
csim.object_2d = 'true'
csim.object_global = 'false'
csim.object_static = 'static'
csim.object_drift_x = '0'
csim.object_drift_y = '0'
csim.object_drift_z = '0'
# OH: pack object generation settings
csim.num_pack_objects = 0
csim.object_pack = '/home/meru/data/objects.pack'


csim.blocksize = 100
csim.center = (0, 0, 0)
csim.scenario_options='--num-pings-per-second=1 --force-same-object-host=true'
csim.debug = True
csim.valgrind = False
csim.profile = False
csim.loc = 'standard'
csim.cseg = 'uniform'
csim.cseg_service_host = 'indus'
csim.oseg = 'oseg_craq'
csim.oseg_unique_craq_prefix = 'l' # NOTE: this is really a default, you should set unique = x in your .cluster
csim.oseg_analyze_after = '10' #Will perform oseg analysis after this many seconds of the run.

csim.vis_mode = 'object'
csim.vis_seed = 1

csim.loglevels = {
            "prox" : "warn",
            }






csim=sim.ClusterSim(cc,csim);

csim.generate_deployment()
csim.clean_remote_data()
csim.generate_ip_file()
csim.run_cluster_sim()
csim.retrieve_data()
csim.object_latency_analysis()
csim.message_latency_analysis()
csim.bandwidth_analysis()
csim.latency_analysis()
