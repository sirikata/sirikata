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

#include "ObjectHost.hpp"
#include "Object.hpp"
#include "ObjectFactory.hpp"
#include "ScenarioFactory.hpp"

#include "Options.hpp"
#include "Statistics.hpp"
#include "TabularServerIDMap.hpp"
#include "ScenarioFactory.hpp"


void *main_loop(void *);
int main(int argc, char** argv) {
    using namespace CBR;

    InitOptions();
    ParseOptions(argc, argv);

    std::string time_server=GetOption("time-server")->as<String>();
    TimeSync sync;
    sync.start(time_server);


    ObjectHostID oh_id = GetOption("ohid")->as<ObjectHostID>();
    String trace_file = GetPerServerFile(STATS_OH_TRACE_FILE, oh_id);
    Trace* gTrace = new Trace(trace_file);

    String filehandle = GetOption("serverips")->as<String>();
    std::ifstream ipConfigFileHandle(filehandle.c_str());
    ServerIDMap * server_id_map = new TabularServerIDMap(ipConfigFileHandle);


    MaxDistUpdatePredicate::maxDist = GetOption(MAX_EXTRAPOLATOR_DIST)->as<float64>();

    BoundingBox3f region = GetOption("region")->as<BoundingBox3f>();
    Vector3ui32 layout = GetOption("layout")->as<Vector3ui32>();


    Duration duration = GetOption("duration")->as<Duration>();

    // Get the starting time
    String start_time_str = GetOption("wait-until")->as<String>();
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOption("wait-additional")->as<Duration>();


    srand( GetOption("rand-seed")->as<uint32>() );

    IOService* ios = IOServiceFactory::makeIOService();
    IOStrand* mainStrand = ios->createStrand();

    ObjectHostContext* ctx = new ObjectHostContext(oh_id, ios, mainStrand, gTrace, start_time, duration);

    ObjectFactory* obj_factory = new ObjectFactory(ctx, region, duration);

    ObjectHost* obj_host = new ObjectHost(ctx, gTrace, server_id_map);
    Scenario* scenario = ScenarioFactory::getSingleton().getConstructor(GetOption("scenario")->as<String>())(GetOption("scenario-options")->as<String>());
    scenario->initialize(ctx);

    SSTConnectionManager* sstConnMgr = new SSTConnectionManager(ctx);

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
    ctx->add(ctx);
    ctx->add(obj_host);
    ctx->add(obj_factory);
    ctx->add(scenario);
    ctx->add(sstConnMgr);
    ctx->run(2);

    ctx->cleanup();

    if (GetOption(PROFILE)->as<bool>()) {
        ctx->profiler->report();
    }

    gTrace->prepareShutdown();

    delete sstConnMgr;
    delete server_id_map;
    delete obj_factory;
    delete scenario;
    delete obj_host;
    delete ctx;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;

    delete mainStrand;
    IOServiceFactory::destroyIOService(ios);

    sync.stop();

    return 0;
}
