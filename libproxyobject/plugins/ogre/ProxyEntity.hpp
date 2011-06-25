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
    Network::IOTimerPtr mDestroyTimer;

    void handleDestroyTimeout();

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
    virtual float32 priority();
    virtual void tick(const Time& t, const Duration& deltaTime);
    virtual bool isDynamic() const;

    ProxyObject &getProxy() const {
        return *mProxy;
    }
    const ProxyObjectPtr &getProxyPtr() const {
        return mProxy;
    }


    static ProxyEntity *fromMovableObject(Ogre::MovableObject *obj);

    // PositionListener
    virtual void updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef);

    // ProxyObjectListener
    virtual void validated();
    virtual void invalidated();
    virtual void destroyed();

    // interface from MeshListener
    virtual void onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& newMesh,const SpaceObjectReference& sporef);
    virtual void onSetScale (ProxyObjectPtr proxy, float32 newScale ,const SpaceObjectReference& sporef);


    void extrapolateLocation(TemporalValue<Location>::Time current);
};
typedef std::tr1::shared_ptr<Entity> EntityPtr;

}
}

#endif //SIRIKATA_OGRE_PROXY_ENTITY_HPP__
