/*  Sirikata
 *  Options.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Options.hpp"

namespace Sirikata {

void InitOptions() {
    InitializeOptions::module(CBR_MODULE)
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_defaultLevel) )
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_atLeastLevel) )
        .addOption( reinterpret_cast<Sirikata::OptionValue*>(Sirikata_Logging_OptionValue_moduleLevel) )
        .addOption(new OptionValue("ohstreamlib","tcpsst",Sirikata::OptionValueType<String>(),"Which library to use to communicate with the object host"))
        .addOption(new OptionValue("ohstreamoptions","--send-buffer-size=32768 --parallel-sockets=1 --no-delay=true",Sirikata::OptionValueType<String>(),"TCPSST stream options such as how many bytes to collect for sending during an ongoing asynchronous send call."))
        .addOption(new OptionValue("spacestreamlib","tcpsst",Sirikata::OptionValueType<String>(),"Which library to use to communicate with the object host"))
        .addOption(new OptionValue("spacestreamoptions","--send-buffer-size=32768 --parallel-sockets=1 --no-delay=true",Sirikata::OptionValueType<String>(),"TCPSST stream options such as how many bytes to collect for sending during an ongoing asynchronous send call."))
        .addOption(new OptionValue("test", "none", Sirikata::OptionValueType<String>(), "Type of test to run"))

        .addOption(new OptionValue("falloff", "sqr", Sirikata::OptionValueType<String>(), "Type of communication falloff function to use.  Valid values are sqr and guassian. Default is sqr."))
        .addOption(new OptionValue("flatness", "8", Sirikata::OptionValueType<double>(), "k where e^-kx is the bandwidth function and x is the distance between 2 server points"))
        .addOption(new OptionValue("const-cutoff", "64", Sirikata::OptionValueType<double>(), "cutoff below with a constant bandwidth is used"))

        .addOption(new OptionValue("server-port", "8080", Sirikata::OptionValueType<String>(), "Port for server side of test"))
        .addOption(new OptionValue("client-port", "8081", Sirikata::OptionValueType<String>(), "Port for client side of test"))
        .addOption(new OptionValue("host", "127.0.0.1", Sirikata::OptionValueType<String>(), "Host to connect to for test"))

        .addOption(new OptionValue("id", "1", Sirikata::OptionValueType<ServerID>(), "Server ID for this server"))
        .addOption(new OptionValue("ohid", "1", Sirikata::OptionValueType<ObjectHostID>(), "Object host ID for this server"))
        .addOption(new OptionValue("region", "<<-100,-100,-100>,<100,100,100>>", Sirikata::OptionValueType<BoundingBox3f>(), "Simulation region"))
        .addOption(new OptionValue("layout", "<2,1,1>", Sirikata::OptionValueType<Vector3ui32>(), "Layout of servers in uniform grid - ixjxk servers"))
        .addOption(new OptionValue("max-servers", "0", Sirikata::OptionValueType<uint32>(), "Maximum number of servers available for the simulation; if set to 0, use the number of servers specified in the layout option"))
        .addOption(new OptionValue("num-oh", "0", Sirikata::OptionValueType<uint32>(), "Number of object hosts being used during this simulation"))
        .addOption(new OptionValue("duration", "1s", Sirikata::OptionValueType<Duration>(), "Duration of the simulation"))
        .addOption(new OptionValue("serverips", "serverip.txt", Sirikata::OptionValueType<String>(), "The file containing the server ip list"))

        .addOption(new OptionValue(SEND_BANDWIDTH, "2000000000", Sirikata::OptionValueType<uint32>(), "Total outbound bandwidth for this server in bytes per second")) // FIXME remove
        .addOption(new OptionValue(RECEIVE_BANDWIDTH, "2000000000", Sirikata::OptionValueType<uint32>(), "Total inbound bandwidth for this server in bytes per second")) // FIXME remove
        .addOption(new OptionValue("capexcessbandwidth", "false", Sirikata::OptionValueType<bool>(), "Total bandwidth for this server in bytes per second"))

        .addOption(new OptionValue("rand-seed", "0", Sirikata::OptionValueType<uint32>(), "The random seed to synchronize all servers"))

        .addOption(new OptionValue(STATS_TRACE_FILE, "trace.txt", Sirikata::OptionValueType<String>(), "The filename to save the trace to"))
        .addOption(new OptionValue(STATS_OH_TRACE_FILE, "trace.txt", Sirikata::OptionValueType<String>(), "The filename to save the trace to"))
        .addOption(new OptionValue(STATS_SAMPLE_RATE, "250ms", Sirikata::OptionValueType<Duration>(), "Frequency to sample non-event statistics such as queue information."))

        .addOption(new OptionValue("time-server", "ahoy.stanford.edu", Sirikata::OptionValueType<String>(), "The server to sync with"))
        .addOption(new OptionValue("wait-until","",Sirikata::OptionValueType<String>(),"The date to wait until before starting"))
        .addOption(new OptionValue("wait-additional","0s",Sirikata::OptionValueType<Duration>(),"How much additional time after date has passed to wait until before starting"))

        .addOption(new OptionValue(MAX_EXTRAPOLATOR_DIST, "1.0", Sirikata::OptionValueType<float64>(), "The maximum distance an object is permitted to deviate from the predictions by other objects before an update is sent out."))

        .addOption(new OptionValue(ANALYSIS_LOC, "false", Sirikata::OptionValueType<bool>(), "Do a loc analysis instead of a normal run"))
        .addOption(new OptionValue(ANALYSIS_LOCVIS, "none", Sirikata::OptionValueType<String>(), "The type of visualization to run, none to disable."))
        .addOption(new OptionValue(ANALYSIS_LOCVIS_SEED, "5", Sirikata::OptionValueType<int>(), "Do a loc analysis on this object"))

        .addOption(new OptionValue(ANALYSIS_BANDWIDTH, "false", Sirikata::OptionValueType<bool>(), "Do a bandwidth analysis instead of a normal run"))
        .addOption(new OptionValue(ANALYSIS_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Do a latency analysis instead of a normal run"))

        .addOption(new OptionValue(ANALYSIS_OBJECT_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Do a object distance latency analysis instead of a normal run"))
        .addOption(new OptionValue(ANALYSIS_MESSAGE_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Do a message stage latency analysis instead of a normal run"))

        .addOption(new OptionValue(ANALYSIS_WINDOWED_BANDWIDTH, "", Sirikata::OptionValueType<String>(), "Do a windowed bandwidth analysis of the specified type: datagram, packet"))
        .addOption(new OptionValue(ANALYSIS_WINDOWED_BANDWIDTH_WINDOW, "2000ms", Sirikata::OptionValueType<Duration>(), "Size of the window in windowed bandwidth analysis"))
        .addOption(new OptionValue(ANALYSIS_WINDOWED_BANDWIDTH_RATE, "10ms", Sirikata::OptionValueType<Duration>(), "Frequency of samples in windowed bandwidth analysis, i.e. how much to slide the window by"))

        .addOption(new OptionValue(ANALYSIS_OSEG, "false", Sirikata::OptionValueType<bool>(), "Run OSEG analyses - migrates, lookups, processed lookups"))
      .addOption(new OptionValue(OSEG_ANALYZE_AFTER,"0", Sirikata::OptionValueType<int>(),"Only run the oseg analysis after this many seconds of the run have elapsed."))


        .addOption(new OptionValue(ANALYSIS_LOC_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Run location latency analysis - latency of location updates"))

        .addOption(new OptionValue(ANALYSIS_PROX_DUMP, "", Sirikata::OptionValueType<String>(), "Run proximity dump analysis -- just dumps a textual form of all proximity events to the specified file"))

        .addOption(new OptionValue(ANALYSIS_FLOW_STATS, "false", Sirikata::OptionValueType<bool>(), "Get summary object pair flow statistics"))

        .addOption(new OptionValue(OBJECT_NUM_RANDOM, "100", Sirikata::OptionValueType<uint32>(), "Number of random objects to generate."))

        .addOption(new OptionValue(OBJECT_CONNECT_PHASE, "0s", Sirikata::OptionValueType<Duration>(), "Length of time to initiate connections over. Connection requests will be uniformly distributed."))
        .addOption(new OptionValue(OBJECT_STATIC, "random", Sirikata::OptionValueType<String>(), "Whether objects should be static (static) or move randomly (randome) or drift in one direction (drift)."))
      .addOption(new OptionValue(OBJECT_DRIFT_X, "0",Sirikata::OptionValueType<float>(), "If select drift for motion path (under OBJECT_STATIC), then this is the x component of all objects' drifts"))
      .addOption(new OptionValue(OBJECT_DRIFT_Y, "0",Sirikata::OptionValueType<float>(), "If select drift for motion path (under OBJECT_STATIC), then this is the y component of all objects' drifts"))
      .addOption(new OptionValue(OBJECT_DRIFT_Z, "0",Sirikata::OptionValueType<float>(), "If select drift for motion path (under OBJECT_STATIC), then this is the z component of all objects' drifts"))

        .addOption(new OptionValue(OBJECT_SIMPLE, "false", Sirikata::OptionValueType<bool>(), "Simple object distribution - all the same size, useful for sanity checking queries"))
        .addOption(new OptionValue(OBJECT_2D, "false", Sirikata::OptionValueType<bool>(), "Constrain location and motion to just 2 dimensions."))
        .addOption(new OptionValue(OBJECT_QUERY_FRAC, "0.1", Sirikata::OptionValueType<float>(), "Percent of objects which should issue prox queries."))

        .addOption(new OptionValue(OBJECT_PACK_DIR, "", Sirikata::OptionValueType<String>(), "Directory to store and load pack files from."))
        .addOption(new OptionValue(OBJECT_PACK, "", Sirikata::OptionValueType<String>(), "Filename of the object pack to use to generate objects."))
        .addOption(new OptionValue(OBJECT_PACK_OFFSET, "0", Sirikata::OptionValueType<uint32>(), "Offset into the object pack to start generating objects at."))
        .addOption(new OptionValue(OBJECT_PACK_NUM, "0", Sirikata::OptionValueType<uint32>(), "Number of objects to load from a pack file."))
        .addOption(new OptionValue(OBJECT_PACK_DUMP, "", Sirikata::OptionValueType<String>(), "If non-empty, dumps any generated objects to the specified file."))

        .addOption(new OptionValue(OBJECT_SL_FILE, "", Sirikata::OptionValueType<String>(), "Filename of the object pack to use to generate objects."))
        .addOption(new OptionValue(OBJECT_SL_NUM, "0", Sirikata::OptionValueType<uint32>(), "Number of objects to load from a pack file."))
        .addOption(new OptionValue(OBJECT_SL_CENTER, "", Sirikata::OptionValueType<Vector3f>(), "The center point to start adding SL objects from. They will be added in order of increasing distance from this point."))


        .addOption(new OptionValue(SERVER_QUEUE, "fair", Sirikata::OptionValueType<String>(), "The type of ServerMessageQueue to use for routing."))
        .addOption(new OptionValue(SERVER_QUEUE_LENGTH, "8192", Sirikata::OptionValueType<uint32>(), "Length of queue for each server."))
        .addOption(new OptionValue(SERVER_RECEIVER, "fair", Sirikata::OptionValueType<String>(), "The type of ServerMessageReceiver to use for routing."))
        .addOption(new OptionValue(SERVER_ODP_FLOW_SCHEDULER, "region", Sirikata::OptionValueType<String>(), "The type of ODPFlowScheduler to use for routing."))
        .addOption(new OptionValue(FORWARDER_RECEIVE_QUEUE_SIZE, "16384", Sirikata::OptionValueType<uint32>(), "The type of ODPFlowScheduler to use for routing."))
        .addOption(new OptionValue(FORWARDER_SEND_QUEUE_SIZE, "65536", Sirikata::OptionValueType<uint32>(), "The type of ODPFlowScheduler to use for routing."))

        .addOption(new OptionValue(NETWORK_TYPE, "tcp", Sirikata::OptionValueType<String>(), "The networking subsystem to use."))
        .addOption(new OptionValue("monitor-load", "false", Sirikata::OptionValueType<bool>(), "Does the LoadMonitor monitor queue sizes?"))


        .addOption(new OptionValue(OSEG,OSEG_OPTION_CRAQ,Sirikata::OptionValueType<String>(),"Specifies which type of oseg to use."))

        .addOption(new OptionValue(OSEG_UNIQUE_CRAQ_PREFIX,"G",Sirikata::OptionValueType<String>(),"Specifies a unique character prepended to Craq lookup calls.  Note: takes in type string.  Will only select the first character in the string.  Also note that it acceptable values range from g to z, and are case sensitive."))


        .addOption(new OptionValue(OSEG_LOOKUP_QUEUE_SIZE, "2000", Sirikata::OptionValueType<uint32>(), "Number of new lookups you can have on oseg lookup queue."))

        .addOption(new OptionValue(OSEG_CACHE_SIZE, "200", Sirikata::OptionValueType<uint32>(), "Maximum number of entries in the OSeg cache."))

        .addOption(new OptionValue(CACHE_SELECTOR,CACHE_TYPE_ORIGINAL_LRU,Sirikata::OptionValueType<String>(),"Which caching algorithm to use."))

         .addOption(new OptionValue(CACHE_COMM_SCALING,"1.0",Sirikata::OptionValueType<double>(),"What the communication falloff function scaling factor is."))
         .addOption(new OptionValue("send-capacity-overestimate","1",Sirikata::OptionValueType<double>(),"How much to overestimate send capacity when queue is not blocked."))
         .addOption(new OptionValue("receive-capacity-overestimate","1",Sirikata::OptionValueType<double>(),"How much to overestimate recv capacity when queue is not blocked."))
        .addOption(new OptionValue(OSEG_CACHE_CLEAN_GROUP_SIZE, "25", Sirikata::OptionValueType<uint32>(), "Number of items to remove from the OSeg cache when it reaches the maximum size."))
        .addOption(new OptionValue(OSEG_CACHE_ENTRY_LIFETIME, "8s", Sirikata::OptionValueType<Duration>(), "Maximum lifetime for an OSeg cache entry."))

        .addOption(new OptionValue(CSEG, "uniform", Sirikata::OptionValueType<String>(), "Type of Coordinate Segmentation implementation to use."))
        .addOption(new OptionValue(LOC, "standard", Sirikata::OptionValueType<String>(), "Type of location service to run."))
        .addOption(new OptionValue(LOC_MAX_PER_RESULT, "10", Sirikata::OptionValueType<uint32>(), "Maximum number of loc updates to report in each result message."))
        .addOption(new OptionValue(PROX_MAX_PER_RESULT, "10", Sirikata::OptionValueType<uint32>(), "Maximum number of changes to report in each result message."))
        .addOption(new OptionValue(PROFILE, "false", Sirikata::OptionValueType<bool>(), "Whether to report profiling information."))
        .addOption(new OptionValue("random-splits-merges", "false", Sirikata::OptionValueType<bool>(), "Whether to enable random splits and merges in DistributedCoordinateSegmentation."))
        .addOption(new OptionValue("cseg-service-host", "meru00", Sirikata::OptionValueType<String>(), "Hostname of machine running the CSEG service (running with --cseg=distributed)"))
        .addOption(new OptionValue("cseg-service-tcp-port", "2234", Sirikata::OptionValueType<String>(), "TCP listening port number on host running the CSEG service (running with --cseg=distributed)"))
        .addOption(new OptionValue("cseg-server-ll-port", "3234", Sirikata::OptionValueType<uint16>(), "Port where CSEG servers can be contacted for lower-tree requests."))
        .addOption(new OptionValue("num-cseg-servers", "1", Sirikata::OptionValueType<uint16>(), "Number of CSEG servers for the distributed implementation."))

        .addOption(new OptionValue("cseg-uses-world-pop", "false", Sirikata::OptionValueType<bool>(), "If true, CSEG uses the world population data to create the BSP tree."))
        .addOption(new OptionValue("cseg-world-width", "8640", Sirikata::OptionValueType<uint32>(), "The number of cells across the width of the world population dataset."))
        .addOption(new OptionValue("cseg-world-height", "3432", Sirikata::OptionValueType<uint32>(), "The number of cells across the height of the world population dataset."))
        .addOption(new OptionValue("cseg-max-leaf-population", "800", Sirikata::OptionValueType<uint32>(), "The maximum number of avatars/people at the leaf of the BSP tree."))
      .addOption(new OptionValue("cseg-population-density-file", "glds00ag.asc", Sirikata::OptionValueType<String>(), "The file containing the population density numbers."))
      .addOption(new OptionValue("cseg-serverips", "cseg_serverip.txt", Sirikata::OptionValueType<String>(), "The file containing the server ip list for cseg servers."))
      .addOption(new OptionValue("cseg-id", "1", Sirikata::OptionValueType<ServerID>(), "Server ID for this CSEG server"))
      .addOption(new OptionValue("additional-cseg-duration", "5s", Sirikata::OptionValueType<Duration>(), "Additional duration to run CSEG after the simulation"))
      .addOption(new OptionValue("scenario", "ping", Sirikata::OptionValueType<String>(), "ObjectHost-wide script dictating mass wide object behaviors"))
      .addOption(new OptionValue("scenario-options", "", Sirikata::OptionValueType<String>(), "Options for ObjectHost-wide script dictating mass wide object behaviors"))
      .addOption(new OptionValue("object-host-receive-buffer", "32768", Sirikata::OptionValueType<size_t>(), "size of the object host space node connection receive queue"))
      .addOption(new OptionValue("object-host-send-buffer", "32768", Sirikata::OptionValueType<size_t>(), "size of the object host space node cnonection send queue"))
      .addOption(new OptionValue("route-object-message-buffer", "64", Sirikata::OptionValueType<size_t>(), "size of the buffer between network and main strand for space server message routing"))

      ;
}

void ParseOptions(int argc, char** argv) {
    OptionSet* options = OptionSet::getOptions(CBR_MODULE,NULL);
    options->parse(argc, argv);
}

OptionValue* GetOption(const char* name) {
    OptionSet* options = OptionSet::getOptions(CBR_MODULE,NULL);
    return options->referenceOption(name);
}

// FIXME method naming
String GetPerServerString(const String& orig, const ServerID& sid) {
    int32 dot_pos = orig.rfind(".");
    String prefix = orig.substr(0, dot_pos);
    String ext = orig.substr(dot_pos+1);

    char buffer[1024];
    sprintf(buffer, "%s-%04d.%s", prefix.c_str(), (uint32)sid, ext.c_str());

    return buffer;
}

String GetPerServerFile(const char* opt_name, const ServerID& sid) {
    String orig = GetOption(opt_name)->as<String>();
    return GetPerServerString(orig, sid);
}

String GetPerServerFile(const char* opt_name, const ObjectHostID& ohid) {
    return (GetPerServerFile(opt_name, (ServerID)ohid.id)); // FIXME relies on fact that ServerID and ObjectHostID are both uint64
}

} // namespace Sirikata
