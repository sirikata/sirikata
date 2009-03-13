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
#include "Analysis.hpp"

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
            printf ("Match offset %f\n",offset);
            Timer::setSystemClockOffset(Duration::seconds((float32)offset));
        }
        fclose(ntp);
        close(ntppipes[0]);
        close(ntppipes[1]);
    }
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
    ObjectTrace* object_trace = new ObjectTrace();

    uint32 nobjects = GetOption("objects")->as<uint32>();
    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();
    Duration duration = GetOption("duration")->as<Duration>();

    srand( GetOption("rand-seed")->as<uint32>() );

    ObjectFactory* obj_factory = new ObjectFactory(nobjects, region, duration);

    {
        LocationErrorAnalysis(STATS_OBJECT_TRACE_FILE, 2);
        exit(0);
    }

    LocationService* loc_service = new OracleLocationService(obj_factory);
    ServerMap* server_map = new UniformServerMap(
        loc_service,
        std::tr1::bind(&integralExpFunction,GetOption("flatness")->as<double>(),_1,_2,_3,_4),
        region,
        layout
    );

    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);
    Network* network=new RaknetNetwork(server_id_map);
    Proximity* prox = new Proximity(obj_factory, loc_service);

    SendQueue* sq=new FairSendQueue(network, GetOption("bandwidth")->as<uint32>(),GetOption("capexcessbandwidth")->as<bool>(), bandwidth_stats);
    obj_factory->createObjectQueues(sq);

    ServerID server_id = GetOption("id")->as<ServerID>();
    Server* server = new Server(server_id, obj_factory, loc_service, server_map, prox, network, sq, bandwidth_stats, object_trace);

    bool sim = GetOption("sim")->as<bool>();
    Duration sim_step = GetOption("sim-step")->as<Duration>();

    float time_dilation = GetOption("time-dilation")->as<float>();
    float inv_time_dilation = 1.f / time_dilation;

    ///////////Wait until start of simulation/////////////////////
    if (GetOption("wait-until")->as<String>().length()) {
        Duration waiting_time=Timer::getSpecifiedDate(GetOption("wait-until")->as<String>())-Timer::now();
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

    String object_trace_file = GetPerServerFile(STATS_OBJECT_TRACE_FILE, server_id);
    if (!object_trace_file.empty()) object_trace->save(object_trace_file);
    delete object_trace;

    String sync_file = GetPerServerFile(STATS_SYNC_FILE, server_id);
    if (!sync_file.empty()) {
        std::ofstream sos(sync_file.c_str(), std::ios::out);
        if (sos)
            sos << Timer::getSystemClockOffset().milliseconds() << std::endl;
    }

    return 0;
}
