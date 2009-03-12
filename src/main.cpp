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
#include "ServerMap.hpp"
#include "Proximity.hpp"
#include "Server.hpp"

#include "Options.hpp"
#include "Statistics.hpp"

#include "OracleLocationService.hpp"
#include "UniformServerMap.hpp"
#include "Test.hpp"
#include "RaknetNetwork.hpp"
#include "FairSendQueue.hpp"
#include "FIFOSendQueue.hpp"

#include "TabularServerIDMap.hpp"
#include "ExpIntegral.hpp"

int main(int argc, char** argv) {
    using namespace CBR;

    InitOptions();
    ParseOptions(argc, argv);

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

    BandwidthStatistics* bandwidth_stats = new BandwidthStatistics();
    LocationStatistics* location_stats = new LocationStatistics();

    uint32 nobjects = GetOption("objects")->as<uint32>();
    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();
    Duration duration = GetOption("duration")->as<Duration>();

    srand( GetOption("rand-seed")->as<uint32>() );

    ObjectFactory* obj_factory = new ObjectFactory(nobjects, region, duration);

    LocationService* loc_service = new OracleLocationService(obj_factory);
    ServerMap* server_map = new UniformServerMap(
        loc_service,
        std::tr1::bind(&integralExpFunction,1.,_1,_2,_3,_4),
        region,
        layout
    );

    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    Network* network=new RaknetNetwork(server_id_map);
    Proximity* prox = new Proximity(obj_factory, loc_service);

    SendQueue* sq=new FairSendQueue(network, GetOption("bandwidth")->as<uint32>(),!GetOption("capexcessbandwidth")->as<bool>(), bandwidth_stats);
    obj_factory->createObjectQueues(sq);

    ServerID server_id = GetOption("id")->as<ServerID>();
    Server* server = new Server(server_id, obj_factory, loc_service, server_map, prox, network, sq, bandwidth_stats, location_stats);

    bool sim = GetOption("sim")->as<bool>();
    Duration sim_step = GetOption("sim-step")->as<Duration>();

    float time_dilation = GetOption("time-dilation")->as<float>();
    float inv_time_dilation = 1.f / time_dilation;

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

        while( true ) {
            Duration elapsed = timer.elapsed() * inv_time_dilation;
            if (elapsed > duration)
                break;
            server->tick(tbegin + elapsed);
        }
    }

    delete server;
    delete sq;
    delete prox;
    delete network;
    delete server_id_map;
    delete server_map;
    delete loc_service;
    delete obj_factory;


    String bandwidth_file = GetPerServerFile(STATS_BANDWIDTH_FILE, server_id);
    if (!bandwidth_file.empty()) bandwidth_stats->save(bandwidth_file);
    delete bandwidth_stats;

    String location_file = GetPerServerFile(STATS_LOCATION_FILE, server_id);
    if (!location_file.empty()) location_stats->save(location_file);
    delete location_stats;


    return 0;
}
