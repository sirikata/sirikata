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
#include <proxyobject/ModelsSystemFactory.hpp> // MCB:
#include <util/RoutableMessageHeader.hpp>
#include <options/Options.hpp>
#include <util/PluginManager.hpp>
#include <proxyobject/SimulationFactory.hpp>

#include <task/EventManager.hpp>
#include <task/WorkQueue.hpp>
#include "CDNConfig.hpp"

#include <oh/ObjectHost.hpp>
#include <proxyobject/LightInfo.hpp>
#include <oh/TopLevelSpaceConnection.hpp>
#include <oh/SpaceConnection.hpp>
#include <oh/HostedObject.hpp>
#include <oh/SpaceIDMap.hpp>
#include <oh/ObjectHostTimeOffsetManager.hpp>
#include <network/IOServiceFactory.hpp>
#include <network/IOService.hpp>
#include <util/KnownServices.hpp>
#include <persistence/ObjectStorage.hpp>
#include <persistence/ReadWriteHandlerFactory.hpp>
#include <ObjectHostBinary_Persistence.pbj.hpp>
#include <ObjectHostBinary_Sirikata.pbj.hpp>
#include <time.h>
#include <boost/thread.hpp>
#include <options/Options.hpp>
namespace Sirikata {

using Task::GenEventManager;
using Transfer::TransferManager;

OptionValue *cdnConfigFile;
OptionValue *floatExcept;
OptionValue *dbFile;
OptionValue *host;
OptionValue *frameRate;
InitializeGlobalOptions main_options("",
//    simulationPlugins=new OptionValue("simulationPlugins","ogregraphics",OptionValueType<String>(),"List of plugins that handle simulation."),
    cdnConfigFile=new OptionValue("cdnConfig","cdn = ($import=cdn.txt)",OptionValueType<String>(),"CDN configuration."),
    floatExcept=new OptionValue("sigfpe","false",OptionValueType<bool>(),"Enable floating point exceptions"),
    dbFile=new OptionValue("db","scene.db",OptionValueType<String>(),"Persistence database"),
    frameRate=new OptionValue("framerate","60",OptionValueType<double>(),"The desired framerate at which to run the object host"),
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
    OptionValue *ohOption;
    OptionValue *spaceIdMapOption;
    OptionValue *mainSpaceOption;
    InitializeGlobalOptions gbo("",
                                ohOption=new OptionValue("objecthost","",OptionValueType<String>(),"Options passed to the object host"),
                                mainSpaceOption=new OptionValue("mainspace","12345678-1111-1111-1111-DEFA01759ACE",OptionValueType<UUID>(),"space which to connect default objects to"),
                                spaceIdMapOption=new OptionValue("spaceidmap",
                                                                 "12345678-1111-1111-1111-DEFA01759ACE:{127.0.0.1:5943}",
                                                                 OptionValueType<std::map<std::string,std::string> >(),
                                                                 "Map between space ID's and tcpsst ip's"),
                                NULL);

    PluginManager plugins;
    const char* pluginNames[] = { "tcpsst", "monoscript", "sqlite", "ogregraphics", "bulletphysics","colladamodels", NULL};
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
    SpaceIDMap *spaceMap = new SpaceIDMap;
    SpaceID mainSpace(mainSpaceOption->as<UUID>());
    for (std::map<std::string,std::string>::iterator i=spaceIdMapOption->as<std::map<std::string,std::string> >().begin(),
             ie=spaceIdMapOption->as<std::map<std::string,std::string> >().end();
         i!=ie;
         ++i) {
        SpaceID newSpace(UUID(i->first,UUID::HumanReadable()));
        spaceMap->insert(newSpace, Network::Address::lexical_cast(i->second).as<Network::Address>());
    }
    String localDbFile=dbFile->as<String>();
    if (localDbFile.length()&&localDbFile[0]!='/'&&localDbFile[0]!='\\') {
        FILE * fp=fopen(localDbFile.c_str(),"rb");
        for (int i=0;i<4&&fp==NULL;++i) {
            localDbFile="../"+localDbFile;
            fp=fopen(localDbFile.c_str(),"rb");
        }
        if (fp) fclose(fp);
        else localDbFile=dbFile->as<String>();
    }
    Persistence::ReadWriteHandler *database=Persistence::ReadWriteHandlerFactory::getSingleton()
        .getConstructor("sqlite")(String("--databasefile ")+localDbFile);

    ObjectHost *oh = new ObjectHost(spaceMap, workQueue, ioServ, "");
    oh->registerService(Services::PERSISTENCE, database);

    {
        UUIDLister lister(oh, mainSpace);
        lister.goWait(ioServ, workQueue);
    }

    std::tr1::shared_ptr<ProxyManager> provider = oh->connectToSpace(mainSpace);
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
            .getConstructor ( simName ) ( provider.get(), provider->getTimeOffsetManager(), graphicsCommandArguments );
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
        }
    }
    Duration frameTime = Duration::seconds(1.0/frameRate->as<double>());
    Task::LocalTime curTickTime(Task::LocalTime::now());
    Task::LocalTime lastTickTime=curTickTime;
    while ( continue_simulation ) {
        for(SimList::iterator it = sims.begin(); it != sims.end(); it++) {
            continue_simulation = continue_simulation && (*it)->tick();
        }
        oh->tick();

        curTickTime=Task::LocalTime::now();
        Duration frameSeconds=(curTickTime-lastTickTime);
        if (frameSeconds<frameTime) {
            //printf ("%f/%f Sleeping for %f\n",frameSeconds.toSeconds(), frameTime.toSeconds(),(frameTime-frameSeconds).toSeconds());

            ioServ->post(
                                                              (frameTime-frameSeconds),
                                                              std::tr1::bind(&Network::IOService::stop,
                                                                             ioServ)
                                                              );
            ioServ->run();
            ioServ->reset();
        }else {
            ioServ->poll();
        }
        lastTickTime=curTickTime;
    }

    provider.reset();
    delete oh;

    // delete after OH in case objects want to do last-minute state flushes
    delete database;

    destroyTransferManager(tm);
    delete eventManager;
    delete workQueue;
    Network::IOServiceFactory::destroyIOService(ioServ);

    for(SimList::reverse_iterator it = sims.rbegin(); it != sims.rend(); it++) {
        delete *it;
    }
    sims.clear();
    plugins.gc();
    SimulationFactory::destroy();

    delete spaceMap;

    delete []myargv;

    return 0;
}
