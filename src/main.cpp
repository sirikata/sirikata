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
#include "ObjectFactory.hpp"
#include "LocationService.hpp"
#include "ServerMap.hpp"
#include "Proximity.hpp"
#include "Server.hpp"

#include "OracleLocationService.hpp"
#include "UniformServerMap.hpp"
#include "Test.hpp"
#include "RaknetNetwork.hpp"
#include "FairSendQueue.hpp"
#include "FIFOSendQueue.hpp"

#include "TabularServerIDMap.hpp"

int main(int argc, char** argv) {
    using namespace CBR;

    InitializeOptions::module("cbr")
        .addOption(new OptionValue("test", "none", Sirikata::OptionValueType<String>(), "Type of test to run"))
        .addOption(new OptionValue("server-port", "8080", Sirikata::OptionValueType<String>(), "Port for server side of test"))
        .addOption(new OptionValue("client-port", "8081", Sirikata::OptionValueType<String>(), "Port for client side of test"))
        .addOption(new OptionValue("host", "127.0.0.1", Sirikata::OptionValueType<String>(), "Host to connect to for test"))

        .addOption(new OptionValue("sim", "false", Sirikata::OptionValueType<bool>(), "Turns simulated clock time on or off"))
        .addOption(new OptionValue("sim-step", "10ms", Sirikata::OptionValueType<Duration>(), "Size of simulation time steps"))

        .addOption(new OptionValue("time-dilation", "1.0", Sirikata::OptionValueType<float>(), "Factor by which times will be scaled (to allow faster processing when CPU and bandwidth is readily available, slower when they are overloaded)"))

        .addOption(new OptionValue("id", "1", Sirikata::OptionValueType<ServerID>(), "Server ID for this server"))
        .addOption(new OptionValue("objects", "100", Sirikata::OptionValueType<uint32>(), "Number of objects to simulate"))
        .addOption(new OptionValue("region", "<<-100,-100,-100>,<100,100,100>>", Sirikata::OptionValueType<BoundingBox3f>(), "Simulation region"))
        .addOption(new OptionValue("layout", "<2,1,1>", Sirikata::OptionValueType<Vector3ui32>(), "Layout of servers in uniform grid - ixjxk servers"))
        .addOption(new OptionValue("duration", "1s", Sirikata::OptionValueType<Duration>(), "Duration of the simulation"))
        .addOption(new OptionValue("serverips", "serverip.txt", Sirikata::OptionValueType<String>(), "The file containing the server ip list"))

        .addOption(new OptionValue("bandwidth", "2000000000", Sirikata::OptionValueType<uint32>(), "Total bandwidth for this server in bytes per second"))

        .addOption(new OptionValue("rand-seed", "0", Sirikata::OptionValueType<uint32>(), "The random seed to synchronize all servers"))

        .addOption(new OptionValue("stats.bandwidth-filename", "", Sirikata::OptionValueType<String>(), "The filename to save bandwidth stats to"))
     ;

    OptionSet* options = OptionSet::getOptions("cbr");
    options->parse(argc, argv);

    String test_mode = options->referenceOption("test")->as<String>();
    if (test_mode != "none") {
        String server_port = options->referenceOption("server-port")->as<String>();
        String client_port = options->referenceOption("client-port")->as<String>();
        String host = options->referenceOption("host")->as<String>();
        if (test_mode == "server")
            CBR::testServer(server_port.c_str(), host.c_str(), client_port.c_str());
        else if (test_mode == "client")
            CBR::testClient(client_port.c_str(), host.c_str(), server_port.c_str());
        return 0;
    }

    uint32 nobjects = options->referenceOption("objects")->as<uint32>();
    BoundingBox3f region = options->referenceOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = options->referenceOption("layout")->as<Vector3ui32>();
    Duration duration = options->referenceOption("duration")->as<Duration>();

    srand( options->referenceOption("rand-seed")->as<uint32>() );

    ObjectFactory* obj_factory = new ObjectFactory(nobjects, region, duration);
    
    LocationService* loc_service = new OracleLocationService(obj_factory);
    ServerMap* server_map = new UniformServerMap(
        loc_service,
        region,
        layout
    );

    String filehandle = options->referenceOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    Network* network=new RaknetNetwork(server_id_map);
    Proximity* prox = new Proximity();

    SendQueue* sq=new FairSendQueue(network, options->referenceOption("bandwidth")->as<uint32>());
    obj_factory->createObjectQueues(sq);

    ServerID server_id = options->referenceOption("id")->as<ServerID>();
    Server* server = new Server(server_id, obj_factory, loc_service, server_map, prox, network, sq);
    
    bool sim = options->referenceOption("sim")->as<bool>();
    Duration sim_step = options->referenceOption("sim-step")->as<Duration>();

    float time_dilation = options->referenceOption("time-dilation")->as<float>();
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

    return 0;
}
