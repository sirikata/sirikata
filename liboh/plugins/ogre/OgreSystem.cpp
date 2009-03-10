/*  Sirikata liboh -- Ogre Graphics Plugin
 *  OgreGraphics.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#include "OgreSystem.hpp"
#include <OgrePlugin.h>
volatile char assert_thread_support_is_gequal_2[OGRE_THREAD_SUPPORT*2-3]={0};
volatile char assert_thread_support_is_lequal_2[5-OGRE_THREAD_SUPPORT*2]={0};

namespace Sirikata {
namespace Graphics {
Ogre::Root *OgreSystem::sRoot;
Ogre::Plugin*OgreSystem::sCDNArchivePlugin=NULL;
uint32 OgreSystem::sNumOgreSystems=0;
OgreSystem::OgreSystem(){
    mSceneManager=NULL;
    mRenderTarget=NULL;
    mProxyManager=NULL;
}
bool OgreSystem::initialize(Provider<ProxyCreationListener*>*proxyManager, const String&options) {
    mProxyManager=proxyManager;
    ++sNumOgreSystems;
    proxyManager->addListener(this);
    //add ogre system options here
    OptionValue*pluginFile;
    OptionValue*configFile;
    OptionValue*ogreLogFile;
    OptionValue*restoreConfig;
    OptionValue*createWindow;
    OptionValue*autoCreateWindow;
    OptionValue*windowTitle;
    
    InitializeClassOptions("ogre",this,
                           pluginFile=new OptionValue("pluginfile","plugins.cfg",OptionValueType<String>(),"sets the file ogre should read options from."),
                           configFile=new OptionValue("configfile","ogre.cfg",OptionValueType<String>(),"sets the ogre config file for config options"),
                           ogreLogFile=new OptionValue("logfile","Ogre.log",OptionValueType<String>(),"sets the ogre log file"),
                           restoreConfig=new OptionValue("purgeconfig","false",OptionValueType<bool>(),"Pops up the dialog asking for the screen resolution no matter what"),
                           createWindow=new OptionValue("window","true",OptionValueType<bool>(),"Render to a onscreen window"),
                           autoCreateWindow=new OptionValue("autowindow","true",OptionValueType<bool>(),"Render to a onscreen window"),
                           windowTitle=new OptionValue("windowtitle","Sirikata",OptionValueType<String>(),"Window title name"),
                           mOgreRootDir=new OptionValue("ogreroot",".",OptionValueType<String>(),"Directory with ogre plugins"),
                           mWindowWidth=new OptionValue("windowwidth","1024",OptionValueType<uint32>(),"Window width"),
                           mFullScreen=new OptionValue("fullscreen","false",OptionValueType<bool>(),"Fullscreen"),
                           mWindowHeight=new OptionValue("windowheight","768",OptionValueType<uint32>(),"Window height"),
        NULL);
    bool userAccepted=true;

    OptionSet::getOptions("ogre",this)->parse(options);

    static bool success=((sRoot=OGRE_NEW Ogre::Root(pluginFile->as<String>(),configFile->as<String>(),ogreLogFile->as<String>()))!=NULL
                         &&loadBuiltinPlugins()
                         &&((restoreConfig->as<bool>()&&getRoot()->restoreConfig()) 
                            || (userAccepted=getRoot()->showConfigDialog())));
    if (userAccepted&&success) {
        if (!getRoot()->isInitialised()) {
            bool doAutoWindow=autoCreateWindow->as<bool>();
            sRoot->initialise(doAutoWindow,windowTitle->as<String>());      
            if (!doAutoWindow) {
                getRoot()->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>());
            }
        } else if (createWindow->as<bool>()) {
            sRoot->createRenderWindow(windowTitle->as<String>(),mWindowWidth->as<uint32>(),mWindowHeight->as<uint32>(),mFullScreen->as<bool>());
        }else {
            assert(false&&"Cant remember how to create a render targt");//support creating a render target here
        }
    }
    if (!getRoot()->isInitialised()) {
        return false;
    }

    return true;
}
namespace {
bool ogreLoadPlugin(String root, const String&filename, bool recursive=true) {
    root+='/';
    root+=filename;
#ifndef __APPLE__
    FILE *fp=fopen(root.c_str(),"rb");
#endif
    if (
#ifndef __APPLE__
        fp
#else
        true
#endif
        ) {
#ifndef __APPLE__
        fclose(fp);
#endif
        Ogre::Root::getSingleton().loadPlugin(root);
        return true;
    }else {
        if (recursive)  {
            if (ogreLoadPlugin("../../dependencies/ogre-1.6.1/lib",filename,false))
                return true;
            if (ogreLoadPlugin("../../dependencies/ogre-1.6.x/lib",filename,false))
                return true;
            if (ogreLoadPlugin("../../dependencies/ogre-1.6.1/lib/OGRE",filename,false))
                return true;
            if (ogreLoadPlugin("../../dependencies/ogre-1.6.x/lib/OGRE",filename,false))
                return true;
            if (ogreLoadPlugin("../../dependencies/lib/OGRE",filename,false))
                return true;
            if (ogreLoadPlugin("/usr/local/lib/OGRE",filename,false))
                return true;
            if (ogreLoadPlugin("/usr/lib/OGRE",filename,false))
                return true;
        }
    }
    return false;
}
}
bool OgreSystem::loadBuiltinPlugins () {
    bool retval=true;
#ifdef __APPLE__
    ogreLoadPlugin(String(),"RenderSystem_GL");
    ogreLoadPlugin(String(),"Plugin_CgProgramManager");
    ogreLoadPlugin(String(),"Plugin_ParticleFX");
    ogreLoadPlugin(String(),"Plugin_OctreeSceneManager");
#else
#ifdef _WIN32
#ifdef NDEBUG
   #define OGRE_DEBUG_MACRO ".dll"
#else
   #define OGRE_DEBUG_MACRO "_d.dll"
#endif
#else
#ifdef NDEBUG
   #define OGRE_DEBUG_MACRO ".so"
#else
   #define OGRE_DEBUG_MACRO ".so"
#endif
#endif
    retval=ogreLoadPlugin(mOgreRootDir->as<String>(),"RenderSystem_GL" OGRE_DEBUG_MACRO);
    ogreLoadPlugin(mOgreRootDir->as<String>(),"RenderSystem_Direct3D9" OGRE_DEBUG_MACRO);
    ogreLoadPlugin(mOgreRootDir->as<String>(),"Plugin_CgProgramManager" OGRE_DEBUG_MACRO);
    ogreLoadPlugin(mOgreRootDir->as<String>(),"Plugin_ParticleFX" OGRE_DEBUG_MACRO);
    ogreLoadPlugin(mOgreRootDir->as<String>(),"Plugin_OctreeSceneManager" OGRE_DEBUG_MACRO);
#undef OGRE_DEBUG_MACRO
#endif
    return retval;
    /*
    sCDNArchivePlugin=new CDNArchivePlugin;
    getRoot()->installPlugin(&*sCDNArchivePlugin);
    */
}
Ogre::SceneManager* OgreSystem::getSceneManager(){
    return mSceneManager;
}
Ogre::Root*OgreSystem::getRoot() {
    return &Ogre::Root::getSingleton();
}
Ogre::RenderTarget*OgreSystem::getRenderTarget() {
    return mRenderTarget;
}
OgreSystem::~OgreSystem() {
    mProxyManager->removeListener(this);    
    --sNumOgreSystems;
    if (sNumOgreSystems==0) {
        OGRE_DELETE sCDNArchivePlugin;
        sCDNArchivePlugin=NULL;
        OGRE_DELETE sRoot;
        sRoot=NULL;
    }
}

void OgreSystem::createProxy(ProxyObjectPtr p){
    assert(false&&"Need to implement create proxy");
}
void OgreSystem::destroyProxy(ProxyObjectPtr p){
    assert(false&&"Need to destroy create proxy");
}

}
}
