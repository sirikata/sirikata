#!/usr/bin/python

import sys
import math
import datetime
import types

# FIXME It would be nice to have a better way of making this script able to find
# other modules in sibling packages
sys.path.insert(0, sys.path[0]+"/..")

from config import ClusterConfig
from run import ClusterSubstitute,ClusterRun,ClusterDeploymentRun
from scp import ClusterSCP

from util.cbr_wrapper import RunCBR

import util.stdio
import util.invoke

from graph.windowed_bandwidth import GraphWindowedBandwidth
from graph.windowed_jfi import GraphWindowedJFI
from graph.windowed_queues import GraphWindowedQueues

import socket
import os

CBR_WRAPPER = "util/cbr_wrapper.sh"

# User parameters
class ClusterSimSettings:
    def __init__(self, config, space_svr_pool, layout, num_object_hosts):
        self.config = config
        self.scp=ClusterSCP

        self.layout_x = layout[0]
        self.layout_y = layout[1]
        self.duration = '120s'

        self.region_weight = 'sqr'
        self.region_weight_options = '--flatness=8'

        self.server_queue = 'fair'
        self.server_queue_length = 8192
        self.odp_flow_scheduler = 'region'

        # OH: basic oh settings
        self.num_oh = num_object_hosts
        self.object_connect_phase = '10s'

        # The number of objects settings need some explanation.  The user should always specify
        # the number and type of objects they want in the actual simulation, *per space server*.
        # We take care of pack generation if you specify pack_dump, translating pack objects to
        # random objects automatically during the generation stage. SL trace objects can't be
        # generated, so they are entirely independent.

        # OH: random object generation settings
        self.num_random_objects = 5000
        self.object_static = 'static'
        self.object_drift_x = '-10'
        self.object_drift_y = '0'
        self.object_drift_z = '0'
        self.object_simple = 'true'
        self.object_2d = 'true'
        self.object_query_frac = 0.0

        # OH: pack object generation settings
        if config.pack_dir=="":
          self.pack_dir = '/home/meru/data/'
        else:
          self.pack_dir = config.pack_dir

        self.object_pack = 'objects.pack'
        self.num_pack_objects = 0
        self.pack_dump = False

        # OH: SL trace data loading
        self.object_sl_file = 'sl.dat'
        self.object_sl_center = (0,0,0)
        self.num_sl_objects = 0

        # OH: message trace data loading
        self.message_trace_file = ''


        # OH: scenario / ping settings
        self.scenario = 'ping'
        self.scenario_options = '--num-pings-per-second=1000'

        self.blocksize = 200
        self.zrange=(-10000.,10000)
        self.center = (0, 0, 0)

        self.debug = True
        self.valgrind = False
        self.profile = True
        self.oprofile = False
        self.loc = 'standard'
        self.space_server_pool = space_svr_pool

        self.cseg = 'uniform'
        self.cseg_service_host = ' '
        self.cseg_service_tcp_port = 6234;
        self.cseg_ip_file = "cseg_serverip.txt"

        if (self.cseg == "client"):
            self.num_cseg_servers = 1
        else:
            self.num_cseg_servers = 0

        # Space: OSeg settings
        self.oseg = 'craq'
        self.oseg_unique_craq_prefix = 'M' # NOTE: this is really a default, you should set unique = x in your .cluster
        self.oseg_options = ""

        self.oseg_lookup_queue_size = 2000;
        self.oseg_analyze_after = '40' #Will perform oseg analysis after this many seconds of the run.

        self.oseg_cache_size = 200
        self.oseg_cache_clean_group = 25
        self.oseg_cache_entry_lifetime = "8s"
        self.oseg_cache_selector = "cache_originallru"
        self.oseg_cache_comm_scaling = "1.0"

        # Space: PINTO settings
        self.pinto_type = 'master'
        self.pinto_opts = ''

        # Space: Prox settings
        self.prox_server_query_handler_type = 'rtreecut'
        self.prox_server_query_handler_opts = ''
        self.prox_object_query_handler_type = 'rtreecut'
        self.prox_object_query_handler_opts = ''


        # Pinto Settings
        self.pinto_handler_type = 'rtreecut'
        self.pinto_handler_opts = ''

        # Visualization
        self.vis_mode = 'object'
        self.vis_seed = 1


        # Trace:
        # list of trace types to enable, e.g. ['object', 'oseg'] will
        # result in --trace-object --trace-oseg being passed in
        self.traces = {
            'all' : [],
            'space' : [],
            'simoh' : [],
            'cseg' : [],
            'pinto' : [],
            'analysis' : []
            }

        self.loglevels = {
            "prox" : "warn",
            #"tcpsst" : "insane",
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
        return "<<%f,%f,%f>,<%f,%f,%f>>" % (neg[0]+self.center[0], neg[1]+self.center[1], self.zrange[0], pos[0]+self.center[0], pos[1]+self.center[1], self.zrange[1])

    def unique(self):
        if (self.config.unique == None):
            print 'Using the default CRAQ prefix.  You should set unique in your .cluster file.'
            return self.oseg_unique_craq_prefix
        return self.config.unique

    def oseg_options_param(self):
        if (self.oseg == 'craq'):
            return '--oseg-options=' + "--oseg_unique_craq_prefix=" + self.unique(),
        return ''

    # Options to space about pinto
    def pinto_options_param(self):
        if (self.pinto_type == 'master'):
            assert(self.pinto_ip)
            assert(self.pinto_port)
            return '--pinto-options=' + '--host=' + str(self.pinto_ip) + ' --port=' + str(self.pinto_port)
        return ''

class ClusterSim:
    def __init__(self, config, settings, io=util.stdio.StdIO()):
        self.config = config
        self.settings = settings
        self.io = io

    def num_servers(self):
        return self.settings.space_server_pool + self.settings.num_oh + self.settings.num_cseg_servers + self.num_pinto_servers()

    def max_space_servers(self):
        return self.settings.space_server_pool

    def num_pinto_servers(self):
        if self.settings.pinto_type == 'master':
            return 1
        return 0

    def ip_file(self):
        return "serverip.txt"

    def scripts_dir(self):
        return self.config.code_dir + "/scripts/"

    def pack_filename(self, relname):
        return "remote:" + self.settings.pack_dir + relname

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
        if (self.settings.oprofile):
            params.append("--oprofile")
        # NOTE: Since --profile=x gets passed through to the actual executable,
        # nothing added after this will be processed by the wrapper script.
        if (self.settings.profile):
            params.append("--profile=true")
        return params

    def common_parameters(self):
        params = [
            '--plugins=' + self.config.plugins
            ]

        for tracetype in self.settings.traces['all']:
            params.append( '--trace-%s=true' % (tracetype) )

        return params

    def cbr_parameters(self):
        class_params = {
            'space.plugins' : '--space.plugins=' + self.config.space_plugins,
            'ssid' : '--id=%(node)d',
            'net' : '--net=tcp',
            'server.queue' : "--server.queue=" + self.settings.server_queue,
            'server.queue.length' : "--server.queue.length=" + str(self.settings.server_queue_length),
            'server.odp.flowsched' : "--server.odp.flowsched=" + self.settings.odp_flow_scheduler,
            'loc' : "--loc=" + self.settings.loc,
            'cseg' : "--cseg=" + self.settings.cseg,
            'cseg-service-host' : "--cseg-service-host=" + self.settings.cseg_service_host,
            'cseg-service-tcp-port' : "--cseg-service-tcp-port=" + str(self.settings.cseg_service_tcp_port),
            'oseg' : "--oseg=" + self.settings.oseg,
            'oseg-options' : self.settings.oseg_options_param(),
            'oseg-cache-selector' : "--oseg-cache-selector=" + self.settings.oseg_cache_selector,
            'oseg-cache-scaling' : "--oseg-cache-scaling=" + self.settings.oseg_cache_comm_scaling,
            'oseg_lookup_queue_size' : "--oseg_lookup_queue_size=" + str(self.settings.oseg_lookup_queue_size),
            'oseg-cache-size' : "--oseg-cache-size=" + str(self.settings.oseg_cache_size),
            'oseg-cache-clean-group-size' : "--oseg-cache-clean-group-size=" + str(self.settings.oseg_cache_clean_group),
            'oseg-cache-entry-lifetime' : "--oseg-cache-entry-lifetime=" + str(self.settings.oseg_cache_entry_lifetime),
            'pinto' : '--pinto=' + str(self.settings.pinto_type),
            'pinto-options' : self.settings.pinto_options_param(),
            'prox.server.handler' : '--prox.server.handler=' + self.settings.prox_server_query_handler_type,
            'prox.object.handler' : '--prox.object.handler=' + self.settings.prox_object_query_handler_type,
            }

        if len(self.settings.prox_server_query_handler_opts) > 0:
            class_params['prox.server.handler-options'] = '--prox.server.handler-options=' + self.settings.prox_server_query_handler_opts
        if len(self.settings.prox_object_query_handler_opts) > 0:
            class_params['prox.object.handler-options'] = '--prox.object.handler-options=' + self.settings.prox_object_query_handler_opts

        for tracetype in self.settings.traces['space']:
            class_params[ ('trace-%s' % (tracetype)) ] =  ('--trace-%s=true' % (tracetype))

        params = ['%(' + x + ')s' for x in class_params.keys()]

        return (params, class_params)


    def oh_objects_per_server(self):
        return self.settings.num_random_objects + self.settings.num_pack_objects

    def oh_parameters(self, genpack):
        class_params = {
            'oh.plugins' : '--oh.plugins=' + self.config.simoh_plugins,
            'oh.id' : '--ohid=%(node)d ',
            'oh.connect' : '--object.connect=' + self.settings.object_connect_phase,
            'oh.static' : '--object.static=' + self.settings.object_static,
            'oh.simple' : '--object.simple=' + self.settings.object_simple,
            'oh.2d' : '--object.2d=' + self.settings.object_2d,
            'oh.query-frac' : '--object.query-frac=' + str(self.settings.object_query_frac),
            'oh.pack-dir' : '--object.pack-dir=' + self.settings.pack_dir,
            'object.pack-offset': lambda index : '--object.pack-offset=' + str(index*self.oh_objects_per_server()),
            'object-drift-x' : "--object_drift_x=" + self.settings.object_drift_x,
            'object-drift-y' : "--object_drift_y=" + self.settings.object_drift_y,
            'object-drift-z' : "--object_drift_z=" + self.settings.object_drift_z,
            }
        if (len(self.settings.object_pack)):
            class_params['object.pack'] = '--object.pack=' + self.settings.object_pack
        # For pack generation, we take the specified number of pack objects and make random objects of them, turning pack dump on.
        # For non pack generation, we respect whatever the user specified.  In that case, if they specify random objects, they get them,
        # if they specify pack objects they get those.
        # Therefore, the basic approach is to always specify what kind of objects you want in the full simulation and we take
        # care of getting the pack generation correct.
        if (self.settings.pack_dump and genpack):
            class_params['object.pack-dump'] = '--object.pack-dump=true'
            class_params['oh.num-random'] = '--object.num.random=' + str(self.settings.num_pack_objects * self.settings.num_oh)
            class_params['object.pack-num'] = '--object.pack-num=' + str(0)
        else:
            class_params['oh.num-random'] = '--object.num.random=' + str(self.settings.num_random_objects)
            class_params['object.pack-num'] = '--object.pack-num=' + str(self.settings.num_pack_objects)

        if (len(self.settings.object_sl_file)):
            class_params['object.sl-file'] = '--object.sl-file=' + self.settings.object_sl_file
        if (self.settings.num_sl_objects):
            class_params['object.sl-num'] = '--object.sl-num=' + str(self.settings.num_sl_objects)
        class_params['object.sl-center'] = '--object.sl-center=' + ('<%f,%f,%f>' % self.settings.object_sl_center)
        class_params['object.scenario'] = '--scenario=' + self.settings.scenario
        if self.settings.scenario_options:
            class_params['object.scenario-options'] = '--scenario-options=' + self.settings.scenario_options

        for tracetype in self.settings.traces['simoh']:
            class_params[ ('trace-%s' % (tracetype)) ] =  ('--trace-%s=true' % (tracetype))

        params = ['%(' + x + ')s' for x in class_params.keys()]

        return (params, class_params)

    def cseg_parameters(self):
        class_params = {
            'cseg.plugins' : '--cseg.plugins=' + self.config.cseg_plugins,
            'cseg-id' : lambda index : '--cseg-id=' + str(index),
            'num-cseg-servers' : '--num-cseg-servers=' + str(self.settings.num_cseg_servers),
            'cseg-server-options' : '--cseg-servermap-options=' + '--filename=' + self.settings.cseg_ip_file,
            'max-servers' : '--max-servers=' + str(self.settings.space_server_pool),
            }
        for tracetype in self.settings.traces['cseg']:
            class_params[ ('trace-%s' % (tracetype)) ] =  ('--trace-%s=true' % (tracetype))

        params = ['%(' + x + ')s' for x in class_params.keys()]

        return (params, class_params)

    def pinto_parameters(self):
        class_params = {
            'port' : '--port=' + str(self.settings.pinto_port),
            'handler' : ' --handler=' + str(self.settings.pinto_handler_type),
            }

        if self.config.pinto_plugins != '':
            class_params['pinto.plugins'] = '--pinto.plugins=' + self.config.pinto_plugins

        if self.settings.pinto_handler_opts != '':
            class_params['handler-options'] = '--handler-options=' + self.settings.pinto_handler_opts

        for tracetype in self.settings.traces['pinto']:
            class_params[ ('trace-%s' % (tracetype)) ] =  ('--trace-%s=true' % (tracetype))

        params = ['%(' + x + ')s' for x in class_params.keys()]

        return (params, class_params)

    def analysis_parameters(self):
        params = [
            '--analysis.plugins=' + self.config.analysis_plugins,
            "--layout=" + self.settings.layout(),
            "--num-oh=" + str(self.settings.num_oh),
            "--analysis.total.num.all.servers=" + str(self.num_servers()), 
            '--servermap=' + 'tabular',
            '--servermap-options=' + '--filename=' + self.ip_file(),
            "--duration=" + self.settings.duration,
            '--max-servers=' + str(self.max_space_servers()),
            '--region-weight=' + str(self.settings.region_weight),
            '--region-weight-args=' + str(self.settings.region_weight_options),
            ]

        for tracetype in self.settings.traces['analysis']:
            params.append( '--trace-%s=true' % (tracetype) )

        return params

    def vis_parameters(self):
        class_params = {
            'vis' : lambda index : ['--analysis.locvis=' + self.settings.vis_mode, '--analysis.locvis.seed=' + str(self.settings.vis_seed)]
            }
        params = ['%(' + x + ')s' for x in class_params.keys()]
        return (params, class_params)

    def run_pre(self):
        self.generate_deployment()

        self.clean_local_data()
        self.clean_remote_data()
        self.generate_ip_file()

    def run_main(self):
        self.run_cluster_sim()
        self.retrieve_data()


    def run_analysis(self):
        self.bandwidth_analysis()
        self.latency_analysis()
        self.oseg_analysis()
        self.object_latency_analysis()

        self.loc_latency_analysis()
        self.prox_dump_analysis()

    def run(self):
        self.run_pre()
        self.run_main()
        self.run_analysis()

    def generate_deployment(self):
        self.config.generate_deployment(self.num_servers())

    def clean_local_data(self):
        util.invoke.invoke(['rm -f trace*'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f serverip*'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f analysis.trace'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f *.ps'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f *.dat'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f distance_latency_histogram.csv'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f loc_latency*'], shell=True, io=self.io)
        util.invoke.invoke(['rm -f prox.log'], shell=True, io=self.io)

    def clean_remote_data(self):
        clean_cmd = "cd " + self.scripts_dir() + "; rm trace*; rm serverip*;"
        ClusterRun(self.config, clean_cmd, io=self.io)

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
        ClusterSCP(self.config, [serveripfile, "remote:" + self.scripts_dir()], io=self.io)

        # Generate the cseg-serverip file, copy it to all nodes
        serveripfile = self.settings.cseg_ip_file
        fp = open(serveripfile,'w')
        for i in range(0, self.settings.num_cseg_servers):
            fp.write(self.config.deploy_nodes[server_index].node+":"+str(port)+":"+str(port+1)+'\n')

            if (self.settings.cseg_service_host == " "):
                self.settings.cseg_service_host = self.config.deploy_nodes[server_index].node

            port += 2
            server_index += 1

        fp.close()
        ClusterSCP(self.config, [serveripfile, "remote:" + self.scripts_dir()], io=self.io)

        # Generate pinto "ip file", aka the local list we use to create parameter lists
        if self.num_pinto_servers() > 0:
            assert(self.num_pinto_servers() == 1)
            self.settings.pinto_ip = self.config.deploy_nodes[server_index].node
            self.settings.pinto_port = port
            port += 2
            server_index += 1

    def push_init_data(self):
        # If we're using a dump file, push it
        if (len(self.settings.object_pack)):
            ClusterSCP(self.config, [self.settings.object_pack, self.pack_filename(self.settings.object_pack)], io=self.io)
        if (len(self.settings.object_sl_file)):
            ClusterSCP(self.config, [self.settings.object_sl_file, self.pack_filename(self.settings.object_sl_file)], io=self.io)
        if (len(self.settings.message_trace_file)):
            print "coopying ",self.settings.message_trace_file, self.pack_filename(self.settings.message_trace_file)
            ClusterSCP(self.config, [self.settings.message_trace_file, self.pack_filename(self.settings.message_trace_file)], io=self.io)


    def fill_parameters(self, node_params, param_list, node_class_matches, idx):
        for parm,parm_val in param_list.items():
            if parm not in node_params:
                node_params[parm] = []
            if not node_class_matches:
                node_params[parm].append('')
            else:
                if type(parm_val) == types.FunctionType:
                    node_params[parm].append(parm_val(idx))
                else:
                    node_params[parm].append(parm_val)


    # instance_types: [] of tuples (type, count) - types of nodes, e.g.
    #                 'space', 'simoh', 'cseg' and how many of each should
    #                 be deployed
    # local: bool indicating whether this should all be run locally or remotely
    # genpack: bool indicating if this is a genpack run
    def run_instances(self, instance_types, local, genpack):
        # get various types of parameters, along with node-specific dict-functors
        debug_params = self.debug_parameters()
        common_params = self.common_parameters()
        cbr_params, cbr_param_functor_dict = self.cbr_parameters()
        oh_params, oh_param_functor_dict = self.oh_parameters(genpack)
        vis_params, vis_param_functor_dict = self.vis_parameters()
        cseg_params, cseg_param_functor_dict = self.cseg_parameters()
        pinto_params, pinto_param_functor_dict = self.pinto_parameters()

        # Construct the node-specific parameter lists
        node_params = {}
        node_params['binary'] = []

        num_cseg_instances = 0
        num_pinto_instances = 0

        for node_class in instance_types:
            node_type = node_class[0]
            node_count = node_class[1]

            for x in range(0, node_count):
                # FIXME should just map directly to binaries
                if (node_type == 'space'):
                    node_params['binary'].append('space')
                elif (node_type == 'simoh'):
                    node_params['binary'].append('simoh')
                elif (node_type == 'genpack'):
                    node_params['binary'].append('genpack')
                elif (node_type == 'vis'):
                    node_params['binary'].append('analysis')
                elif (node_type == 'cseg'):
                    node_params['binary'].append('cseg')
                    num_cseg_instances += 1
                elif (node_type == 'pinto'):
                    node_params['binary'].append('pinto')
                    num_pinto_instances += 1
                else:
                    node_params['binary'].append('')
                self.fill_parameters(node_params, vis_param_functor_dict, node_type == 'analysis', x)
                self.fill_parameters(node_params, cseg_param_functor_dict, node_type == 'cseg', num_cseg_instances)
                self.fill_parameters(node_params, pinto_param_functor_dict, node_type == 'pinto', num_pinto_instances)
                self.fill_parameters(node_params, oh_param_functor_dict, node_type == 'simoh' or node_type == 'genpack', x)
                self.fill_parameters(node_params, cbr_param_functor_dict, node_type == 'space', x)

        wait_until_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S%z")

        # Construct the command lines to do the actual run and dispatch them
        cmd_seq = []
        cmd_seq.extend( [
            "%(binary)s",
            ] )
        cmd_seq.extend(debug_params)
        cmd_seq.extend(common_params)
        ts_string = ""
        if self.config.timeserver:
            ts_string = "--time-server=" + self.config.timeserver
        cmd_seq.extend( [
                ts_string,
                "--layout=" + self.settings.layout(),
                "--region=" + self.settings.region(),
                '--servermap=' + 'tabular',
                '--servermap-options=' + '--filename=' + self.ip_file(),
                "--duration=" + self.settings.duration,
                "--wait-until=" + wait_until_time,
                "--wait-additional=10s",
                '--region-weight=' + str(self.settings.region_weight),
                '--region-weight-args=' + str(self.settings.region_weight_options),
                "--capexcessbandwidth=false",
                ])
        cmd_seq.extend(cbr_params)
        cmd_seq.extend(oh_params)
        cmd_seq.extend(vis_params)
        cmd_seq.extend(cseg_params)
        cmd_seq.extend(pinto_params)

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
            RunCBR(cmd, io=self.io)
        else: # cluster deployment
            # Generate the command string itself
            #  Note: we need to quote the entire command, so we we quote each parameter with escaped quotes,
            #        then separate with spaces, then wrap in regular quotes
            real_cmd_seq = [CBR_WRAPPER]
            real_cmd_seq.extend(cmd_seq)
            cmd_seq = real_cmd_seq
            cmd = ' '.join([ '"' + x + '"' for x in cmd_seq])
            # FIXME this gets prepended here because I can't get all the quoting to work out with ssh properly
            # if it is included as part of the normal process
            cmd = 'cd ' + self.scripts_dir() + ' ; ' + cmd
            # And deploy
            ClusterDeploymentRun(self.config, cmd, node_params, io=self.io)


    def run_cluster_sim(self):
        # As a special case, if we need a pack, run a special instance
        if self.settings.pack_dump:
            instances = [('genpack', 1)]
            self.run_instances(instances, True, True)

        # note: we do this here since we added push_init_data after a
        # bunch of things were already setup to use the old sequence
        # of initialization
        self.push_init_data()

        # Then run the real simulation
        instances = [
            ('space', self.settings.space_server_pool),
            ('cseg', self.settings.num_cseg_servers),
            ('pinto', self.num_pinto_servers()),
            ('simoh', self.settings.num_oh)
            ]
        self.run_instances(instances, False, False)



    def vis(self):
        instances = [
            ('vis', 1)
            ]
        self.run_instances(instances, True, False)


    def retrieve_data(self):
        # Copy the trace and sync data back here
        trace_file_pattern = "remote:" + self.scripts_dir() + "trace-%(node)04d.txt"
        ClusterSCP(self.config, [trace_file_pattern, "."], io=self.io)

    def construct_analysis_cmd(self, user_args):
        cmd = ['analysis']
        analysis_params = self.analysis_parameters()
        cmd.extend(analysis_params)
        cmd.extend(user_args)
        return cmd

    def bandwidth_analysis(self):
        RunCBR(
            self.construct_analysis_cmd([
                        '--analysis.windowed-bandwidth=datagram',
                        '--analysis.windowed-bandwidth.rate=100ms'
                        ]),
            io=self.io)

        GraphWindowedBandwidth('windowed_bandwidth_datagram_send.dat')
        GraphWindowedBandwidth('windowed_bandwidth_datagram_receive.dat')

        GraphWindowedQueues('windowed_queue_info_send_datagram.dat')
        GraphWindowedQueues('windowed_queue_info_receive_datagram.dat')

        GraphWindowedJFI('windowed_bandwidth_datagram_send.dat', 'windowed_queue_info_send_datagram.dat', 'jfi_datagram_send')
        #GraphWindowedJFI('windowed_bandwidth_datagram_receive.dat', 'windowed_queue_info_receive_datagram.dat', 'jfi_datagram_receive')


    def latency_analysis(self):
        RunCBR(self.construct_analysis_cmd([
                    '--analysis.latency=true'
                    ]),
               io=self.io)

    def object_latency_analysis(self):
        RunCBR(self.construct_analysis_cmd([
                    '--analysis.object.latency=true'
                    ]),
               io=self.io)

    def message_latency_analysis(self, filename=None):
        stdout_fp = None
        if filename != None:
            stdout_fp = open(filename, 'w')

        RunCBR(self.construct_analysis_cmd([
                    '--analysis.message.latency=true'
                    ]),
               io=self.io,
               stdout=stdout_fp,
               stderr=stdout_fp)
        if stdout_fp != None:
            stdout_fp.close()

    def oseg_analysis(self):
        RunCBR(self.construct_analysis_cmd([
                    '--analysis.oseg=true',
                    "--oseg_analyze_after=" + self.settings.oseg_analyze_after
                    ]),
               io=self.io)

    def loc_latency_analysis(self):
        RunCBR(self.construct_analysis_cmd([
                    '--analysis.loc.latency=true'
                    ]),
               io=self.io)

    def prox_dump_analysis(self):
        RunCBR(self.construct_analysis_cmd([
                    '--analysis.prox.dump=prox.log'
                    ]),
               io=self.io)

    def flow_stats_analysis(self, filename=None):
        stdout_fp = None
        if filename != None:
            stdout_fp = open(filename, 'w')

        RunCBR(self.construct_analysis_cmd([
                    '--analysis.flow.stats=true'
                    ]),
               io=self.io,
               stdout=stdout_fp,
               stderr=stdout_fp)
        if stdout_fp != None:
            stdout_fp.close()


if __name__ == "__main__":
    cc = ClusterConfig()
    cs = ClusterSimSettings(cc, 4, (2,2), 1)
#    cs = ClusterSimSettings(cc, 2, (2,1), 1)
#    cs = ClusterSimSettings(cc, 3, (3,1), 1)
#    cs = ClusterSimSettings(cc, 2, (2,1), 1)
#    cs = ClusterSimSettings(cc, 8, (8,1), 1)
#    cs = ClusterSimSettings(cc, 8, (8,1), 1)
#    cs = ClusterSimSettings(cc, 14, (2,2), 1)
#    cs = ClusterSimSettings(cc, 4, (2,2), 1)
#    cs = ClusterSimSettings(cc, 4, (4,1), 1)
#    cs = ClusterSimSettings(cc, 16, (4,4), 1)

    # For command line, enable traces by default that are needed for the analyses.
    cs.traces = ['oseg', 'migration', 'ping', 'message']

    cs.oseg_cache_selector = "cache_communication";
    cs.oseg_cache_comm_scaling = "1.0";

    cluster_sim = ClusterSim(cc, cs)

    if len(sys.argv) < 2:
        cluster_sim.run()
    else:
        if sys.argv[1] == 'vis':
            cluster_sim.vis()
        elif sys.argv[1] == 'clean':
            cluster_sim.clean_local_data()
