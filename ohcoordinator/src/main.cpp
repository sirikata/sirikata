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

#include <sirikata/core/network/IOStrandImpl.hpp>

#include <sirikata/ohcoordinator/ObjectHostSession.hpp>
#include <sirikata/ohcoordinator/ObjectSessionManager.hpp>
#include <sirikata/ohcoordinator/Authenticator.hpp>

#include "Server.hpp"

#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/network/ServerIDMap.hpp>

#include <sirikata/ohcoordinator/SpaceContext.hpp>
#include <sirikata/mesh/Filter.hpp>
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace {
using namespace Sirikata;

// Some platforms can't bind as many variables as we want to use, so we need to
// manually package them up.
struct ServerData {
    SpaceContext* space_context;
    Authenticator* auth;
    ObjectHostSessionManager* oh_sess_mgr;
    ObjectSessionManager* obj_sess_mgr;
};
void createServer(Server** server_out, ServerData sd, Address4 addr) {
    if (addr == Address4::Null) {
        SILOG(coordinator, fatal, "The requested server ID isn't in ServerIDMap");
        sd.space_context->shutdown();
    }

    Server* server = new Server(sd.space_context, sd.auth, addr, sd.oh_sess_mgr, sd.obj_sess_mgr);
    sd.space_context->add(server);

    *server_out = server;
}
}

int main(int argc, char** argv) {

    using namespace Sirikata;

    DynamicLibrary::Initialize();

    InitOptions();
    Trace::Trace::InitOptions();
    SpaceTrace::InitOptions();
    InitSpaceOptions();
    ParseOptions(argc, argv, OPT_CONFIG_FILE);

    PluginManager plugins;
    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_SPACE_PLUGINS) );

    // Fill defaults after plugin loading to ensure plugin-added
    // options get their defaults.
    FillMissingOptionDefaults();
    // Rerun original parse to make sure any newly added options are
    // properly parsed.
    ParseOptions(argc, argv, OPT_CONFIG_FILE);

    ReportVersion(); // After options so log goes to the right place

    std::string time_server=GetOptionValue<String>("time-server");
    NTPTimeSync sync;
    if (time_server.size() > 0)
        sync.start(time_server);

    ServerID server_id = GetOptionValue<ServerID>("id");
    String trace_file = GetPerServerFile(STATS_TRACE_FILE, server_id);
    Sirikata::Trace::Trace* gTrace = new Trace::Trace(trace_file);

    // Compute the starting date/time
    String start_time_str = GetOptionValue<String>("wait-until");
    Time start_time = start_time_str.empty() ? Timer::now() : Timer::getSpecifiedDate( start_time_str );
    start_time += GetOptionValue<Duration>("wait-additional");

    Duration duration = GetOptionValue<Duration>("duration");

    Network::IOService* ios = new Network::IOService("Space");
    Network::IOStrand* mainStrand = ios->createStrand("Space Main");

    OHDPSST::ConnectionManager* ohSstConnMgr = new OHDPSST::ConnectionManager();

    SpaceContext* space_context = new SpaceContext("space", server_id, ohSstConnMgr, ios, mainStrand, start_time, gTrace, duration);

    String servermap_type = GetOptionValue<String>("servermap");
    String servermap_options = GetOptionValue<String>("servermap-options");
    ServerIDMap * server_id_map =
        ServerIDMapFactory::getSingleton().getConstructor(servermap_type)(space_context, servermap_options);

    space_context->add(space_context);

    String timeseries_type = GetOptionValue<String>(OPT_TRACE_TIMESERIES);
    String timeseries_options = GetOptionValue<String>(OPT_TRACE_TIMESERIES_OPTIONS);
    Trace::TimeSeries* time_series = Trace::TimeSeriesFactory::getSingleton().getConstructor(timeseries_type)(space_context, timeseries_options);

    srand( GetOptionValue<uint32>("rand-seed") );

    ObjectHostSessionManager* oh_sess_mgr = new ObjectHostSessionManager(space_context);
    ObjectSessionManager* obj_sess_mgr = new ObjectSessionManager(space_context);

    String auth_type = GetOptionValue<String>(SPACE_OPT_AUTH);
    String auth_opts = GetOptionValue<String>(SPACE_OPT_AUTH_OPTIONS);
    Authenticator* auth =
        AuthenticatorFactory::getSingleton().getConstructor(auth_type)(space_context, auth_opts);

    // We need to do an async lookup, and to finish it the server needs to be
    // running. But we can't create the server until we have the address from
    // this lookup. We isolate as little as possible into this callback --
    // creating the server, finishing prox initialization, and getting them both
    // registered. We pass storage for the Server to the callback so we can
    // handle cleaning it up ourselves.
    using std::tr1::placeholders::_1;
    Server* server = NULL;
    ServerData sd;
    sd.space_context = space_context;
    sd.auth = auth;
    sd.oh_sess_mgr = oh_sess_mgr;
    sd.obj_sess_mgr = obj_sess_mgr;
    server_id_map->lookupExternal(
        space_context->id(),
        space_context->mainStrand->wrap(
            std::tr1::bind( &createServer, &server, sd, _1)
        )
    );

    // If we're one of the initial nodes, we'll have to wait until we hit the start time
    {
        Time now_time = Timer::now();
        if (start_time > now_time) {
            Duration sleep_time = start_time - now_time;
            printf("Waiting %f seconds\n", sleep_time.toSeconds() ); fflush(stdout);
            Timer::sleep(sleep_time);
        }
    }

    ///////////Go go go!! start of simulation/////////////////////


    space_context->add(auth);
    space_context->add(ohSstConnMgr);

    space_context->run(3);

    space_context->cleanup();

    if (GetOptionValue<bool>(PROFILE)) {
        space_context->profiler->report();
    }

    gTrace->prepareShutdown();
    Mesh::FilterFactory::destroy();
    ModelsSystemFactory::destroy();
    delete server;
    delete server_id_map;

    delete obj_sess_mgr;
    delete oh_sess_mgr;

    gTrace->shutdown();
    delete gTrace;
    gTrace = NULL;


    delete space_context;
    space_context = NULL;

    delete time_series;

    delete mainStrand;

    delete ios;

    delete ohSstConnMgr;

    sync.stop();

    plugins.gc();

    Sirikata::Logging::finishLog();

    return 0;
}
