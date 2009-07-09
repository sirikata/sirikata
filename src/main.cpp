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
#include "Network.hpp"
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
#include "Test.hpp"
#include "RaknetNetwork.hpp"
#include "SSTNetwork.hpp"
#include "LossyQueue.hpp"
#include "FIFOObjectMessageQueue.hpp"
#include "FIFOServerMessageQueue.hpp"
#include "FairServerMessageQueue.hpp"
#include "FairObjectMessageQueue.hpp"
#include "TabularServerIDMap.hpp"
#include "ExpIntegral.hpp"
#include "PartiallyOrderedList.hpp"
#include "UniformCoordinateSegmentation.hpp"
#include "LBCoordinateSegmentation.hpp"
#include "LoadMonitor.hpp"

#include "LocObjectSegmentation.hpp"
#include "UniformObjectSegmentation.hpp"
//#include "ChordObjectSegmentation.hpp"

#include "OSegHasher.hpp"


#include "ServerWeightCalculator.hpp"
namespace {
CBR::Network* gNetwork = NULL;
CBR::Trace* gTrace = NULL;
}
void *main_loop(void *);
int main(int argc, char** argv) {
    using namespace CBR;

        
    InitOptions();
    ParseOptions(argc, argv);
    std::string time_server=GetOption("time-server")->as<String>();
    int ntppipes[2];
    pipe(ntppipes);
    if (0==fork()) {
        close(ntppipes[0]);
        close(1);
        dup2(ntppipes[1],1);
        execlp("ntpdate","ntpdate","-q","-p","8",time_server.c_str(),NULL);
    }
    {
        FILE * ntp=fdopen(ntppipes[0],"rb");
        int stratum;
        float offset;
        float delay;
        int ip[4];
        if (3==fscanf(ntp,"server %*s stratum %d, offset %f, delay %f\n",&stratum,&offset,&delay)) {
            //printf ("Match offset %f\n",offset);
            Timer::setSystemClockOffset(Duration::seconds((float32)offset));
        }
        fclose(ntp);
        close(ntppipes[0]);
        close(ntppipes[1]);
    }

    gTrace = new Trace();

    String network_type = GetOption(NETWORK_TYPE)->as<String>();
    if (network_type == "raknet")
        gNetwork = new RaknetNetwork();
    else if (network_type == "sst")
        gNetwork = new SSTNetwork(gTrace);
    gNetwork->init(&main_loop);
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


    MaxDistUpdatePredicate::maxDist = GetOption(MAX_EXTRAPOLATOR_DIST)->as<float64>();

    
    uint32 nobjects = GetOption("objects")->as<uint32>();
    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();


    
    uint32 nservers = GetOption("max-servers")->as<uint32>();
    if (nservers == 0) {
      nservers = layout.x * layout.y * layout.z;
    }



    Duration duration = GetOption("duration")->as<Duration>();

    srand( GetOption("rand-seed")->as<uint32>() );

    ServerID server_id = GetOption("id")->as<ServerID>();
    ObjectFactory* obj_factory = new ObjectFactory(nobjects, region, duration);
    LocationService* loc_service = new OracleLocationService(obj_factory);


    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    gTrace->setServerIDMap(server_id_map);
    Proximity* prox = new Proximity(obj_factory, loc_service);


    
    ServerMessageQueue* sq = NULL;
    String server_queue_type = GetOption(SERVER_QUEUE)->as<String>();
    if (server_queue_type == "fifo")
        sq = new FIFOServerMessageQueue(gNetwork,GetOption(RECEIVE_BANDWIDTH)->as<uint32>(), server_id, server_id_map, gTrace);
    else if (server_queue_type == "fair")
        sq = new FairServerMessageQueue(gNetwork, GetOption(SEND_BANDWIDTH)->as<uint32>(),GetOption(RECEIVE_BANDWIDTH)->as<uint32>(), server_id, server_id_map, gTrace);
    else {
        assert(false);
        exit(-1);
    }


    
    CoordinateSegmentation* cseg=//new LBCoordinateSegmentation(server_id, region, layout, sq, gTrace);
                                 new UniformCoordinateSegmentation(region, layout);


    
    LoadMonitor* loadMonitor = new LoadMonitor(server_id, sq, cseg);


    
    if ( GetOption(ANALYSIS_LOC)->as<bool>() ) {
        LocationErrorAnalysis lea(STATS_TRACE_FILE, nservers);
        printf("Total error: %f\n", (float)lea.globalAverageError( Duration::milliseconds((uint32)10), obj_factory));
        exit(0);
    }
    else if ( GetOption(ANALYSIS_LOCVIS)->as<bool>() ) {
        LocationVisualization lea(STATS_TRACE_FILE, nservers, obj_factory,loc_service,cseg);
        lea.displayRandomViewerError(GetOption(ANALYSIS_LOCVIS_SEED)->as<int>(), Duration::milliseconds((uint32)30));
        exit(0);
    }
    else if ( GetOption(ANALYSIS_LATENCY)->as<bool>() ) {
        LatencyAnalysis la(STATS_TRACE_FILE,nservers);
        
        exit(0);
    }
    else if ( GetOption(ANALYSIS_BANDWIDTH)->as<bool>() ) {
        BandwidthAnalysis ba(STATS_TRACE_FILE, nservers);
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
                ba.computeSendRate(sender, receiver);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
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
        BandwidthAnalysis ba(STATS_TRACE_FILE, nservers);
        Time start_time(0);
        Time end_time = start_time + duration;
        printf("Send rates\n");
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketSendRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_send_file);
            }
        }
        printf("Receive rates\n");
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.computeWindowedDatagramReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.computeWindowedPacketReceiveRate(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_analysis_receive_file);
            }
        }
        // Queue information
        //  * Raw dump
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.dumpDatagramQueueInfo(sender, receiver, std::cout, queue_info_file);
                else if (windowed_analysis_type == "packet")
                    ba.dumpPacketQueueInfo(sender, receiver, std::cout, queue_info_file);
            }
        }
        //  * Send
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketSendQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_send_file);
            }
        }
        //  * Receive
        for(ServerID sender = 1; sender <= nservers; sender++) {
            for(ServerID receiver = 1; receiver <= nservers; receiver++) {
                if (windowed_analysis_type == "datagram")
                    ba.windowedDatagramReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
                else if (windowed_analysis_type == "packet")
                    ba.windowedPacketReceiveQueueInfo(sender, receiver, window, sample_rate, start_time, end_time, std::cout, windowed_queue_info_receive_file);
            }
        }

        //bftm additional object messages log file creation.


        String object_segmentation_filename = "object_segmentation_file";
        object_segmentation_filename += ".dat";
        
        ObjectSegmentationAnalysis osegAnalysis (STATS_TRACE_FILE, nservers);
        std::ofstream object_seg_stream (object_segmentation_filename.c_str());
        osegAnalysis.printData(object_seg_stream);
        object_seg_stream.flush();
        object_seg_stream.close();

        //end bftm additional object message log file creation.
        
        exit(0);
    }



    ObjectMessageQueue* oq = NULL;
    String object_queue_type = GetOption(OBJECT_QUEUE)->as<String>();
    if (object_queue_type == "fifo")
        oq = new FIFOObjectMessageQueue(sq, loc_service, cseg, GetOption(SEND_BANDWIDTH)->as<uint32>(), gTrace);
    else if (object_queue_type == "fairfifo")
        oq = new FairObjectMessageQueue<Queue<FairObjectMessageNamespace::ServerMessagePair*> > (sq, loc_service, cseg, GetOption(SEND_BANDWIDTH)->as<uint32>(),gTrace);
    else if (object_queue_type == "fairlossy")
        oq = new FairObjectMessageQueue<LossyQueue<FairObjectMessageNamespace::ServerMessagePair*> > (sq, loc_service, cseg, GetOption(SEND_BANDWIDTH)->as<uint32>(),gTrace);
    else if (object_queue_type == "fairreorder")
        oq = new FairObjectMessageQueue<PartiallyOrderedList<FairObjectMessageNamespace::ServerMessagePair*,ServerID > >(sq, loc_service, cseg, GetOption(SEND_BANDWIDTH)->as<uint32>(),gTrace);
    else {
        assert(false);
        exit(-1);
    }


    obj_factory->setObjectMessageQueue(oq);
    ServerWeightCalculator* weight_calc =
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

    
    //Create OSeg
    std::vector<ServerID> dummyServerList; //bftm note: this should be filled in later with a list of server names.
    std::map<UUID,ServerID> dummyObjectToServerMap; //bftm note: this should be filled in later with a list of object ids and where they are located


    
    //Trying to populate objectToServerMap
      for(ObjectFactory::iterator it = obj_factory->begin(); it != obj_factory->end(); it++)
      {
        UUID obj_id = *it;
        Vector3f start_pos = loc_service->currentPosition(obj_id);
        dummyObjectToServerMap[obj_id] = cseg->lookup(start_pos);
      }


      
    //End of populating objectToServerMap
      
      //      ObjectSegmentation* oseg = new ChordObjectSegmentation(cseg,dummyObjectToServerMap,server_id, gTrace);
      //      ObjectSegmentation* oseg = new UniformObjectSegmentation(cseg,dummyObjectToServerMap,server_id, gTrace);
      ObjectSegmentation* oseg = new LocObjectSegmentation(cseg,loc_service,dummyObjectToServerMap);
    
    //end create oseg

    
    //    Server* server = new Server(server_id, obj_factory, loc_service, cseg, prox, oq, sq, loadMonitor, gTrace);
    Server* server = new Server(server_id, obj_factory, loc_service, cseg, prox, oq, sq, loadMonitor, gTrace,oseg);

    bool sim = GetOption("sim")->as<bool>();
    Duration sim_step = GetOption("sim-step")->as<Duration>();

    float time_dilation = GetOption("time-dilation")->as<float>();
    float inv_time_dilation = 1.f / time_dilation;

    ///////////Wait until start of simulation/////////////////////


    {
        Duration waiting_time=Duration::seconds(GetOption("wait-additional")->as<float>());
        if (GetOption("wait-until")->as<String>().length()) {
            waiting_time+=(Timer::getSpecifiedDate(GetOption("wait-until")->as<String>())-Timer::now());
        }
        float32 waitin=waiting_time.milliseconds();
        printf ("waiting for %f seconds\n",waitin/1000.);
        if (waitin>0)
            usleep(waitin*1000.);
    }

    ///////////Go go go!! start of simulation/////////////////////

    Time tbegin = Time(0);
    Time tend = tbegin + duration;

    if (sim) {

        for(Time t = tbegin; t < tend; t += sim_step){
	    server->tick(t);
        }
    }
    else {
      Timer timer;
        timer.start();
        gNetwork->start();

        while( true ) {
            Duration elapsed = timer.elapsed() * inv_time_dilation;
            if (elapsed > duration)
                break;
            gNetwork->reportQueueInfo(tbegin + elapsed);
            gNetwork->service(tbegin + elapsed);
            cseg->tick(tbegin + elapsed);
            server->tick(tbegin + elapsed);
        }
    }

    gTrace->prepareShutdown();

    delete server;
    delete sq;
    delete prox;
    delete server_id_map;
    delete weight_calc;
    delete cseg;
    delete loc_service;
    delete obj_factory;

    String sync_file = GetPerServerFile(STATS_SYNC_FILE, server_id);
    if (!sync_file.empty()) {
        std::ofstream sos(sync_file.c_str(), std::ios::out);
        if (sos)
            sos << Timer::getSystemClockOffset().milliseconds() << std::endl;
    }


    delete gNetwork;
    gNetwork=NULL;

    String trace_file = GetPerServerFile(STATS_TRACE_FILE, server_id);
    if (!trace_file.empty()) gTrace->save(trace_file);

    delete gTrace;
    gTrace = NULL;

    return 0;
}
