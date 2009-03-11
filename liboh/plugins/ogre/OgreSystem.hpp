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

#ifndef _SIRIKATA_OGRE_GRAPHICS_
#define _SIRIKATA_OGRE_GRAPHICS_
#include <util/Platform.hpp>
#include <util/ListenerProvider.hpp>
#include <ProxyCreationListener.hpp>
#include <OgreRoot.h>
namespace Sirikata { namespace Graphics {

class OgreSystem: public ProxyCreationListener {
    Ogre::SceneManager *mSceneManager;
    Ogre::RenderTarget *mRenderTarget;
    OptionValue*mWindowWidth;
    OptionValue*mWindowHeight;
    OptionValue*mFullScreen;
    OptionValue* mOgreRootDir;
    static uint32 sNumOgreSystems;
    static Ogre::Plugin*sCDNArchivePlugin;
    static Ogre::Root *sRoot;
    Provider<ProxyCreationListener*>*mProxyManager;
    bool loadBuiltinPlugins();
    OgreSystem();
    bool initialize(Provider<ProxyCreationListener*>*proxyManager,
                    const String&options);
public:
    static ProxyCreationListener* create(Provider<ProxyCreationListener*>*proxyManager,
                                         const String&options){
        OgreSystem*os= new OgreSystem;
        if (os->initialize(proxyManager,options))
            return os;
        delete os;
        return NULL;
    }
    Ogre::RenderTarget *getRenderTarget();
    static Ogre::Root *getRoot();
    Ogre::SceneManager* getSceneManager();
    virtual void createProxy(ProxyObjectPtr p);
    virtual void destroyProxy(ProxyObjectPtr p);
    ~OgreSystem();
};


} }
#endif
