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
#include <ObjectHost_Sirikata.pbj.hpp>
#include <time.h>
namespace Sirikata {

using Task::GenEventManager;
using Transfer::TransferManager;

OptionValue *cdnConfigFile;
OptionValue *floatExcept;
OptionValue *dbFile;
InitializeGlobalOptions main_options("",
//    simulationPlugins=new OptionValue("simulationPlugins","ogregraphics",OptionValueType<String>(),"List of plugins that handle simulation."),
    cdnConfigFile=new OptionValue("cdnConfig","cdn = ($import=cdn.txt)",OptionValueType<String>(),"CDN configuration."),
    floatExcept=new OptionValue("sigfpe","false",OptionValueType<bool>(),"Enable floating point exceptions"),
    dbFile=new OptionValue("db","scene.db",OptionValueType<String>(),"Persistence database"),
    NULL
);

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
    spaceMap->insert(mainSpace, Network::Address("127.0.0.1","5943"));

    Persistence::ReadWriteHandler *database=Persistence::ReadWriteHandlerFactory::getSingleton()
        .getConstructor("sqlite")(String("--databasefile ")+dbFile->as<String>());

    ObjectHost *oh = new ObjectHost(spaceMap, workQueue, ioServ);
    oh->registerService(Services::PERSISTENCE, database);

    {//to deallocate HostedObjects before armageddon
    HostedObjectPtr firstObject (HostedObject::construct<HostedObject>(oh, UUID::random()));
    firstObject->initializeConnect(
            Location(Vector3d(0, ((double)(time(NULL)%10)) - 5 , 0), Quaternion::identity(),
                     Vector3f(0.2,0,0), Vector3f(0,0,1), 0.),
            "meru://daniel@/cube.mesh",
            BoundingSphere3f(Vector3f::nil(),1),
            NULL,
            mainSpace);
    firstObject->setScale(Vector3f(3,3,3));

    HostedObjectPtr secondObject (HostedObject::construct<HostedObject>(oh, UUID::random()));
    secondObject->initializeConnect(
        Location(Vector3d(0,0,25), Quaternion::identity(),
                 Vector3f(0.1,0,0), Vector3f(0,0,1), 0.),
        "",
        BoundingSphere3f(Vector3f::nil(),0),
        NULL,
        mainSpace);

    HostedObjectPtr thirdObject (HostedObject::construct<HostedObject>(oh, UUID::random()));
    {
        LightInfo li;
        li.setLightDiffuseColor(Color(0.976471, 0.992157, 0.733333));
        li.setLightAmbientColor(Color(.24,.25,.18));
        li.setLightSpecularColor(Color(0,0,0));
        li.setLightShadowColor(Color(0,0,0));
        li.setLightPower(1.0);
        li.setLightRange(75);
        li.setLightFalloff(1,0,0.03);
        li.setLightSpotlightCone(30,40,1);
        li.setCastsShadow(true);
        thirdObject->initializeConnect(
            Location(Vector3d(10,0,10), Quaternion::identity(),
                     Vector3f::nil(), Vector3f(0,0,1), 0.),
            "",
            BoundingSphere3f(Vector3f::nil(),0),
            &li,
            mainSpace);
    }

    HostedObjectPtr scriptedObject (HostedObject::construct<HostedObject>(oh, UUID::random()));
    {
    std::map<String,String> args;
    args["Assembly"]="Sirikata.Runtime";
    args["Class"]="PythonObject";
    args["Namespace"]="Sirikata.Runtime";
    args["PythonModule"]="test";
    args["PythonClass"]="exampleclass";
    scriptedObject->initializeScript("monoscript", args);
    scriptedObject->initializeConnect(
            Location(Vector3d(0, ((double)(time(NULL)%10)) - 5 , 0), Quaternion::identity(),
                     Vector3f(0.2,0,0), Vector3f(0,0,1), 0.),
            "meru://daniel@/cube.mesh",
            BoundingSphere3f(Vector3f::nil(),1),
            NULL,
            mainSpace);
    scriptedObject->setScale(Vector3f(3,3,3));
    }
    }
    ProxyManager *provider = oh->getProxyManager(mainSpace);

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
        }
    }
    while ( continue_simulation ) {
        for(SimList::iterator it = sims.begin(); it != sims.end(); it++) {
            continue_simulation = continue_simulation && (*it)->tick();
        }
        Network::IOServiceFactory::pollService(ioServ);
    }

    delete oh;

    // delete after OH in case objects want to do last-minute state flushes
    delete database;

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
