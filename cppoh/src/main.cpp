/*  Sirikata
 *  main.cpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/proxyobject/LightInfo.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/SpaceIDMap.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <Protocol_Persistence.pbj.hpp>
#include <Protocol_Sirikata.pbj.hpp>
#include <time.h>
#include <boost/thread.hpp>

#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include "Options.hpp"
#include <sirikata/oh/Trace.hpp>

#include <sirikata/core/network/ServerIDMap.hpp>
#include <sirikata/core/network/NTPTimeSync.hpp>

#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/oh/ObjectHostContext.hpp>

#include <sirikata/oh/ObjectFactory.hpp>

#ifdef __GNUC__
#include <fenv.h>
#endif

int main (int argc, char** argv) {
    using namespace Sirikata;

    InitOptions();
    Trace::Trace::InitOptions();
    OHTrace::InitOptions();
    InitCPPOHOptions();

    ParseOptions(argc, argv);

    PluginManager plugins;

    plugins.loadList( GetOptionValue<String>(OPT_PLUGINS) );
    plugins.loadList( GetOptionValue<String>(OPT_OH_PLUGINS) );


    String time_server = GetOptionValue<String>("time-server");
    NTPTimeSync sync;
    if (time_server.size() > 0)
        sync.start(time_server);

#ifdef __GNUC__
#ifndef __APPLE__
    if (GetOptionValue<bool>(OPT_SIGFPE)) {
        feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
    }
#endif
#endif


    ObjectHostID oh_id = GetOptionValue<ObjectHostID>("ohid");
    String trace_file = GetPerServerFile(STATS_OH_TRACE_FILE, oh_id);
    Trace::Trace* trace = new Trace::Trace(trace_file);

    String servermap_type = GetOptionValue<String>("servermap");
    String servermap_options = GetOptionValue<String>("servermap-options");
    ServerIDMap * server_id_map =
        ServerIDMapFactory::getSingleton().getConstructor(servermap_type)(servermap_options);

    srand( GetOptionValue<uint32>("rand-seed") );

    Network::IOService* ios = Network::IOServiceFactory::makeIOService();
    Network::IOStrand* mainStrand = ios->createStrand();

    Time start_time = Timer::now(); // Just for stats in ObjectHostContext.
    Duration duration = Duration::zero(); // Indicates to run forever.
    ObjectHostContext* ctx = new ObjectHostContext(oh_id, ios, mainStrand, trace, start_time, duration);


    SSTConnectionManager* sstConnMgr = new SSTConnectionManager(ctx);

    SpaceIDMap *spaceMap = new SpaceIDMap;
    SpaceID mainSpace(GetOptionValue<UUID>(OPT_MAIN_SPACE));
    typedef std::map<std::string,std::string> SimpleSpaceIDMap;
    SimpleSpaceIDMap spaceIdMap(GetOptionValue<SimpleSpaceIDMap>(OPT_SPACEID_MAP));
    for (SimpleSpaceIDMap::iterator i = spaceIdMap.begin(),
             ie=spaceIdMap.end();
         i!=ie;
         ++i) {
        SpaceID newSpace(UUID(i->first,UUID::HumanReadable()));
        spaceMap->insert(newSpace, Network::Address::lexical_cast(i->second).as<Network::Address>());
    }

    ObjectHost *oh = new ObjectHost(ctx, spaceMap, ios, "");

    // Add all the spaces to the ObjectHost.
    // FIXME we're adding all spaces and having them use the same ServerIDMap
    // because its difficult to encode this in the options.
    // FIXME once this is working, the above SpaceIDMap (spaceMap) shouldn't be
    // used the same way -- it was only mapping to a single address instead of
    // an entire ServerIDMap.
    for (SimpleSpaceIDMap::iterator i = spaceIdMap.begin(),
             ie=spaceIdMap.end();
         i!=ie;
         ++i) {
        SpaceID newSpace(UUID(i->first,UUID::HumanReadable()));
        oh->addServerIDMap(newSpace, server_id_map);
    }


    // FIXME simple test example
    // This is the camera.  We need it early on because other things depend on
    // having its ObjectHostProxyManager.
    HostedObjectPtr obj = HostedObject::construct<HostedObject>(ctx, oh, UUID::random(), true);
    obj->init();
    // Note: We currently just use the proxy manager for the default space. Not
    // sure if we should do something about handling multiple spaces.
    ProxyManagerPtr proxy_manager = obj->getDefaultProxyManager( mainSpace );

    typedef std::vector<TimeSteppedSimulation*> SimList;
    SimList sims;

    typedef std::list<String> StringList;
    StringList oh_sims(GetOptionValue<StringList>(OPT_OH_SIMS));
    for(StringList::iterator it = oh_sims.begin(); it != oh_sims.end(); it++)
        SILOG(cppoh,error,*it);
    for(StringList::iterator it = oh_sims.begin(); it != oh_sims.end(); it++) {
        String simName = *it;
        SILOG(cppoh,info,String("Initializing ") + simName);
        TimeSteppedSimulation *sim =
            SimulationFactory::getSingleton()
            .getConstructor ( simName ) ( ctx, proxy_manager.get(), "" );
        if (!sim) {
            SILOG(cppoh,error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
            std::cerr << "Press enter to continue" << std::endl;
            fgetc(stdin);
            exit(0);
        }
        else {
            oh->addListener(sim);
            SILOG(cppoh,info,String("Successfully initialized ") + simName);
            sims.push_back(sim);
        }
    }
    String scriptFile=GetOptionValue<String>(OPT_CAMERASCRIPT);

    // FIXME
    // TEST
    obj->connect(
        mainSpace,
        Location( Vector3d::nil(), Quaternion::identity(), Vector3f::nil(), Vector3f::nil(), 0),
        BoundingSphere3f(Vector3f::nil(), 1.f),
        "meerakat:///ewencp/male_avatar.dae",
        SolidAngle(0.00000001f),
        UUID::null(),
        scriptFile,
        scriptFile.empty()?String():GetOptionValue<String>(OPT_CAMERASCRIPTTYPE));


    String objfactory_type = GetOptionValue<String>(OPT_OBJECT_FACTORY);
    String objfactory_options = GetOptionValue<String>(OPT_OBJECT_FACTORY_OPTS);
    ObjectFactory* obj_factory = NULL;
    if (!objfactory_type.empty()) {
        obj_factory = ObjectFactoryFactory::getSingleton().getConstructor(objfactory_type)(ctx, oh, mainSpace, objfactory_options);
        obj_factory->generate();
    }

    ///////////Go go go!! start of simulation/////////////////////
    ctx->add(ctx);
    ctx->add(oh);
    ctx->add(sstConnMgr);
    for(SimList::iterator it = sims.begin(); it != sims.end(); it++)
        ctx->add(*it);
    ctx->run(1);


    obj.reset();

    ctx->cleanup();
    trace->prepareShutdown();

    proxy_manager.reset();
    delete oh;

    for(SimList::reverse_iterator it = sims.rbegin(); it != sims.rend(); it++) {
        delete *it;
    }
    sims.clear();
    plugins.gc();
    SimulationFactory::destroy();

    delete spaceMap;

    delete ctx;

    trace->shutdown();
    delete trace;
    trace = NULL;

    delete mainStrand;
    Network::IOServiceFactory::destroyIOService(ios);

    sync.stop();

    return 0;
}
