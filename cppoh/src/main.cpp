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

#include <oh/Platform.hpp>
#include <util/RoutableMessageHeader.hpp>
#include <options/Options.hpp>
#include <util/PluginManager.hpp>
#include <oh/SimulationFactory.hpp>

#include <task/EventManager.hpp>
#include <task/WorkQueue.hpp>
#include "CDNConfig.hpp"

#include <oh/ObjectHost.hpp>
#include <oh/LightInfo.hpp>
#include <oh/TopLevelSpaceConnection.hpp>
#include <oh/SpaceConnection.hpp>
#include <oh/HostedObject.hpp>
#include <oh/SpaceIDMap.hpp>
#include <network/IOServiceFactory.hpp>
#include <util/KnownServices.hpp>
#include <persistence/ObjectStorage.hpp>
#include <persistence/ReadWriteHandlerFactory.hpp>
#include <ObjectHostBinary_Persistence.pbj.hpp>
#include <ObjectHostBinary_Sirikata.pbj.hpp>
#include <time.h>
namespace Sirikata {

using Task::GenEventManager;
using Transfer::TransferManager;

OptionValue *cdnConfigFile;
OptionValue *floatExcept;
OptionValue *dbFile;
OptionValue *host;
InitializeGlobalOptions main_options("",
//    simulationPlugins=new OptionValue("simulationPlugins","ogregraphics",OptionValueType<String>(),"List of plugins that handle simulation."),
    cdnConfigFile=new OptionValue("cdnConfig","cdn = ($import=cdn.txt)",OptionValueType<String>(),"CDN configuration."),
    floatExcept=new OptionValue("sigfpe","false",OptionValueType<bool>(),"Enable floating point exceptions"),
    dbFile=new OptionValue("db","scene.db",OptionValueType<String>(),"Persistence database"),
    host=new OptionValue("host","localhost",OptionValueType<String>(),"space address"),
    NULL
);

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
    void goWait(Network::IOService *ioServ, Task::WorkQueue *queue) {
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

int main ( int argc,const char**argv ) {

    int myargc = argc+2;
    const char **myargv = new const char*[myargc];
    memcpy(myargv, argv, argc*sizeof(const char*));
    myargv[argc] = "--moduleloglevel";
    myargv[argc+1] = "transfer=fatal,ogre=fatal,task=fatal,resource=fatal";

    using namespace Sirikata;

    PluginManager plugins;
    const char* pluginNames[] = { "tcpsst", "monoscript", "sqlite", "ogregraphics", "bulletphysics", NULL};
    for(const char** plugin_name = pluginNames; *plugin_name != NULL; plugin_name++)
        plugins.load( DynamicLibrary::filename(*plugin_name) );

    OptionSet::getOptions ( "" )->parse ( myargc,myargv );

#ifdef __GNUC__
#ifndef __APPLE__
    if (floatExcept->as<bool>()) {
        feenableexcept(FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW|FE_UNDERFLOW);
    }
#endif
#endif

    OptionMapPtr transferOptions (new OptionMap);
    {
        std::string contents(cdnConfigFile->as<String>());
        std::string::size_type pos(0);
        parseConfig(contents, transferOptions, transferOptions, pos);
        std::cout << *transferOptions;
    }

    initializeProtocols();

    Network::IOService *ioServ = Network::IOServiceFactory::makeIOService();
    Task::WorkQueue *workQueue = new Task::LockFreeWorkQueue;
    Task::GenEventManager *eventManager = new Task::GenEventManager(workQueue);

    SpaceID mainSpace(UUID("12345678-1111-1111-1111-DEFA01759ACE",UUID::HumanReadable()));
    SpaceIDMap *spaceMap = new SpaceIDMap;
    spaceMap->insert(mainSpace, Network::Address(host->as<String>(),"5943"));

    Persistence::ReadWriteHandler *database=Persistence::ReadWriteHandlerFactory::getSingleton()
        .getConstructor("sqlite")(String("--databasefile ")+dbFile->as<String>());

    ObjectHost *oh = new ObjectHost(spaceMap, workQueue, ioServ);
    oh->registerService(Services::PERSISTENCE, database);

    {
        UUIDLister lister(oh, mainSpace);
        lister.goWait(ioServ, workQueue);
    }

    ProxyManager *provider = oh->getProxyManager(mainSpace);
    if (!provider) {
        SILOG(cppoh,error,String("Unable to load database in ") + String(dbFile->as<String>()));
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
    OptionSet::getOptions("")->parse(myargc,myargv);

    String graphicsCommandArguments;
    {
        std::ostringstream os;
        os << "--transfermanager=" << tm << " ";
        os << "--eventmanager=" << eventManager << " ";
        os << "--workqueue=" << workQueue << " ";
        graphicsCommandArguments = os.str();
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
    const uint32 nSimRequests = 2;
    SimulationRequest simRequests[nSimRequests] = {
        {"ogregraphics", true},
        {"bulletphysics", false}
    };
    for(uint32 ir = 0; ir < nSimRequests && continue_simulation; ir++) {
        String simName = simRequests[ir].name;
        SILOG(cppoh,info,String("Initializing ") + simName);
        TimeSteppedSimulation *sim =
            SimulationFactory::getSingleton()
            .getConstructor ( simName ) ( provider,graphicsCommandArguments );
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
            SILOG(cppoh,info,String("Successfully initialized ") + simName);
            sims.push_back(sim);
			sim->forwardMessagesTo(oh);
        }
    }
    while ( continue_simulation ) {
        for(SimList::iterator it = sims.begin(); it != sims.end(); it++) {
            continue_simulation = continue_simulation && (*it)->tick();
        }
        Network::IOServiceFactory::pollService(ioServ);
    }
	for(SimList::iterator it = sims.begin(); it != sims.end(); it++) {
		(*it)->endForwardingMessagesTo(oh);
	}
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

    Network::IOServiceFactory::destroyIOService(ioServ);
    delete spaceMap;

    delete []myargv;

    return 0;
}
