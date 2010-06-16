/*  Sirikata
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



#include <sirikata/cbrcore/Timer.hpp>
#include <sirikata/cbrcore/TimeSync.hpp>
#include <sirikata/cbrcore/TimeProfiler.hpp>

#include "SpaceNetwork.hpp"

#include "Forwarder.hpp"

#include "LocationService.hpp"

#include "Proximity.hpp"
#include "Server.hpp"

#include <sirikata/cbrcore/Options.hpp>
#include <sirikata/cbrcore/Statistics.hpp>
#include "StandardLocationService.hpp"
#include "TCPSpaceNetwork.hpp"
#include "FairServerMessageReceiver.hpp"
#include "FairServerMessageQueue.hpp"
#include <sirikata/cbrcore/TabularServerIDMap.hpp>
#include "UniformCoordinateSegmentation.hpp"
#include "CoordinateSegmentationClient.hpp"
#include <sirikata/cbrcore/LoadMonitor.hpp>
#include "CraqObjectSegmentation.hpp"

#include <sirikata/cbrcore/SpaceContext.hpp>


int main(int argc, char** argv) {

    using namespace Sirikata;

    InitOptions();
    Trace::InitOptions();
    ParseOptions(argc, argv);

    std::string time_server=GetOption("time-server")->as<String>();
    TimeSync sync;
    if (time_server.size() > 0)
        sync.start(time_server);


    ServerID server_id = GetOption("id")->as<ServerID>();
    String trace_file = GetPerServerFile(STATS_TRACE_FILE, server_id);
    Sirikata::Trace* gTrace = new Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOption("wait-until")->as<String>();
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOption("wait-additional")->as<Duration>();

    Duration duration = GetOption("duration")->as<Duration>();

    IOService* ios = IOServiceFactory::makeIOService();
    IOStrand* mainStrand = ios->createStrand();


    Time init_space_ctx_time = Time::null() + (Timer::now() - start_time);

    SpaceContext* space_context = new SpaceContext(server_id, ios, mainStrand, start_time, init_space_ctx_time, gTrace, duration);

    Sirikata::SpaceNetwork* gNetwork = NULL;
    String network_type = GetOption(NETWORK_TYPE)->as<String>();
    if (network_type == "tcp")
      gNetwork = new TCPSpaceNetwork(space_context);

    /*
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
    */

    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();


    uint32 max_space_servers = GetOption("max-servers")->as<uint32>();
    if (max_space_servers == 0)
      max_space_servers = layout.x * layout.y * layout.z;
    uint32 num_oh_servers = GetOption("num-oh")->as<uint32>();
    uint32 nservers = max_space_servers + num_oh_servers;



    srand( GetOption("rand-seed")->as<uint32>() );


    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    gNetwork->setServerIDMap(server_id_map);


    Forwarder* forwarder = new Forwarder(space_context);


    String cseg_type = GetOption(CSEG)->as<String>();
    CoordinateSegmentation* cseg = NULL;
    if (cseg_type == "uniform")
        cseg = new UniformCoordinateSegmentation(space_context, region, layout);
    else if (cseg_type == "client") {
      cseg = new CoordinateSegmentationClient(space_context, region, layout, server_id_map);
    }
    else {
        assert(false);
        exit(-1);
    }



    LocationService* loc_service = NULL;
    String loc_service_type = GetOption(LOC)->as<String>();
    if (loc_service_type == "standard")
        loc_service = new StandardLocationService(space_context);
    else
        assert(false);


    ServerMessageQueue* sq = NULL;
    String server_queue_type = GetOption(SERVER_QUEUE)->as<String>();
    if (server_queue_type == "fair") {
        sq = new FairServerMessageQueue(
            space_context, gNetwork,
            (ServerMessageQueue::Sender*)forwarder);
    }
    /*
    else if (server_queue_type == "fifo")
        sq = new FIFOServerMessageQueue(space_context, gNetwork, server_id_map,
        (ServerMessageQueue::Listener*)forwarder,
        GetOption(SEND_BANDWIDTH)->as<uint32>());
    */
    else {
        assert(false);
        exit(-1);
    }

    ServerMessageReceiver* server_message_receiver = NULL;
    String server_receiver_type = GetOption(SERVER_RECEIVER)->as<String>();
    if (server_queue_type == "fair")
        server_message_receiver =
                new FairServerMessageReceiver(space_context, gNetwork, (ServerMessageReceiver::Listener*)forwarder);
    else {
        assert(false);
        exit(-1);
    }



    LoadMonitor* loadMonitor = new LoadMonitor(space_context, sq, cseg);


    //Create OSeg
    std::string oseg_type=GetOption(OSEG)->as<String>();
    IOStrand* osegStrand = space_context->ioService->createStrand();
    ObjectSegmentation* oseg = NULL;
    if (oseg_type == OSEG_OPTION_CRAQ)
    {
      //using craq approach
      std::vector<UUID> initServObjVec;

      std::vector<CraqInitializeArgs> craqArgsGet;
      CraqInitializeArgs cInitArgs1;

      cInitArgs1.ipAdd = "localhost";
      cInitArgs1.port  =     "10498"; //craq version 2
      craqArgsGet.push_back(cInitArgs1);

      std::vector<CraqInitializeArgs> craqArgsSet;
      CraqInitializeArgs cInitArgs2;
      cInitArgs2.ipAdd = "localhost";
      cInitArgs2.port  =     "10499";
      craqArgsSet.push_back(cInitArgs2);


      std::string oseg_craq_prefix=GetOption(OSEG_UNIQUE_CRAQ_PREFIX)->as<String>();

      if (oseg_type.size() ==0)
      {
        std::cout<<"\n\nERROR: Incorrect craq prefix for oseg.  String must be at least one letter long.  (And be between G and Z.)  Please try again.\n\n";
        assert(false);
      }

      oseg = new CraqObjectSegmentation (space_context, cseg, initServObjVec, craqArgsGet, craqArgsSet, oseg_craq_prefix[0],osegStrand,space_context->mainStrand);

    }      //end craq approach

    //end create oseg


    // We have all the info to initialize the forwarder now
    forwarder->initialize(oseg, sq, server_message_receiver, loc_service);


    Proximity* prox = new Proximity(space_context, loc_service);


    Server* server = new Server(space_context, forwarder, loc_service, cseg, prox, oseg, server_id_map->lookupExternal(space_context->id()));

      prox->initialize(cseg);

    // If we're one of the initial nodes, we'll have to wait until we hit the start time
    {
        Time now_time = Timer::now();
        if (start_time > now_time) {
            Duration sleep_time = start_time - now_time;
            printf("Waiting %f seconds\n", sleep_time.toSeconds() ); fflush(stdout);
#if SIRIKATA_PLATFORM == PLATFORM_WINDOWS
            Sleep( sleep_time.toMilliseconds() );
#else
            usleep( sleep_time.toMicroseconds() );
#endif
        }
    }

    ///////////Go go go!! start of simulation/////////////////////
    SSTConnectionManager* sstConnMgr = new SSTConnectionManager(space_context);

    space_context->add(space_context);
    space_context->add(gNetwork);
    space_context->add(cseg);
    space_context->add(loc_service);
    space_context->add(prox);
    space_context->add(server);
    space_context->add(oseg);
    space_context->add(loadMonitor);
    space_context->add(sstConnMgr);


    space_context->run(2);

    space_context->cleanup();

    if (GetOption(PROFILE)->as<bool>()) {
        space_context->profiler->report();
    }

    gTrace->prepareShutdown();
    prox->shutdown();

    delete server;
    delete sq;
    delete server_message_receiver;
    delete prox;
    delete server_id_map;


    delete cseg;
    delete oseg;
    delete loc_service;
    delete sstConnMgr;
    delete forwarder;

    delete gNetwork;
    gNetwork=NULL;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;


    delete space_context;
    space_context = NULL;

    delete mainStrand;
    delete osegStrand;

    IOServiceFactory::destroyIOService(ios);


    sync.stop();

    return 0;
}
