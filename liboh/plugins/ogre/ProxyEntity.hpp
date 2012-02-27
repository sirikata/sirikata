/*  Sirikata Graphical Object Host
 *  Entity.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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

#ifndef SIRIKATA_OGRE_PROXY_ENTITY_HPP__
#define SIRIKATA_OGRE_PROXY_ENTITY_HPP__

#include <sirikata/ogre/Entity.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/core/network/IOTimer.hpp>

namespace Sirikata {
namespace Graphics {

class OgreSystem;
class ProxyEntity;

class ProxyEntityListener {
public:
    virtual ~ProxyEntityListener();
    virtual void proxyEntityDestroyed(ProxyEntity*) {};
};

/** Ogre entities using ProxyObjects for their information. */
class ProxyEntity
    : public Sirikata::Graphics::Entity,
      public PositionListener,
      public ProxyObjectListener,
      public MeshListener,
      public Provider<ProxyEntityListener*>
{
protected:
    ProxyObjectPtr mProxy;
    bool mActive; // Whether we've added ourselves to the download planner
    Network::IOTimerPtr mDestroyTimer;
    // Indicates if the parent cares about this ProxyEntity anymore,
    // i.e. if it can self destruct if it determines it's no longer
    // needed for rendering.
    bool mCanDestroy;

    void handleDestroyTimeout();

    // Invoked when some condition switches to making it possible that
    // deletion could occur. If all conditions are satisfied, this
    // causes self destruction. NOTE: This *must not* be followed by
    // any other operations since it may result in the destruction of
    // this object.
    void tryDelete();
public:
    /** NOTE that you *MUST* call initializeToProxy with the same ProxyObjectPtr
     * immediately after. We can't completely split the construction and
     * initialization right now because Entity expects a name at construction,
     * but to initialize we may need some virtual functions.
     */
    ProxyEntity(OgreRenderer *scene, const ProxyObjectPtr &ppo);
    virtual ~ProxyEntity();

    void initializeToProxy(const ProxyObjectPtr &ppo);

    // Entity Overrides
    virtual BoundingSphere3f bounds();
    virtual void tick(const Time& t, const Duration& deltaTime);
    virtual bool isDynamic() const;
    virtual bool isMobile() const;

    ProxyObject &getProxy() const {
        return *mProxy;
    }
    const ProxyObjectPtr &getProxyPtr() const {
        return mProxy;
    }


    static ProxyEntity *fromMovableObject(Ogre::MovableObject *obj);

    // PositionListener
    virtual void updateLocation(
        ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation,
        const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,
        const SpaceObjectReference& sporef);

    // ProxyObjectListener
    virtual void validated(ProxyObjectPtr proxy);
    virtual void invalidated(ProxyObjectPtr proxy, bool permanent);
    virtual void destroyed(ProxyObjectPtr proxy);

    // interface from MeshListener
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);
    virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef);

    void extrapolateLocation(TemporalValue<Location>::Time current);

private:

    void iUpdateLocation(
        ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation,
        const TimedMotionQuaternion& newOrient,
        const BoundingSphere3f& newBounds,
        const SpaceObjectReference& sporef, Liveness::Token lt);

    // ProxyObjectListener
    void iValidated(ProxyObjectPtr proxy,Liveness::Token lt);
    void iInvalidated(ProxyObjectPtr proxy, bool permanent,Liveness::Token lt);
    void iDestroyed(ProxyObjectPtr proxy,Liveness::Token lt);

    // interface from MeshListener
    void iOnSetMesh (
        ProxyObjectPtr proxy, Transfer::URI const& newMesh,
        const SpaceObjectReference& sporef,Liveness::Token lt);
    
    void iOnSetScale (
        ProxyObjectPtr proxy, float32 newScale,
        const SpaceObjectReference& sporef,Liveness::Token lt);

    void iHandleDestroyTimeout(Liveness::Token lt);
    
};
typedef std::tr1::shared_ptr<Entity> EntityPtr;

}
}

#endif //SIRIKATA_OGRE_PROXY_ENTITY_HPP__
