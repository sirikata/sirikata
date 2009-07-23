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
#include <ObjectHost_Sirikata.pbj.hpp>

namespace Sirikata {

using Task::GenEventManager;
using Transfer::TransferManager;

OptionValue *cdnConfigFile;
OptionValue *floatExcept;
InitializeGlobalOptions main_options("",
//    simulationPlugins=new OptionValue("simulationPlugins","ogregraphics",OptionValueType<String>(),"List of plugins that handle simulation."),
    cdnConfigFile=new OptionValue("cdnConfig","cdn = ($import=cdn.txt)",OptionValueType<String>(),"CDN configuration."),
    floatExcept=new OptionValue("sigfpe","false",OptionValueType<bool>(),"Enable floating point exceptions"),
    NULL
);

std::string cut(const std::string &source, std::string::size_type start, std::string::size_type end) {
	using namespace std;
    while (isspace(source[start]) && start < end) {
        ++start;
    }
    while (end > start && isspace(source[end-1])) {
        --end;
    }
    return source.substr(start,end-start);
}

namespace {
typedef std::map<OptionMapPtr, std::string> GlobalMap;
std::ostream &display (std::ostream &os, const OptionMap &om, const GlobalMap &globals, int level) {
    int numspaces = level*4;
    for (OptionMap::const_iterator iter = om.begin(); iter != om.end(); ++iter) {
        OptionMapPtr child = (*iter).second;
        os << std::string(numspaces, ' ');
        os << (*iter).first << " = ";
        GlobalMap::const_iterator globaliter = globals.find(child);
        if (level>0 && globaliter != globals.end()) {
            os << "$" << (*globaliter).second;
        } else {
            os << child->getValue();
            if (!child->empty()) {
                os << "(" << std::endl;
                display(os,*child,globals,level+1);
                os << std::string(numspaces, ' ');
                os << ")";
            }
        }
        os << std::endl;
    }
    return os;
}
}
std::ostream &operator<< (std::ostream &os, const OptionMap &om) {
    GlobalMap globals;
    for (OptionMap::const_iterator iter = om.begin(); iter != om.end(); ++iter) {
        globals[(*iter).second] = (*iter).first;
    }
    return display(os, om, globals, 0);
}

enum ParseState {KEYSTATE, EQSTATE, VALUESTATE, NUMSTATES };

void handleCommand(const std::string &input, const OptionMapPtr &globalvariables, const OptionMapPtr &options, std::string::size_type &pos, const std::string &key, const std::string &value) {
    if (key == "$import") {
        std::string::size_type slash = value.rfind('/');
        std::string::size_type backslash = value.rfind('\\');
        if (slash == std::string::npos && backslash ==std::string::npos) {
            std::string fname = "./" + value;
            if (value.find(".txt")!=std::string::npos) {
                FILE *fp = NULL;
                for (int i = 0; i < 4 && fp == NULL; i++) {
                    fp = fopen(fname.c_str(),"rt");
                    fname = "./."+fname;
                }
                std::string str;
                while (fp && !feof(fp)) {
                    char buf[1024];
                    int numread = fread(buf, 1, 1024, fp);
                    if (numread <= 0) {
                        break;
                    }
                    str += std::string(buf, buf+numread);
                }
                if (fp) {
                    fclose(fp);
                } else {
                    SILOG(main,error,"Configuration file "<<value<<" does not exist");
                }
                std::string::size_type subpos = 0;
                // current level becomes the global level.
                parseConfig(str, options, options, subpos);
            }
        }
    } else {
        SILOG(main,error,"Unknown command "<<key<<"("<<value<<")");
    }
}

void parseConfig(
        const std::string &input,
        const OptionMapPtr &globalvariables,
        const OptionMapPtr &options,
        std::string::size_type &pos)
{
    std::string::size_type len = input.length();
    std::string key;
    std::string::size_type beginpos = pos;
    OptionMapPtr currentValue;
    /* states:
       0 = Key, looking for '='
       1 = Looking for value, but have not encountered whitespace.
       2 = Value, will end at next whitespace character.
    */
    ParseState state = KEYSTATE;
    for (; pos <= len; ++pos) {
        char ch;
        if (pos >= len) {
            ch = ')';
        } else {
            ch = input[pos];
        }
        if (state==KEYSTATE && ch == '#') {
            /* comment up to ')' */
            for (; pos < len; ++pos) {
                if (input[pos]=='\n') break;
            }
            beginpos = pos;
        } else if (ch == '(' || ch==')' || (state != KEYSTATE && isspace(ch))) {
            if (state == VALUESTATE) {
                std::string value(cut(input, beginpos, pos));
                if (value.length()) {
                    OptionMapPtr toInsert;
                    if (value[0]=='$') {
                        toInsert = (*globalvariables).get(value.substr(1));
                        if (!toInsert) {
                            toInsert = OptionMapPtr(new OptionMap);
                            (*globalvariables).put(value.substr(1), toInsert);
                        }
                    }
                    if (key.length() && key[0]=='$') {
                        handleCommand(input,globalvariables,options,pos,key,value);
                    } else {
                        if (!toInsert) {
                            toInsert = (*options).get(key);
                            if (!toInsert) {
                                toInsert = OptionMapPtr(new OptionMap(value));
                            }
                        }
                        currentValue = options->put(key, toInsert);
                    }
                    state = KEYSTATE;
                }
                beginpos = pos+1;
           }
            if (ch == '(') {
                if (!currentValue) {
                    currentValue = options->get(key);
                    if (!currentValue) {
                        currentValue = options->put(key, OptionMapPtr(new OptionMap));
                    }
                }
                /* Note: If you have KEY = $variable ( key1=value1 key2=value2 ) will add key1 and key2 to variable as well. */
                pos++;
                parseConfig(input, globalvariables, currentValue, pos);
                beginpos = pos+1;
                state = KEYSTATE;
            } else if (ch == ')') {
                break;
            }
        } else if (state == KEYSTATE && ch=='=') {
            key = cut(input, beginpos, pos);
            state = EQSTATE;
            beginpos = pos+1;
            currentValue = OptionMapPtr();
        } else if (state == EQSTATE && !isspace(ch)) {
            state = VALUESTATE;
        }
    }
}

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
    plugins.load ( DynamicLibrary::filename("ogregraphics") );
    plugins.load ( DynamicLibrary::filename("bulletphysics") );
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


