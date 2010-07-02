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

#include <sirikata/core/util/Timer.hpp>
#include <sirikata/core/network/NTPTimeSync.hpp>

#include "ObjectHost.hpp"
#include "Object.hpp"
#include "ObjectFactory.hpp"
#include "ScenarioFactory.hpp"

#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/network/ServerIDMap.hpp>

#include <sirikata/core/network/IOServiceFactory.hpp>

void *main_loop(void *);
int main(int argc, char** argv) {
    using namespace Sirikata;

    InitOptions();
    Trace::Trace::InitOptions();
    OHTrace::InitOptions();
    InitSimOHOptions();
    ParseOptions(argc, argv);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_OH_PLUGINS) );

    std::string time_server=GetOptionValue<String>("time-server");
    NTPTimeSync sync;
    if (time_server.size() > 0)
        sync.start(time_server);


    ObjectHostID oh_id = GetOptionValue<ObjectHostID>("ohid");
    String trace_file = GetPerServerFile(STATS_OH_TRACE_FILE, oh_id);
    Trace::Trace* gTrace = new Trace::Trace(trace_file);

    String servermap_type = GetOptionValue<String>("servermap");
    String servermap_options = GetOptionValue<String>("servermap-options");
    ServerIDMap * server_id_map =
        ServerIDMapFactory::getSingleton().getConstructor(servermap_type)(servermap_options);

    MaxDistUpdatePredicate::maxDist = GetOptionValue<float64>(MAX_EXTRAPOLATOR_DIST);

    BoundingBox3f region = GetOptionValue<BoundingBox3f>("region");
    Vector3ui32 layout = GetOptionValue<Vector3ui32>("layout");


    Duration duration = GetOptionValue<Duration>("duration");

    // Get the starting time
    String start_time_str = GetOptionValue<String>("wait-until");
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOptionValue<Duration>("wait-additional");


    srand( GetOptionValue<uint32>("rand-seed") );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* mainStrand = ios->createStrand();

    ObjectHostContext* ctx = new ObjectHostContext(oh_id, ios, mainStrand, gTrace, start_time, duration);

    ObjectFactory* obj_factory = new ObjectFactory(ctx, region, duration);

    ObjectHost* obj_host = new ObjectHost(ctx, gTrace, server_id_map);
    Scenario* scenario = ScenarioFactory::getSingleton().getConstructor(GetOptionValue<String>("scenario"))(GetOptionValue<String>("scenario-options"));
    scenario->initialize(ctx);

    SSTConnectionManager* sstConnMgr = new SSTConnectionManager(ctx);

    // If we're one of the initial nodes, we'll have to wait until we hit the start time
    {
        Time now_time = Timer::now();
        if (start_time > now_time) {
            Duration sleep_time = start_time - now_time;
            printf("Waiting %f seconds\n", sleep_time.toSeconds() ); fflush(stdout);
#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
            Sleep( sleep_time.toMilliseconds() );
#else
            usleep( sleep_time.toMicroseconds() );
#endif
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

    if (GetOptionValue<bool>(PROFILE)) {
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
    Network::IOServiceFactory::destroyIOService(ios);

    sync.stop();

    return 0;
}
