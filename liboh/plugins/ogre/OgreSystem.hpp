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
#include <util/Time.hpp>
#include <util/ListenerProvider.hpp>
#include <oh/TimeSteppedSimulation.hpp>
#include <oh/ProxyObject.hpp>
#include <OgrePrerequisites.h>
#include <OgreResourceManager.h>
#include <OgrePixelFormat.h>
//Thank you Apple:
// /System/Library/Frameworks/CoreServices.framework/Headers/../Frameworks/CarbonCore.framework/Headers/MacTypes.h
#ifdef nil
#undef nil
#endif

namespace Meru {
class CDNArchivePlugin;
class ResourceFileUpload;
typedef int ResourceUploadStatus;
}

namespace Ogre {
struct RaySceneQueryResultEntry;
}

namespace Sirikata {
class ProxyPositionObject;
namespace Input {
class SDLInputManager;
}
namespace Task {
class EventResponse;
class Event;
typedef std::tr1::shared_ptr<Event> EventPtr;
}
namespace Transfer {
class TransferManager;
}
/** Namespace for the OGRE Graphics Plugin: see class OgreSystem. */
namespace Graphics {
class Entity;
using Input::SDLInputManager;
class CameraEntity;

/** Represents one OGRE SceneManager, a single environment. */
class OgreSystem: public TimeSteppedSimulation {
    class MouseHandler; // Defined in OgreSystemMouseHandler.cpp.
    friend class MouseHandler;
    MouseHandler *mMouseHandler;
    class Transfer::TransferManager *mTransferManager;
    Task::EventResponse performUpload(Task::EventPtr ev);
    void allocMouseHandler();
    void destroyMouseHandler();

    SDLInputManager *mInputManager;
    Ogre::SceneManager *mSceneManager;
    static Ogre::RenderTarget *sRenderTarget;
    Ogre::RenderTarget *mRenderTarget;
    typedef std::tr1::unordered_map<SpaceObjectReference,Entity*,SpaceObjectReference::Hasher> SceneEntitiesMap;
    SceneEntitiesMap mSceneEntities;
    std::list<Entity*> mMovingEntities;
    friend class Entity; //Entity will insert/delete itself from these arrays.
    friend class CameraEntity; //CameraEntity will insert/delete itself from the scene cameras array.
    OptionValue*mWindowWidth;
    OptionValue*mWindowHeight;
    OptionValue*mWindowDepth;
    OptionValue*mFullScreen;
    OptionValue* mOgreRootDir;
    ///How many seconds we aim to spend in each frame
    OptionValue*mFrameDuration;
    OptionSet*mOptions;
    Time mLastFrameTime;
    static Ogre::Plugin*sCDNArchivePlugin;
    static Ogre::Root *sRoot;
    static ::Meru::CDNArchivePlugin *mCDNArchivePlugin;
    Provider<ProxyCreationListener*>*mProxyManager;
    bool loadBuiltinPlugins();
    OgreSystem();
    bool initialize(Provider<ProxyCreationListener*>*proxyManager,
                    const String&options);
    bool renderOneFrame(Time, Duration frameTime);
    ///all the things that should happen just before the frame
    void preFrame(Time, Duration);
    ///all the things that should happen once the frame finishes
    void postFrame(Time, Duration);
    void destroyRenderTarget(Ogre::ResourcePtr &name);
    Ogre::RenderTarget* createRenderTarget(const String &name, uint32 width, uint32 height, bool automipmap, Ogre::PixelFormat pf);
    Vector3d mFloatingPointOffset;
    Ogre::RaySceneQuery* mRayQuery;

public:
    OptionValue *mParallaxSteps;
    OptionValue *mParallaxShadowSteps;
    static std::list<OgreSystem*> sActiveOgreScenes;
    static uint32 sNumOgreSystems;
    std::list<CameraEntity*> mAttachedCameras;
    CameraEntity *mPrimaryCamera;

    SDLInputManager *getInputManager() {
        return mInputManager;
    }
    OptionSet*getOptions(){
        return mOptions;
    }
    const OptionSet*getOptions()const{
        return mOptions;
    }
    void selectObject(Entity *obj, bool reset=true); // Defined in OgreSystemMouseHandler.cpp
    const Vector3d& getOffset()const {return mFloatingPointOffset;}
    void destroyRenderTarget(const String &name);
    ///creates or restores a render target. if name is 0 length it will return the render target associated with this OgreSystem
    Ogre::RenderTarget* createRenderTarget(String name,uint32 width=0, uint32 height=0);
    static TimeSteppedSimulation* create(Provider<ProxyCreationListener*>*proxyManager,
                                         const String&options){
        OgreSystem*os= new OgreSystem;
        if (os->initialize(proxyManager,options))
            return os;
        delete os;
        return NULL;
    }
    void uploadFinished(const std::map<Meru::ResourceFileUpload,Meru::ResourceUploadStatus>&);
    Entity* getEntity(const SpaceObjectReference &proxyId) const {
        SceneEntitiesMap::const_iterator iter = mSceneEntities.find(proxyId);
        if (iter != mSceneEntities.end()) {
            return (*iter).second;
        } else {
            return NULL;
        }
    }
    Entity* getEntity(const ProxyObjectPtr &proxy) const {
        return getEntity(proxy->getObjectReference());
    }
    Entity* rayTrace(const Vector3d &position,
                     const Vector3f &direction,
                     double &returnResult,
                     int which=0) const;
    virtual Duration desiredTickRate()const;
    ///returns if rendering should continue
    virtual bool tick();    
    Ogre::RenderTarget *getRenderTarget();
    static Ogre::Root *getRoot();
    Ogre::SceneManager* getSceneManager();
    virtual void createProxy(ProxyObjectPtr p);
    virtual void destroyProxy(ProxyObjectPtr p);
    ~OgreSystem();
};


} }
#endif