    SpaceID mainSpace(UUID("12345678-1111-1111-1111-DEFA01759ACE",UUID::HumanReadable()));
    SpaceIDMap *spaceMap = new SpaceIDMap;
    spaceMap->insert(mainSpace, Network::Address("127.0.0.1","5943"));

    ObjectHost *oh = new ObjectHost(spaceMap, ioServ);
    {//to deallocate HostedObjects before armageddon
    HostedObjectPtr firstObject (HostedObject::construct<HostedObject>(oh));
    firstObject->initializeConnect(
            UUID::random(),
            Location(Vector3d(0, ((double)(time(NULL)%10)) - 5 , 0), Quaternion::identity(),
                     Vector3f(0.2,0,0), Vector3f(0,0,1), 0.),
            "meru://daniel@/cube.mesh",
            BoundingSphere3f(Vector3f::nil(),1),
            NULL,
            mainSpace);
    firstObject->setScale(Vector3f(3,3,3));

    HostedObjectPtr secondObject (HostedObject::construct<HostedObject>(oh));
    secondObject->initializeConnect(
        UUID::random(),
        Location(Vector3d(0,0,25), Quaternion::identity(),
                 Vector3f(0.1,0,0), Vector3f(0,0,1), 0.),
        "",
        BoundingSphere3f(Vector3f::nil(),0),
        NULL,
        mainSpace);
        
    HostedObjectPtr thirdObject (HostedObject::construct<HostedObject>(oh));
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
            UUID::random(),
            Location(Vector3d(10,0,10), Quaternion::identity(),
                     Vector3f::nil(), Vector3f(0,0,1), 0.),
            "",
            BoundingSphere3f(Vector3f::nil(),0),
            &li,
            mainSpace);
    }
    ProxyManager *provider = oh->getProxyManager(mainSpace);
    
    Task::WorkQueue *workQueue = new Task::LockFreeWorkQueue;
    Task::GenEventManager *eventManager = new Task::GenEventManager(workQueue);
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
    String graphicsPluginName ( "ogregraphics" );
    String physicsPluginName ( "bulletphysics" );
    SILOG(cppoh,error,"dbm: initializing graphics");
    TimeSteppedSimulation *graphicsSystem=
        SimulationFactory::getSingleton()
        .getConstructor ( graphicsPluginName ) ( provider,graphicsCommandArguments );
    SILOG(cppoh,error,"dbm: initializing physics");
    TimeSteppedSimulation *physicsSystem=
        SimulationFactory::getSingleton()
        .getConstructor ( physicsPluginName ) ( provider,graphicsCommandArguments );
    if (!physicsSystem) {
        SILOG(cppoh,error,"physicsSystem NULL!");
    }
    else {
        SILOG(cppoh,error,"physicsSystem: " << std::hex << (unsigned long)physicsSystem);
    }
    if ( graphicsSystem ) {
        while ( graphicsSystem->tick() ) {
            physicsSystem->tick();
            Network::IOServiceFactory::pollService(ioServ);
        }
    } else {
        SILOG(cppoh,error,"Fatal Error: Unable to load OGRE Graphics plugin. The PATH environment variable is ignored, so make sure you have copied the DLLs from dependencies/ogre/bin/ into the current directory. Sorry about this!");
        std::cout << "Press enter to continue" << std::endl;
        std::cerr << "Press enter to continue" << std::endl;
        fgetc(stdin);
    }
    delete graphicsSystem;

    
    delete eventManager;
    delete workQueue;
    }
    plugins.gc();
    SimulationFactory::destroy();

    delete oh;
    Network::IOServiceFactory::destroyIOService(ioServ);
    delete spaceMap;

    delete []myargv;

    return 0;
}
