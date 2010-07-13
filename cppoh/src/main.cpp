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
#include <sirikata/proxyobject/ModelsSystemFactory.hpp> // MCB:
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>

#include <sirikata/core/task/EventManager.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/options/CDNConfig.hpp>

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/proxyobject/LightInfo.hpp>
#include <sirikata/oh/TopLevelSpaceConnection.hpp>
#include <sirikata/oh/SpaceConnection.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/SpaceIDMap.hpp>
#include <sirikata/oh/ObjectHostTimeOffsetManager.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/persistence/ObjectStorage.hpp>
#include <sirikata/core/persistence/ReadWriteHandlerFactory.hpp>
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


namespace Sirikata {

using Task::GenEventManager;
using Transfer::TransferManager;

class UUIDLister : public MessageService {
    ObjectHost *mObjectHost;
    SpaceID mSpace;
    MessagePort mPort;
    volatile bool mSuccess;

public:
    UUIDLister(ObjectHost*oh, const SpaceID &space)
        : mObjectHost(oh), mSpace(space), mPort(Services::REGISTRATION) {
        mObjectHost->registerService(mPort, this);
    }
    ~UUIDLister() {
        mObjectHost->unregisterService(mPort);
    }
    bool forwardMessagesTo(MessageService *other) {
        return false;
    }
    bool endForwardingMessagesTo(MessageService *other) {
        return false;
    }
    void processMessage(const RoutableMessageHeader &hdr, MemoryReference body) {
        Persistence::Protocol::Response resp;
        resp.ParseFromArray(body.data(), body.length());
        if (hdr.has_return_status() || resp.has_return_status()) {
            SILOG(cppoh,info,"Failed to connect to database: "<<hdr.has_return_status()<<", "<<resp.has_return_status());
            mSuccess = true;
            return;
        }
        Protocol::UUIDListProperty uuidList;
        if (resp.reads(0).has_return_status()) {
            SILOG(cppoh,info,"Failed to find ObjectList in database.");
            mSuccess = true;
            return;
        }
        uuidList.ParseFromString(resp.reads(0).data());
        for (int i = 0; i < uuidList.value_size(); i++) {
            SILOG(cppoh,info,"Loading object "<<ObjectReference(uuidList.value(i)));
            HostedObjectPtr obj = HostedObject::construct<HostedObject>(mObjectHost, uuidList.value(i));
            obj->initializeRestoreFromDatabase(mSpace, HostedObjectPtr());
        }
        mSuccess = true;
    }
    void go() {
        mSuccess = false;
        Persistence::Protocol::ReadWriteSet rws;
        Persistence::Protocol::IStorageElement el = rws.add_reads();
        el.set_field_name("ObjectList");
//        el.set_object_uuid(UUID::null());
//        el.set_field_id(0);
        RoutableMessageHeader hdr;
        hdr.set_source_object(ObjectReference::spaceServiceID());
        hdr.set_source_port(mPort);
        hdr.set_destination_port(Services::PERSISTENCE);
        hdr.set_destination_object(ObjectReference::spaceServiceID());
        std::string body;
        rws.SerializeToString(&body);
        mObjectHost->processMessage(hdr, MemoryReference(body));
    }
    void goWait(Task::WorkQueue *queue) {
        mSuccess = false;
        go();
        while (!mSuccess) {
            // needs to happen in one thread for now...
            queue->dequeuePoll();
            //Network::IOServiceFactory::pollService(ioServ); // kills ioservice if there's nothing to be processed...
        }
    }
};

}

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

    OptionMapPtr transferOptions (new OptionMap);
    {
        std::string contents( GetOptionValue<String>(OPT_CDN_CONFIG) );
        std::string::size_type pos(0);
        parseConfig(contents, transferOptions, transferOptions, pos);
        std::cout << *transferOptions;
    }

    // FIXME: Initializes protocol handlers (http, rest, file, x-shasumofuri,
    // x-hashed) in the Transfer system.  This seems like it should be handled
    // automatically with plugins.
    initializeProtocols();


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

    Task::WorkQueue *workQueue = new Task::LockFreeWorkQueue;
    Task::GenEventManager *eventManager = new Task::GenEventManager(workQueue);
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
    String localDbFile = GetOptionValue<String>(OPT_DB);
    if (localDbFile.length()&&localDbFile[0]!='/'&&localDbFile[0]!='\\') {
        FILE * fp=fopen(localDbFile.c_str(),"rb");
        for (int i=0;i<4&&fp==NULL;++i) {
            localDbFile="../"+localDbFile;
            fp=fopen(localDbFile.c_str(),"rb");
        }
        if (fp) fclose(fp);
        else localDbFile = GetOptionValue<String>(OPT_DB);
    }
    Persistence::ReadWriteHandler *database=Persistence::ReadWriteHandlerFactory::getSingleton()
        .getConstructor("sqlite")(String("--databasefile ")+localDbFile);

    ObjectHost *oh = new ObjectHost(ctx, spaceMap, workQueue, ios, "");
    oh->registerService(Services::PERSISTENCE, database);

    {
        UUIDLister lister(oh, mainSpace);
        lister.goWait(workQueue);
    }

    std::tr1::shared_ptr<ProxyManager> provider = oh->connectToSpace(mainSpace);
    if (!provider) {
        SILOG(cppoh,error,String("Unable to load database in ") + GetOptionValue<String>(OPT_DB));
        std::cout << "Press enter to continue" << std::endl;
        std::cerr << "Press enter to continue" << std::endl;
        fgetc(stdin);
        return 1;
    }

    TransferManager *tm;
    try {
        tm = initializeTransferManager((*transferOptions)["cdn"], eventManager);
    } catch (OptionDoesNotExist &err) {
        SILOG(input,fatal,"Fatal Error: Failed to load CDN config: " << err.what());
        std::cout << "Press enter to continue" << std::endl;
        std::cerr << "Press enter to continue" << std::endl;
        fgetc(stdin);
        return 1;
    }

    String graphicsCommandArguments;
    {
        std::ostringstream os;
        os << "--transfermanager=" << tm << " ";
        os << "--eventmanager=" << eventManager << " ";
        os << "--workqueue=" << workQueue << " ";
        graphicsCommandArguments = os.str();
    }

    // MCB: seems like a good place to initialize models system
    ModelsSystem* mm ( ModelsSystemFactory::getSingleton ().getConstructor ( "colladamodels" ) ( provider.get(), graphicsCommandArguments ) );

    if ( mm )
    {
        SILOG(cppoh,info,"Created ModelsSystemFactory ");
    }
    else
    {
        SILOG(cppoh,error,"Failed to create ModelsSystemFactory ");
    }

    if (!provider) {
        SILOG(cppoh,error,"Failed to get TopLevelSpaceConnection for main space "<<mainSpace);
    }

    bool continue_simulation = true;

    typedef std::vector<TimeSteppedSimulation*> SimList;
    SimList sims;

    struct SimulationRequest {
        const char* name;
        bool required;
    };
    const uint32 nSimRequests = 1;
    SimulationRequest simRequests[nSimRequests] = {
        {"ogregraphics", true}
    };
    for(uint32 ir = 0; ir < nSimRequests && continue_simulation; ir++) {
        String simName = simRequests[ir].name;
        SILOG(cppoh,info,String("Initializing ") + simName);
        TimeSteppedSimulation *sim =
            SimulationFactory::getSingleton()
            .getConstructor ( simName ) ( ctx, provider.get(), provider->getTimeOffsetManager(), graphicsCommandArguments );
        if (!sim) {
            if (simRequests[ir].required) {
                SILOG(cppoh,error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
                std::cout << "Press enter to continue" << std::endl;
                std::cerr << "Press enter to continue" << std::endl;
                fgetc(stdin);
                continue_simulation = false;
            }
            else {
                SILOG(cppoh,info,String("Couldn't load ") + simName + String(" plugin."));
            }
        }
        else {
            oh->addListener(sim);
            SILOG(cppoh,info,String("Successfully initialized ") + simName);
            sims.push_back(sim);
        }
    }

    ///////////Go go go!! start of simulation/////////////////////
    ctx->add(ctx);
    ctx->add(oh);
    ctx->add(sstConnMgr);
    for(SimList::iterator it = sims.begin(); it != sims.end(); it++)
        ctx->add(*it);
    ctx->run(2);

    ctx->cleanup();
    trace->prepareShutdown();

    provider.reset();
    delete oh;

    // delete after OH in case objects want to do last-minute state flushes
    delete database;

    destroyTransferManager(tm);
    delete eventManager;
    delete workQueue;

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
