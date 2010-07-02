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
import flow_fairness
class FlowPairFairness(flow_fairness.FlowFairness):
    def _setup_cluster_sim(self, rate, io):
        self.cs.scenario = 'loadpackettrace'

        if self.local: localval = 'true'
        else: localval = 'false'
        self.cs.object_simple='false'
        
        self.cs.scenario_options = ' '.join(
            ['--num-pings-per-second=' + str(rate),
             '--num-objects-per-server=512',
             '--ping-size=' + str(self.payload_size),
             '--local=' + localval,
             " --tracefile="+trmsgfile
             ]
            )
        self.cs.odp_flow_scheduler = self.scheme

        if 'object' not in self.cs.traces['simoh']: self.cs.traces['simoh'].append('object')
        if 'ping' not in self.cs.traces['simoh']: self.cs.traces['simoh'].append('ping')
        if 'message' not in self.cs.traces['all']: self.cs.traces['all'].append('message')
        cluster_sim = ClusterSim(self.cc, self.cs, io=io)
        return cluster_sim


if __name__ == "__main__":
    nss=16
    
    nobjects = 1000#19000#326
    packname = '1a_objects.pack'
    # If genpack is True, the sim will be run the first time with a
    # single object host to generate data, dump it, and pull it down.
    # Then run with genpack = False to push that pack up to all nodes
    # and use it across multiple object hosts.
    genpack = False
    numoh = 1

    if (genpack):
        numoh = 1

    cc = ClusterConfig()
    import math;
    edgex=int(math.sqrt(nss))
    edgey=int(nss/int(math.sqrt(nss)))
             
    cs = ClusterSimSettings(cc, nss, (edgex,edgey), numoh)
    
    cs.region_weight_options = '--flatness=8'
    cs.debug = True
    
    cs.valgrind = False
    cs.profile = False
    cs.oprofile = False
    cs.loglevels["oh"]="insane";
    cs.loc = 'standard'
    cs.blocksize = 256
    cs.tx_bandwidth = 50000000
    cs.rx_bandwidth = 5000000
    cs.oseg_cache_clean_group=25;
    cs.oseg_cache_entry_lifetime= "10000s"


    
    #if (genpack):
    #    # Pack generation, run with 1 oh
    #    assert(cs.num_oh == 1)
    #    cs.num_random_objects = nobjects
    #    cs.num_pack_objects = 0
    #    cs.object_pack = ''
    #    cs.pack_dump = packname
    #elif (numoh > 1):
    #    # Use pack across multiple ohs
    #    cs.num_random_objects = 0
    #    cs.num_pack_objects = nobjects / cs.num_oh
    #    cs.object_pack = packname
    #    cs.pack_dump = ''
    #else:
    #    # Only 1 oh, just use random
    #    cs.num_random_objects = nobjects
    #    cs.num_pack_objects = 0
    #    cs.object_pack = ''
    #    cs.pack_dump = ''
    cs.num_random_objects = 0
    cs.object_sl_file='sl.trace.'+str(edgex)+'x'+str(edgey);
    cs.object_sl_center=(384,384,0);
    cs.object_connect_phase = '20s'
    cs.center=[cs.blocksize*edgex/2,cs.blocksize*edgey/2,0]
    cs.zrange=(-10000,10000)
    cs.object_static = 'static'
    cs.object_query_frac = 0.0

    cs.duration = '420s'

    rates = sys.argv[1:]
    nobjectlist=[250,500,750,1000,1250,1500,1750,2000];#+=
    nobjectlist+=[2500,3000,3500,4000,4500]+range(5000,20000,1000)
    nobjectlist.reverse()
    #nobjectlist=[19000]
    caches=[256]*len(nobjectlist)
    #caches+=[250]*len(nobjectlist)
    #caches+=[750]*len(nobjectlist)

    #caches+=[75]*len(nobjectlist)#[10,15,20,25,30,35,40]
    #nobjectlist=nobjectlist*4#run with 4 caches
    cs.oseg_cache_size=caches[0];
    cs.oseg_cache_selector='cache_communication';
    plan = FlowPairFairness(cc, cs, scheme='csfq', payload=1024)
    oldoptions=plan.cs.scenario_options;
    done={}
    adder=""
    print "SCENARIO OPTIONS ",plan.cs.scenario_options
    for rate in rates:
        for nobjectsindex in range(len(nobjectlist)):

            cs.oseg_cache_size=caches[nobjectsindex];
            
            nobjects=nobjectlist[nobjectsindex]
            if nobjects in done:
                adder+='c';
                done={}
            msgfile='messagetrace.'+str(nobjects);
            global trmsgfile

            cs.num_sl_objects=nobjects;
            cs.message_trace_file=msgfile;
            trace_location=cs.pack_dir+'/'+msgfile
            trmsgfile=trace_location
            print 'loading file '+cs.object_sl_file+' with trace '+msgfile
            plan.run(rate)
            plan.analysis()
            nam='endtoend';
            
            if len(rates)>1:
                nam+='-'+str(rate);
            nam+=adder
            nam+='.'
            nam+=str(nobjects)
            os.rename(flow_fairness.get_latency_logfile_name(rate),nam);
            done[nobjects]=True
            plan.graph()
    
