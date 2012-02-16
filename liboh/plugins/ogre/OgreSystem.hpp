/*  Sirikata libproxyobject -- Ogre Graphics Plugin
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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Time.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include <sirikata/ogre/OgreHeaders.hpp>
#include <OgreResourceManager.h>
#include <OgrePixelFormat.h>

#include <sirikata/ogre/OgreRenderer.hpp>

#include <sirikata/oh/ConnectionEventListener.hpp>
#include <sirikata/oh/HostedObject.hpp>

namespace Ogre {
struct RaySceneQueryResultEntry;
class SubEntity;
}

namespace Sirikata {
class ProxyObject;
/** Namespace for the OGRE Graphics Plugin: see class OgreSystem. */
namespace Graphics {
class ProxyEntity;
class CubeMap;
struct IntersectResult;
class OgreSystemMouseHandler;

/** Represents one OGRE SceneManager, a single environment. */
class OgreSystem :
        public OgreRenderer,
        protected SessionEventListener,
        public ProxyCreationListener,
        public ConnectionEventListener
{
    ConnectionEventProvider* mConnectionEventProvider;
    VWObjectPtr mViewer;
    SpaceObjectReference mPresenceID;


    friend class OgreSystemMouseHandler;
    OgreSystemMouseHandler *mMouseHandler;
    void allocMouseHandler();
    void destroyMouseHandler();
    void tickInputHandler(const Task::LocalTime& t) const;


    OgreSystem(Context* ctx,Network::IOStrandPtr sStrand);
    bool initialize(ConnectionEventProvider* cevtprovider, VWObjectPtr viewer, const SpaceObjectReference& presenceid, const String&options);



    Ogre::RaySceneQuery* mRayQuery;
    CubeMap *mCubeMap;

    Invokable* mOnReadyCallback;
    Invokable* mOnResetReadyCallback;

    void handleUIReady();
    void iHandleUIReady(Liveness::Token osAlive);
    void handleUIResetReady();
    void handleUpdateUIViewport(int32 left, int32 top, int32 right, int32 bottom);

    ProxyEntity* internalRayTrace(const Ogre::Ray &traceFrom,
        bool aabbOnly,
        int&resultCount,
        double &returnResult,
        Vector3f &returnNormal,
        int& returnSubMesh,
        IntersectResult *returnIntersectResult, bool texcoord,
        int which=0,
        SpaceObjectReference ignore = SpaceObjectReference::null()) const;

    Mesh::VisualPtr mDefaultMesh;
public:

    Camera *mPrimaryCamera;
    Camera* mOverlayCamera;

    // Overrides for Service
    virtual void stop();

    // For classes that only have access to OgreSystem and not a Context
    Time simTime();

    VWObjectPtr getViewer() const { return mViewer; }
    SpaceObjectReference getViewerPresence() const { return mPresenceID; }

    String getResourcesDir() const { return mResourcesDir; }

    Camera*getPrimaryCamera() {
        return mPrimaryCamera;
    }

    static TimeSteppedSimulation* create(
        Context* ctx,
        ConnectionEventProvider* cevtprovider,
        HostedObjectPtr obj,
        const SpaceObjectReference& presenceid,
        const String& options,
        Network::IOStrandPtr sStrand
    )
    {
        OgreSystem*os= new OgreSystem(ctx,sStrand);
        if (os->initialize(cevtprovider, obj, presenceid, options))
            return os;
        delete os;
        return NULL;
    }

    ProxyEntity* getEntity(const SpaceObjectReference &proxyId) const;
    ProxyEntity* getEntity(const ProxyObjectPtr &proxy) const;
    // Invoked when a ProxyEntity is self destructing
    void entityDestroyed(ProxyEntity*);

    bool queryRay(const Vector3d&position,
                  const Vector3f&direction,
                  const double maxDistance,
                  ProxyObjectPtr ignore,
                  double &returnDistance,
                  Vector3f &returnNormal,
                  SpaceObjectReference &returnName);

    Entity* rayTrace(const Vector3d &position,
                     const Vector3f &direction,
                     int&resultCount,
                     double &returnResult,
                     Vector3f&returnNormal,
                     int&subent,
        int which=0,
        SpaceObjectReference ignore = SpaceObjectReference::null()) const;


    virtual void windowResized(Ogre::RenderWindow *rw);
    // Converts from the full window coordinates into coordinates for
    // the actual display viewport (i.e. excluding the overlay
    // regions). Note that these might be negative or greater than the
    // width of the viewport, in which case the return value will be
    // false.
    bool translateToDisplayViewport(float32 x, float32 y, float32* ox, float32* oy);

    virtual void poll();

    virtual bool renderOneFrame(Task::LocalTime t, Duration frameTime);
    virtual void preFrame(Task::LocalTime currentTime, Duration frameTime);
    virtual void postFrame(Task::LocalTime current, Duration frameTime);


    // ProxyCreationListener
    virtual void onCreateProxy(ProxyObjectPtr p);
    virtual void onDestroyProxy(ProxyObjectPtr p);



    virtual void attachCamera(const String&renderTargetName,Camera*);
    virtual void detachCamera(Camera*);


    virtual Mesh::VisualPtr defaultMesh() const { return mDefaultMesh; }

    // *******

    // ConnectionEventListener Interface
    virtual void onConnected(const Network::Address& addr);
    virtual void onDisconnected(const Network::Address& addr, bool requested, const String& reason);

    // SessionEventListener Interface
    virtual void onDisconnected(SessionEventProviderPtr from, const SpaceObjectReference& name);

    // Methods for handling Invokable actions
    virtual boost::any invoke(std::vector<boost::any>& params);

    // Sets the onReady callback, invoked when the basic graphics and UI are
    // ready to be used.
    boost::any setOnReady(std::vector<boost::any>& params);
    // Hide the loading screen, if it was displayed.
    boost::any evalInUI(std::vector<boost::any>& params);
    // Helper which creates a WebView window, either
    boost::any createWindow(const String& name, bool is_html, bool is_file, String content, uint32 width, uint32 height);
    // Create a window using a URL
    boost::any createWindow(std::vector<boost::any>& params);
    // Create a window using a file
    boost::any createWindowFile(std::vector<boost::any>& params);
    // Create a window using HTML
    boost::any createWindowHTML(std::vector<boost::any>& params);
    // Dynamically load a javscript UI module
    boost::any addModuleToUI(std::vector<boost::any>& params);
    boost::any addTextModuleToUI(std::vector<boost::any>& params);

    // Add an input handler function which will be invoked for input
    // events, e.g. mouse and keyboard
    boost::any addInputHandler(std::vector<boost::any>& params);
    boost::any removeInputHandler(std::vector<boost::any>& params);

    boost::any pick(std::vector<boost::any>& params);
    boost::any bbox(std::vector<boost::any>& params);
    boost::any visible(std::vector<boost::any>& params);
    boost::any axis(std::vector<boost::any>& params);
    boost::any world2Screen(std::vector<boost::any>& params);

    boost::any newDrawing(std::vector<boost::any>& params);
    boost::any setMat(std::vector<boost::any>& params);
    boost::any setInheritOrient(std::vector<boost::any>& params);
    boost::any setInheritScale(std::vector<boost::any>& params);
    boost::any setVisible(std::vector<boost::any>& params);
    boost::any shape(std::vector<boost::any>& params);

    boost::any getCamera(std::vector<boost::any>& params);
    boost::any setCameraPosition(std::vector<boost::any>& params);
    boost::any setCameraOrientation(std::vector<boost::any>& params);
    boost::any getAnimationList(std::vector<boost::any>& params);

    boost::any startAnimation(std::vector<boost::any>& params);
    boost::any stopAnimation(std::vector<boost::any>& params);
    boost::any isReady(std::vector<boost::any>& params);

    ~OgreSystem();

private:
    void instantiateAllObjects(ProxyManagerPtr pop);
    double clamp(const double& val);

    void iOnSessionDisconnected(
        Liveness::Token osAlive, SessionEventProviderPtr from,
        const SpaceObjectReference& name);
    void iOnNetworkDisconnected(
        Liveness::Token osAlive,const Network::Address& addr,
        bool requested, const String& reason);

    void iOnCreateProxy(
        Liveness::Token osAlive, ProxyObjectPtr p, bool inInit);
    void iOnDestroyProxy(
        Liveness::Token osAlive,ProxyObjectPtr p);
    
    
    typedef std::tr1::unordered_map<SpaceObjectReference, ProxyEntity*, SpaceObjectReference::Hasher> EntityMap;
    EntityMap mEntityMap;
    String currentMat;
    bool mReady;
    bool initialized;
};


} }
#endif
