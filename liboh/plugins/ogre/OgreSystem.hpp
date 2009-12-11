/*  Sirikata liboh -- Ogre Graphics Plugin
 *  OgreSystem.hpp
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
#include <oh/TimeSteppedQueryableSimulation.hpp>
#include <oh/ProxyObject.hpp>
#include "OgreHeaders.hpp"
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
class SubEntity;
}

namespace Sirikata {
class ProxyObject;
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
class CubeMap;

/** Represents one OGRE SceneManager, a single environment. */
class OgreSystem: public TimeSteppedQueryableSimulation {
    class MouseHandler; // Defined in OgreSystemMouseHandler.cpp.
    friend class MouseHandler;
    MouseHandler *mMouseHandler;
    class Transfer::TransferManager *mTransferManager;
    Task::EventResponse performUpload(Task::EventPtr ev);
    void allocMouseHandler();
    void destroyMouseHandler();
    void tickInputHandler(const Task::LocalTime& t) const;

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
    Task::LocalTime mLastFrameTime;
    static Ogre::Plugin*sCDNArchivePlugin;
    static Ogre::Root *sRoot;
    static ::Meru::CDNArchivePlugin *mCDNArchivePlugin;
    bool loadBuiltinPlugins();
    OgreSystem();
    bool initialize(Provider<ProxyCreationListener*>*proxyManager,
                    const String&options);
    bool renderOneFrame(Task::LocalTime, Duration frameTime);
    ///all the things that should happen just before the frame
    void preFrame(Task::LocalTime, Duration);
    ///all the things that should happen once the frame finishes
    void postFrame(Task::LocalTime, Duration);
    void destroyRenderTarget(Ogre::ResourcePtr &name);

    // Initiate quiting by indicating to the main loop that we want to shut down
    void quit();

    bool mQuitRequested;

    Ogre::RenderTarget* createRenderTarget(const String &name, uint32 width, uint32 height, bool automipmap, Ogre::PixelFormat pf);
    Vector3d mFloatingPointOffset;
    Ogre::RaySceneQuery* mRayQuery;
    CubeMap *mCubeMap;
    Entity* internalRayTrace(const Vector3d &position,
                     const Vector3f &direction,
                     bool aabbOnly,
                     int&resultCount,
                     double &returnResult,
                     Vector3f &returnNormal,
                     Ogre::SubEntity*& returnSubMesh,
                     float *returnTexU, float *returnTexV,
                     int which=0) const;
public:
    bool forwardMessagesTo(MessageService*){return false;}
    bool endForwardingMessagesTo(MessageService*){return false;}
    /**
     * Process a message that may be meant for this system
     */
    void processMessage(const RoutableMessageHeader&,
						MemoryReference message_body){
		NOT_IMPLEMENTED(ogregraphics);
	}

    OptionValue *mParallaxSteps;
    OptionValue *mParallaxShadowSteps;
    static std::list<OgreSystem*> sActiveOgreScenes;
    static uint32 sNumOgreSystems;
    std::list<CameraEntity*> mAttachedCameras;
    CameraEntity *mPrimaryCamera;

    ///adds the camera to the list of attached cameras, making it the primary camera if it is first to be added
    std::list<CameraEntity*>::iterator attachCamera(const String&renderTargetName,CameraEntity*);
    ///removes the camera from the list of attached cameras.
    std::list<CameraEntity*>::iterator detachCamera(std::list<CameraEntity*>::iterator);
    CameraEntity*getPrimaryCamera() {
        return mPrimaryCamera;
    }
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
    static TimeSteppedQueryableSimulation* create(Provider<ProxyCreationListener*>*proxyManager,
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
    bool queryRay(const Vector3d&position,
                  const Vector3f&direction,
                  const double maxDistance,
                  ProxyMeshObjectPtr ignore,
                  double &returnDistance,
                  Vector3f &returnNormal,
                  SpaceObjectReference &returnName);

    Entity* rayTrace(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     Vector3f&returnNormal,
                     Ogre::SubEntity*&subent,
                     int which=0) const;
    Entity* rayTraceAABB(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     Ogre::SubEntity*&subent,
                     int which=0) const;
    virtual Duration desiredTickRate()const;
    ///returns if rendering should continue
    virtual bool tick();
    Ogre::RenderTarget *getRenderTarget();
    static Ogre::Root *getRoot();
    Ogre::SceneManager* getSceneManager();
    virtual void onCreateProxy(ProxyObjectPtr p); // MCB: interface from ProxyCreationListener
    virtual void onDestroyProxy(ProxyObjectPtr p); // MCB: interface from ProxyCreationListener
    ~OgreSystem();
};


} }
#endif
