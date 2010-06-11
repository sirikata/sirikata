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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/space/Space.hpp>
#include <sirikata/core/task/EventManager.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/options/CDNConfig.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ModelsSystemFactory.hpp> // MCB:
namespace Sirikata {
//InitializeOptions main_options("verbose",
void tickSim(Network::IOService*io,TimeSteppedSimulation*sim) {
    sim->tick();
    io->post(Duration::seconds(1./30.),std::tr1::bind(&tickSim,io,sim));

}
}
int main(int argc,const char**argv) {
    using namespace Sirikata;

    OptionValue *spaceOption;
    OptionValue *loadGraphics;
    OptionValue *loadPhysics;
    InitializeGlobalOptions gbo("",spaceOption=new OptionValue("space","",OptionValueType<String>(),"Options passed to the space"),loadGraphics=new OptionValue("graphics","false",OptionValueType<bool>(),"Load the graphics plugin on the space"),loadPhysics=new OptionValue("physics","false",OptionValueType<bool>(),"Load the physics plugin on the space"),NULL);

    OptionSet::getOptions("")->parse(argc,argv);
    PluginManager plugins;

    const char* pluginNames[] = { "tcpsst", "prox", "prox-everyone", "colladamodels", "ogregraphics", "bulletphysics", NULL};
    for(const char** plugin_name = pluginNames; *plugin_name != NULL; plugin_name++) {
        if (strcmp(*plugin_name,"ogregraphics")!=0||loadGraphics->as<bool>())
            if (strcmp(*plugin_name,"bulletphysics")!=0||loadPhysics->as<bool>())
                plugins.load( DynamicLibrary::filename(*plugin_name) );
    }

    Space::Space space(SpaceID(UUID("12345678-1111-1111-1111-DEFA01759ACE", UUID::HumanReadable())),spaceOption->as<String>());
    ProxyManager *provider=space.getProxyManager();
    Task::WorkQueue *workQueue = new Task::LockFreeWorkQueue;
    Task::GenEventManager *eventManager = new Task::GenEventManager(workQueue);
    typedef std::vector<TimeSteppedSimulation*> SimList;
    SimList sims;
    Transfer::TransferManager *tm=NULL;

    if (provider) {
        try {
            OptionMapPtr transferOptions(new OptionMap);
            size_t offset=0;
            parseConfig("cdn = ($import=cdn.txt)",transferOptions,transferOptions,offset);
            tm = initializeTransferManager((*transferOptions)["cdn"], eventManager);
        } catch (OptionDoesNotExist &err) {
            SILOG(input,fatal,"Fatal Error: Failed to load CDN config: " << err.what());
            std::cout << "Press enter to continue" << std::endl;
            std::cerr << "Press enter to continue" << std::endl;
            fgetc(stdin);
            return 1;
        }
        initializeProtocols();
        String graphicsCommandArguments;
        {
            std::ostringstream os;
            os << "--transfermanager=" << tm << " ";
            os << "--eventmanager=" << eventManager << " ";
            os << "--workqueue=" << workQueue << " ";
            graphicsCommandArguments = os.str();
        }

        // MCB: seems like a good place to initialize models system
        ModelsSystem* mm ( ModelsSystemFactory::getSingleton ().getConstructor ( "colladamodels" ) ( provider, graphicsCommandArguments ) );

        if ( mm )
        {
            SILOG(cppoh,info,"Created ModelsSystemFactory ");
        }
        else
        {
            SILOG(cppoh,error,"Failed to create ModelsSystemFactory ");
        }
            struct SimulationRequest {
        const char* name;
        bool required;
    };
    const uint32 nSimRequests = 2;
    SimulationRequest simRequests[nSimRequests] = {
        {"ogregraphics", true},
        {"bulletphysics", false}
    };
    bool continue_simulation=true;
    for(uint32 ir = 0; ir < nSimRequests && continue_simulation; ir++) {
        String simName = simRequests[ir].name;
        SILOG(cppoh,info,String("Initializing ") + simName);
        TimeSteppedSimulation *sim =
            SimulationFactory::getSingleton()
            .getConstructor ( simName ) ( provider, provider->getTimeOffsetManager(), graphicsCommandArguments );
        if (!sim) {
            if (simRequests[ir].required) {
                SILOG(cppoh,error,String("Unable to load ") + simName + String(" plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!"));
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

    }
    for(SimList::iterator it = sims.begin(); it != sims.end(); it++) {

        space.getIOService()->post(std::tr1::bind(&tickSim,space.getIOService(),*it));
    }
    space.run();
    return 0;
}
