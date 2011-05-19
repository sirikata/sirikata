/*  Sirikata
 *  OgreRenderer.hpp
 *
 *  Copyright (c) 2011, Stanford University
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

#ifndef _SIRIKATA_OGRE_OGRE_RENDERER_HPP_
#define _SIRIKATA_OGRE_OGRE_RENDERER_HPP_

#include <sirikata/ogre/Platform.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include "OgreResource.h"
#include <sirikata/mesh/Meshdata.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/transfer/TransferMediator.hpp>
#include <sirikata/proxyobject/TimeSteppedQueryableSimulation.hpp>
#include <OgreWindowEventUtilities.h>

namespace Sirikata {

namespace Input {
class SDLInputManager;
}

namespace Mesh {
class Filter;
}
class ModelsSystem;

namespace Graphics {

class Camera;
class Entity;

class CDNArchivePlugin;

using Input::SDLInputManager;

/** Represents a SQLite database connection. */
class SIRIKATA_OGRE_EXPORT OgreRenderer : public TimeSteppedQueryableSimulation, public Ogre::WindowEventListener {
public:
    OgreRenderer(Context* ctx);
    virtual ~OgreRenderer();

    virtual bool initialize(const String& options);

    Context* context() const { return mContext; }

    void toggleSuspend();
    void suspend();
    void resume();

    // Initiate quiting by indicating to the main loop that we want to shut down
    void quit();

    // Event injection for SDL created windows.
    void injectWindowResized(uint32 w, uint32 h);

    const Vector3d& getOffset() const { return mFloatingPointOffset; }

    virtual Transfer::TransferPoolPtr transferPool();

    virtual Ogre::RenderTarget* createRenderTarget(const String &name, uint32 width, uint32 height, bool automipmap, int pixelFmt);
    virtual Ogre::RenderTarget* createRenderTarget(String name,uint32 width=0, uint32 height=0);
    virtual void destroyRenderTarget(Ogre::ResourcePtr& name);
    virtual void destroyRenderTarget(const String &name);

    Ogre::SceneManager* getSceneManager() {
        return mSceneManager;
    }

    Ogre::RenderTarget* getRenderTarget() {
        return mRenderTarget;
    }

    SDLInputManager *getInputManager() {
        return mInputManager;
    }

    // TimeSteppedQueryableSimulation Interface
    virtual bool queryRay(const Vector3d&position,
        const Vector3f&direction,
        const double maxDistance,
        ProxyObjectPtr ignore,
        double &returnDistance,
        Vector3f &returnNormal,
        SpaceObjectReference &returnName);

    // ProxyCreationListener Interface
    // FIXME we don't really want these here, but we are forced to have them by TimeSteppedQueryableSimulation
    virtual void onCreateProxy(ProxyObjectPtr p) {}
    virtual void onDestroyProxy(ProxyObjectPtr p) {}

    // TimeSteppedSimulation Interface
    virtual Duration desiredTickRate() const;
    virtual void poll();
    virtual void stop();

    // Invokable Interface
    virtual boost::any invoke(std::vector<boost::any>& params);


    // Ogre::WindowEventListener Interface overloads
    virtual void windowResized(Ogre::RenderWindow *rw) {}

    // Options values
    virtual float32 nearPlane();
    virtual float32 farPlane();
    virtual float32 parallaxSteps();
    virtual int32 parallaxShadowSteps();

    ///adds the camera to the list of attached cameras, making it the primary camera if it is first to be added
    virtual void attachCamera(const String& renderTargetName, Camera*);
    ///removes the camera from the list of attached cameras.
    virtual void detachCamera(Camera*);

    typedef std::tr1::function<void(Mesh::MeshdataPtr)> ParseMeshCallback;
    /** Tries to parse a mesh. Can handle different types of meshes and tries to
     *  find the right parser using magic numbers.  If it is unable to find the
     *  right parser, returns NULL.  Otherwise, returns the parsed mesh as a
     *  Meshdata object.
     *  \param orig_uri original URI, used to construct correct relative paths
     *                  for dependent resources
     *  \param fp the fingerprint of the data, used for unique naming and passed
     *            through to the resulting mesh data
     *  \param data the contents of the
     *  \param cb callback to invoke when parsing is complete
     */
    void parseMesh(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb);

  protected:
    static Ogre::Root *getRoot();

    bool loadBuiltinPlugins();
    // Loads system lights if they are being used.
    void loadSystemLights();
    // Helper for loadSystemLights.
    void constructSystemLight(const String& name, const Vector3f& direction, float brightness);

    bool useModelLights() const;

    virtual bool renderOneFrame(Task::LocalTime, Duration frameTime);
    ///all the things that should happen just before the frame
    virtual void preFrame(Task::LocalTime, Duration);
    ///all the things that should happen once the frame finishes
    virtual void postFrame(Task::LocalTime, Duration);


    void parseMeshWork(const Transfer::URI& orig_uri, const Transfer::Fingerprint& fp, Transfer::DenseDataPtr data, ParseMeshCallback cb);


    static Ogre::Root* sRoot;
    static Ogre::Plugin* sCDNArchivePlugin;
    static CDNArchivePlugin* mCDNArchivePlugin;
    static std::list<OgreRenderer*> sActiveOgreScenes;
    static uint32 sNumOgreSystems;
    static Ogre::RenderTarget *sRenderTarget; // FIXME why a static render target?

    Context* mContext;

    bool mQuitRequested;
    bool mQuitRequestHandled;

    bool mSuspended;

    // FIXME because we don't have proper multithreaded support in cppoh, we
    // need to allocate our own thread dedicated to parsing
    Network::IOService* mParsingIOService;
    Network::IOWork* mParsingWork;
    Thread* mParsingThread;

    SDLInputManager *mInputManager;
    Ogre::SceneManager *mSceneManager;
    bool mOgreOwnedRenderWindow;

    Ogre::RenderTarget *mRenderTarget;
    Ogre::RenderWindow *mRenderWindow; // Should be the same as mRenderTarget,
                                       // but we need the RenderWindow form to
                                       // deal with window events.


    OptionValue*mWindowWidth;
    OptionValue*mWindowHeight;
    OptionValue*mWindowDepth;
    OptionValue*mFullScreen;
    OptionValue* mOgreRootDir;
    ///How many seconds we aim to spend in each frame
    OptionValue*mFrameDuration;
    OptionValue *mParallaxSteps;
    OptionValue *mParallaxShadowSteps;
    OptionValue* mModelLights; // Use model or basic lights
    OptionSet* mOptions;

    Vector4f mBackgroundColor;
    Vector3d mFloatingPointOffset;

    Task::LocalTime mLastFrameTime;

    String mResourcesDir;

    // FIXME need to support multiple parsers, see #124
    ModelsSystem* mModelParser;
    Mesh::Filter* mModelFilter;

    Transfer::TransferPoolPtr mTransferPool;


    typedef std::tr1::unordered_map<String,Entity*> SceneEntitiesMap;
    SceneEntitiesMap mSceneEntities;
    std::list<Entity*> mMovingEntities; // FIXME used to call extrapolate
                                        // location. register by Entity, but
                                        // only ProxyEntities use it

    std::tr1::unordered_set<Camera*> mAttachedCameras;

    friend class Entity; //Entity will insert/delete itself from these arrays.
    friend class Camera; //CameraEntity will insert/delete itself from the scene cameras array.
};

} // namespace Graphics
} // namespace Sirikata

#endif //_SIRIKATA_OGRE_OGRE_RENDERER_HPP_
