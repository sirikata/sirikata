/*  cbr
 *  main.cpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "Timer.hpp"
#include "TimeSync.hpp"
#include "TimeProfiler.hpp"

#include "Network.hpp"

#include "Forwarder.hpp"

#include "Object.hpp"
#include "ObjectFactory.hpp"

#include "LocationService.hpp"

#include "Proximity.hpp"
#include "Server.hpp"

#include "Options.hpp"
#include "Statistics.hpp"
#include "Analysis.hpp"
#include "Visualization.hpp"
#include "OracleLocationService.hpp"
#include "StandardLocationService.hpp"
#include "Test.hpp"
#include "SSTNetwork.hpp"
#include "ENetNetwork.hpp"
#include "LossyQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "TabularServerIDMap.hpp"
#include "ExpIntegral.hpp"
#include "SqrIntegral.hpp"
#include "PartiallyOrderedList.hpp"
#include "UniformCoordinateSegmentation.hpp"
#include "DistributedCoordinateSegmentation.hpp"
#include "CoordinateSegmentationClient.hpp"
#include "LoadMonitor.hpp"
#include "LocObjectSegmentation.hpp"
#include "CraqObjectSegmentation.hpp"
#include "ServerProtocolMessagePair.hpp"



#include "ServerWeightCalculator.hpp"
namespace {
CBR::Network* gNetwork = NULL;
CBR::Trace* gTrace = NULL;
}

void *main_loop(void *);

bool is_analysis() {
    using namespace CBR;

    if (GetOption(ANALYSIS_LOC)->as<bool>() ||
        GetOption(ANALYSIS_LOCVIS)->as<String>() != "none" ||
        GetOption(ANALYSIS_LATENCY)->as<bool>() ||
        GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ||
        !GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>().empty() ||
        GetOption(ANALYSIS_OSEG)->as<bool>() )
        return true;

    return false;
}

int main(int argc, char** argv) {
    using namespace CBR;

    InitOptions();
    ParseOptions(argc, argv);

    std::string time_server=GetOption("time-server")->as<String>();
    TimeSync sync;

    if (GetOption("cseg")->as<String>() != "distributed")
      sync.start(time_server);


    ServerID server_id = GetOption("id")->as<ServerID>();
    String trace_file = is_analysis() ? "analysis.trace" : GetPerServerFile(STATS_TRACE_FILE, server_id);
    gTrace = new Trace(trace_file);

    String network_type = GetOption(NETWORK_TYPE)->as<String>();
    if (network_type == "sst")
        gNetwork = new SSTNetwork(gTrace);
    else if (network_type == "enet")
        gNetwork = new ENetNetwork(gTrace,65536,GetOption(RECEIVE_BANDWIDTH)->as<uint32>(),GetOption(SEND_BANDWIDTH)->as<uint32>());
    gNetwork->init(&main_loop);

    sync.stop();

    return 0;
}
void *main_loop(void *) {
    using namespace CBR;



    String test_mode = GetOption("test")->as<String>();
    if (test_mode != "none") {
        String server_port = GetOption("server-port")->as<String>();
        String client_port = GetOption("client-port")->as<String>();
        String host = GetOption("host")->as<String>();
        if (test_mode == "server")
            CBR::testServer(server_port.c_str(), host.c_str(), client_port.c_str());
        else if (test_mode == "client")
            CBR::testClient(client_port.c_str(), host.c_str(), server_port.c_str());
        return 0;
    }


    uint32 nobjects = GetOption("objects")->as<uint32>();
    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();


    uint32 max_space_servers = GetOption("max-servers")->as<uint32>();
    if (max_space_servers == 0)
      max_space_servers = layout.x * layout.y * layout.z;
    uint32 num_oh_servers = GetOption("num-oh")->as<uint32>();
    uint32 nservers = max_space_servers + num_oh_servers;



    Duration duration = GetOption("duration")->as<Duration>();

    float time_dilation = GetOption("time-dilation")->as<float>();
    float inv_time_dilation = 1.f / time_dilation;


    srand( GetOption("rand-seed")->as<uint32>() );


    // Compute the starting date/time
    String start_time_str = GetOption("wait-until")->as<String>();
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOption("wait-additional")->as<Duration>();


    ServerID server_id = GetOption("id")->as<ServerID>();

    Time init_space_ctx_time = Time::null() + (Timer::now() - start_time) * inv_time_dilation;
    SpaceContext* space_context = new SpaceContext(server_id, start_time, init_space_ctx_time, gTrace);

    Forwarder* forwarder = new Forwarder(space_context);

    ObjectFactory* obj_factory = new ObjectFactory(nobjects, region, duration);

    LocationService* loc_service = NULL;
    String loc_service_type = GetOption(LOC)->as<String>();
    if (loc_service_type == "oracle")
        loc_service = new OracleLocationService(space_context, obj_factory);
    else if (loc_service_type == "standard")
        loc_service = new StandardLocationService(space_context);
    else
        assert(false);

    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    gTrace->setServerIDMap(server_id_map);



    ServerMessageQueue* sq = NULL;
    String server_queue_type = GetOption(SERVER_QUEUE)->as<String>();
    if (server_queue_type == "fifo")
        sq = new FIFOServerMessageQueue(space_context, gNetwork, server_id_map, GetOption(SEND_BANDWIDTH)->as<uint32>(),GetOption(RECEIVE_BANDWIDTH)->as<uint32>());
    else if (server_queue_type == "fair")
        sq = new FairServerMessageQueue(space_context, gNetwork, server_id_map, GetOption(SEND_BANDWIDTH)->as<uint32>(),GetOption(RECEIVE_BANDWIDTH)->as<uint32>());
    else {
        assert(false);
        exit(-1);
    }


    String cseg_type = GetOption(CSEG)->as<String>();
    CoordinateSegmentation* cseg = NULL;
    if (cseg_type == "uniform")
        cseg = new UniformCoordinateSegmentation(space_context, region, layout);
    else if (cseg_type == "distributed") {
      cseg = new DistributedCoordinateSegmentation(space_context, region, layout, max_space_servers);
    }
    else if (cseg_type == "client") {
      cseg = new CoordinateSegmentationClient(space_context, region, layout);
    }
    else {
        assert(false);
        exit(-1);
    }


    LoadMonitor* loadMonitor = new LoadMonitor(space_context, sq, cseg);



    if ( GetOption(ANALYSIS_LOC)->as<bool>() ) {
        LocationErrorAnalysis lea(STATS_TRACE_FILE, nservers);
        printf("Total error: %f\n", (float)lea.globalAverageError( Duration::milliseconds((int64)10), obj_factory));
        exit(0);
    }
    else if ( GetOption(ANALYSIS_LOCVIS)->as<String>() != "none") {
        String vistype = GetOption(ANALYSIS_LOCVIS)->as<String>();
        LocationVisualization lea(STATS_TRACE_FILE, nservers, space_context, obj_factory,cseg);

        if (vistype == "object")
            lea.displayRandomViewerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((int64)30));
        else if (vistype == "server")
            lea.displayRandomServerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((int64)30));

        exit(0);
    }
    else if ( GetOption(ANALYSIS_LATENCY)->as<bool>() ) {
        LatencyAnalysis la(STATS_TRACE_FILE,nservers);

        exit(0);
    }
    else if ( GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ) {
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                ba.computeSendRate(sender, receiver);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                ba.computeReceiveRate(sender, receiver);
            }
        }

        ba.computeJFI(server_id);
        exit(0);
    }
    else if ( !GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>().empty() ) {
        String windowed_analysis_type = GetOption(ANALYSIS_WINDOWED_BANDWIDTH)->as<String>();

        String windowed_analysis_send_filename = "windowed_bandwidth_";
        windowed_analysis_send_filename += windowed_analysis_type;
        windowed_analysis_send_filename += "_send.dat";
        String windowed_analysis_receive_filename = "windowed_bandwidth_";
        windowed_analysis_receive_filename += windowed_analysis_type;
        windowed_analysis_receive_filename += "_receive.dat";

        String queue_info_filename = "queue_info_";
        queue_info_filename += windowed_analysis_type;
        queue_info_filename += ".dat";


        String windowed_queue_info_send_filename = "windowed_queue_info_send_";
        windowed_queue_info_send_filename += windowed_analysis_type;
        windowed_queue_info_send_filename += ".dat";
        String windowed_queue_info_receive_filename = "windowed_queue_info_receive_";
        windowed_queue_info_receive_filename += windowed_analysis_type;
        windowed_queue_info_receive_filename += ".dat";

        std::ofstream windowed_analysis_send_file(windowed_analysis_send_filename.c_str());
        std::ofstream windowed_analysis_receive_file(windowed_analysis_receive_filename.c_str());

        std::ofstream queue_info_file(queue_info_filename.c_str());

        std::ofstream windowed_queue_info_send_file(windowed_queue_info_send_filename.c_str());
        std::ofstream windowed_queue_info_receive_file(windowed_queue_info_receive_filename.c_str());

        Duration window = GetOption(ANALYSIS_WINDOWED_BANDWIDTH_WINDOW)->as<Duration>();
        Duration sample_rate = GetOption(ANALYSIS_WINDOWED_BANDWIDTH_RATE)->as<Duration>();
        BandwidthAnalysis ba(STATS_TRACE_FILE, max_space_servers);
        Time start_time = Time::null();
        Time end_time = start_time + duration;
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
            }
        }
        // Queue information
        //  * Raw dump
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.dumpDatagramQueueInfo(sender, receiver, std::cout, queue_info_file);
                else if (windowed_analysis_type == "packet")
                    ba.dumpPacketQueueInfo(sender, receiver, std::cout, queue_info_file);
            }
        }
        //  * Send
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
            }
        }
        //  * Receive
        for(ServerID sender = 1; sender <= max_space_servers; sender++) {
            for(ServerID receiver = 1; receiver <= max_space_servers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
            }
        }

        exit(0);
    }
    else if ( GetOption(ANALYSIS_OSEG)->as<bool>() ) {
        //bftm additional object messages log file creation.

        //oseg migrates
        String object_segmentation_filename = "object_segmentation_file";
        object_segmentation_filename += ".dat";

        ObjectSegmentationAnalysis osegAnalysis (STATS_TRACE_FILE, max_space_servers);
        std::ofstream object_seg_stream (object_segmentation_filename.c_str());
        osegAnalysis.printData(object_seg_stream);
        object_seg_stream.flush();
        object_seg_stream.close();


        //oseg lookup
        String object_segmentation_lookup_filename = "object_segmentation_lookup_file";
        object_segmentation_lookup_filename += ".dat";

        ObjectSegmentationLookupRequestsAnalysis lookupAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_lookup_stream(object_segmentation_lookup_filename.c_str());
        lookupAnalysis.printData(oseg_lookup_stream);
        oseg_lookup_stream.flush();
        oseg_lookup_stream.close();


        //oseg processed lookups
        String object_segmentation_processed_filename = "object_segmentation_processed_file";
        object_segmentation_processed_filename += ".dat";


        ObjectSegmentationProcessedRequestsAnalysis processedAnalysis(STATS_TRACE_FILE,max_space_servers);
        std::ofstream oseg_process_stream(object_segmentation_processed_filename.c_str());

        processedAnalysis.printData(oseg_process_stream);
        oseg_process_stream.flush();
        oseg_process_stream.close();


        //end bftm additional object message log file creation.

        exit(0);
    }





    //Create OSeg
    std::string oseg_type=GetOption(OSEG)->as<String>();

    ObjectSegmentation* oseg;

    if (oseg_type == OSEG_OPTION_LOC)
    {
      //using loc approach
      std::map<UUID,ServerID> dummyObjectToServerMap; //bftm note: this should be filled in later with a list of object ids and where they are located

      //Trying to populate objectToServerMap
      // FIXME this needs to go away, we can't rely on the object factory being there
      for(ObjectFactory::iterator it = obj_factory->begin(); it != obj_factory->end(); it++)
      {
        UUID obj_id = *it;
        Vector3f start_pos = obj_factory->motion(obj_id)->initial().extrapolate(Time::null()).position();
        dummyObjectToServerMap[obj_id] = cseg->lookup(start_pos);
      }

      //      ObjectSegmentation* oseg = new LocObjectSegmentation(space_context, cseg,loc_service,dummyObjectToServerMap);
      oseg = new LocObjectSegmentation(space_context, cseg,loc_service,dummyObjectToServerMap);
    }

    if (oseg_type == OSEG_OPTION_CRAQ)
    {
      //using craq approach
      std::vector<UUID> initServObjVec;
      for(ObjectFactory::iterator it = obj_factory->begin(); it != obj_factory->end(); it++)
      {
        UUID obj_id = *it;
        Vector3f start_pos = loc_service->currentPosition(obj_id);

        if (cseg->lookup(start_pos) == server_id)
        {
          initServObjVec.push_back(obj_id);
          std::cout<<obj_id.toString()<<"\n";
        }
      }

     std::vector<CraqInitializeArgs> craqArgs;
     CraqInitializeArgs cInitArgs;

     cInitArgs.ipAdd = "localhost";
     cInitArgs.port  =     "10299";
     craqArgs.push_back(cInitArgs);

     std::string oseg_craq_prefix=GetOption(OSEG_UNIQUE_CRAQ_PREFIX)->as<String>();

     if (oseg_type.size() ==0)
     {
       std::cout<<"\n\nERROR: Incorrect craq prefix for oseg.  String must be at least one letter long.  (And be between G and Z.)  Please try again.\n\n";
       assert(false);
     }

     std::cout<<"\n\nUniquely appending  "<<oseg_craq_prefix[0]<<"\n\n";
     oseg = new CraqObjectSegmentation (space_context, cseg, initServObjVec, craqArgs, oseg_craq_prefix[0]);


    }      //end craq approach

    //end create oseg



    ObjectMessageQueue* oq = NULL;
    String object_queue_type = GetOption(OBJECT_QUEUE)->as<String>();
    if (object_queue_type == "fifo")
        oq = new FIFOObjectMessageQueue(space_context, forwarder, GetOption(SEND_BANDWIDTH)->as<uint32>());
    else if (object_queue_type == "fairfifo")
        oq = new FairObjectMessageQueue<Queue<ServerProtocolMessagePair*> > (space_context, forwarder, GetOption(SEND_BANDWIDTH)->as<uint32>());
    else if (object_queue_type == "fairlossy")
        oq = new FairObjectMessageQueue<LossyQueue<ServerProtocolMessagePair*> > (space_context, forwarder, GetOption(SEND_BANDWIDTH)->as<uint32>());
    else if (object_queue_type == "fairreorder")
        oq = new FairObjectMessageQueue<PartiallyOrderedList<ServerProtocolMessagePair*,ServerID > >(space_context, forwarder, GetOption(SEND_BANDWIDTH)->as<uint32>());
    else {
        assert(false);
        exit(-1);
    }
    ServerWeightCalculator* weight_calc = NULL;
    if (cseg_type != "distributed") {
      if (GetOption("gaussian")->as<bool>()) {
        weight_calc =
        new ServerWeightCalculator(
            server_id,
            cseg,

            std::tr1::bind(&integralExpFunction,GetOption("flatness")->as<double>(),
                           std::tr1::placeholders::_1,
                           std::tr1::placeholders::_2,
                           std::tr1::placeholders::_3,
                           std::tr1::placeholders::_4),
            sq
        );
      }else {
        weight_calc =
        new ServerWeightCalculator(
            server_id,
            cseg,
            std::tr1::bind(SqrIntegral(false),GetOption("const-cutoff")->as<double>(),GetOption("flatness")->as<double>(),
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3,
                           std::tr1::placeholders::_4),
            sq);
      }
    }

    Proximity* prox = new Proximity(space_context, loc_service);


    //    Server* server = new Server(server_id, loc_service, cseg, prox, oq, sq, loadMonitor, gTrace);
    Server* server = new Server(space_context, forwarder, loc_service, cseg, prox, oq, sq, loadMonitor,oseg, server_id_map);

      prox->initialize(cseg);

      // NOTE: we don't initialize this because nothing should require it. We no longer have object host,
      // and are only maintaining ObjectFactory for analysis, visualization, and oracle purposes.
      //obj_factory->initialize(obj_host->context());

    // If we're one of the initial nodes, we'll have to wait until we hit the start time
    {
        Time now_time = Timer::now();
        if (start_time > now_time) {
            Duration sleep_time = start_time - now_time;
            printf("Waiting %f seconds\n", sleep_time.toSeconds() ); fflush(stdout);
            usleep( sleep_time.toMicroseconds() );
        }
    }

    ///////////Go go go!! start of simulation/////////////////////

    Time tbegin = Time::null();
    Time tend = tbegin + duration;

    Duration stats_sample_rate = GetOption(STATS_SAMPLE_RATE)->as<Duration>();
    Time last_sample_time = Time::null();


    // FIXME we have a special case for the distributed cseg server, this should be
    // turned into a separate binary
    if (cseg_type == "distributed") {
      while( true ) {
        Duration elapsed = (Timer::now() - start_time) * inv_time_dilation;

        space_context->tick(tbegin + elapsed);
        cseg->service();
      }

      exit(0);
    }


    gNetwork->start();

    TimeProfiler whole_profiler("Whole Main Loop");
    whole_profiler.addStage("Loop");

    TimeProfiler profiler("Main Loop");
    profiler.addStage("Context Update");
    profiler.addStage("Network Service");
    profiler.addStage("CSeg Service");
    profiler.addStage("Server Service");

    while( true ) {
        Duration elapsed = (Timer::now() - start_time) * inv_time_dilation;
        if (elapsed > duration)
            break;

        Time curt = tbegin + elapsed;
        Duration since_last_sample = curt - last_sample_time;
        if (since_last_sample > stats_sample_rate) {
            gNetwork->reportQueueInfo(curt);
            last_sample_time = last_sample_time + stats_sample_rate;
        }

        whole_profiler.startIteration();
        profiler.startIteration();

        space_context->tick(curt); profiler.finishedStage();
        gNetwork->service(curt); profiler.finishedStage();
        cseg->service(); profiler.finishedStage();
        server->service(); profiler.finishedStage();

        whole_profiler.finishedStage();
    }

    if (GetOption(PROFILE)->as<bool>()) {
        whole_profiler.report();
        profiler.report();
    }

    gTrace->prepareShutdown();

    prox->shutdown();

    delete server;
    delete sq;
    delete prox;
    delete server_id_map;
    if (weight_calc != NULL)
      delete weight_calc;
    delete cseg;
    delete loc_service;
    delete obj_factory;
    delete forwarder;

    delete gNetwork;
    gNetwork=NULL;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;

    delete space_context;
    space_context = NULL;

    return 0;
}
