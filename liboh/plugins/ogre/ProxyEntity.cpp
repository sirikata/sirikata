/*  Sirikata Graphical Object Host
 *  Entity.cpp
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

#include "ProxyEntity.hpp"
#include <sirikata/ogre/OgreRenderer.hpp>
#include <sirikata/ogre/ResourceDownloadPlanner.hpp>
#include "OgreSystem.hpp"

using namespace Sirikata::Transfer;

namespace Sirikata {
namespace Graphics {

ProxyEntityListener::~ProxyEntityListener() {
}

// mProxy is intentially not initialized to ppo. You need to call initializeToProxy().
ProxyEntity::ProxyEntity(OgreRenderer *scene, const ProxyObjectPtr &ppo)
 : Entity(scene, ppo->getObjectReference().toString()),
   mProxy(),
   mActive(false),
   mCanDestroy(false)
{
    mDestroyTimer = Network::IOTimer::create(
        mScene->context()->mainStrand,
        std::tr1::bind(&ProxyEntity::handleDestroyTimeout, this)
    );
}


ProxyEntity::~ProxyEntity()
{
    bool mobileVal =!getProxy().isStatic();
    removeFromScene(&mobileVal);
    
    SILOG(ogre, detailed, "Killing ProxyEntity " << mProxy->getObjectReference().toString());
    Liveness::letDie();

    /**
       FIXME: ADDING AND REMOVING LISTENERS COULD INVALIDATE ITERATORS
    */
    getProxy().MeshProvider::removeListener(this);

    getProxy().ProxyObjectProvider::removeListener(this);
    getProxy().PositionProvider::removeListener(this);
}

void ProxyEntity::initializeToProxy(const ProxyObjectPtr &ppo) {
    assert( ppo );
    assert( !mProxy || (mProxy->getObjectReference() == ppo->getObjectReference()) );

    /**
       FIXME: ADDING AND REMOVING LISTENERS COULD INVALIDATE ITERATORS
     */
    if (mProxy) {
        mProxy->ProxyObjectProvider::removeListener(this);
        mProxy->PositionProvider::removeListener(this);
        mProxy->MeshProvider::removeListener(this);
    }
    mProxy = ppo;
    mProxy->ProxyObjectProvider::addListener(this);
    mProxy->PositionProvider::addListener(this);
    mProxy->MeshProvider::addListener(this);
    checkDynamic();
}

BoundingSphere3f ProxyEntity::bounds() {
    return getProxy().bounds();
}

void ProxyEntity::tick(const Time& t, const Duration& deltaTime) {
    // Update location from proxy as well as doing normal updates
    Entity::tick(t, deltaTime);
    extrapolateLocation(t);
}

bool ProxyEntity::isDynamic() const {
    return Entity::isDynamic() || isMobile();
}

bool ProxyEntity::isMobile() const
{
    return !getProxy().isStatic();
}

ProxyEntity *ProxyEntity::fromMovableObject(Ogre::MovableObject *movable) {
    return static_cast<ProxyEntity*>( Entity::fromMovableObject(movable) );
}

void ProxyEntity::updateLocation(
    ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation,
    const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,
    const SpaceObjectReference& sporef)
{
    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iUpdateLocation,this,
            proxy,newLocation,newOrient,newBounds,sporef,livenessToken()),
        "ProxyEntity::iUpdateLocation");
}


void ProxyEntity::iUpdateLocation(
    ProxyObjectPtr proxy, const TimedMotionVector3f &newLocation,
    const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,
    const SpaceObjectReference& sporef, Liveness::Token lt)
{
    if (!lt)
        return;
    
    assert(proxy == mProxy);
    SILOG(ogre,detailed,"UpdateLocation "<<this<<" to "<<newLocation.position()<<"; "<<newOrient.position());

    setOgrePosition(Vector3d(newLocation.position()));
    setOgreOrientation(newOrient.position());
    updateScale( newBounds.radius() );
    checkDynamic();
}


void ProxyEntity::validated(ProxyObjectPtr ptr)
{
    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iValidated,this,
            ptr,livenessToken()),
        "ProxyEntity::iValidated");
}

void ProxyEntity::iValidated(ProxyObjectPtr ptr,Liveness::Token lt)
{
    if (!lt)
        return;
    
    assert(ptr == mProxy);

    SILOG(ogre, detailed, "Validating ProxyEntity " << ptr->getObjectReference().toString());

    mCanDestroy = false;

    mDestroyTimer->cancel();

    // Because this could be a new ProxyEntity, created after a bunch
    // of updates have been received by the ProxyObject, we need to
    // refresh its important data
    iUpdateLocation( mProxy, mProxy->location(), mProxy->orientation(), mProxy->bounds(), SpaceObjectReference::null(),lt );

    if (!mActive)
    {
        getScene()->downloadPlanner()->addNewObject(mProxy, this);
        mActive = true;
    }

}


void ProxyEntity::invalidated(ProxyObjectPtr ptr, bool permanent)
{
    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iInvalidated,this,
            ptr,permanent,livenessToken()),
        "ProxyEntity::iInvalidated");
}

void ProxyEntity::iInvalidated(ProxyObjectPtr ptr, bool permanent,Liveness::Token lt)
{
    if (!lt)
        return;
    
    assert(ptr == mProxy);

    SILOG(ogre, detailed, "Invalidating ProxyEntity " << ptr->getObjectReference().toString());

    if (!mActive) return;

    // If the the object really disconnected, it'll be marked as a permanent
    // removal. If it just left the result set then it should still be in the
    // world and shouldn't hurt to leave it around for awhile, and we'll get
    // less flickering if we try to mask an removal/addition pair due to quick
    // changes/data structure rearrangement by the space server.
    if (permanent)
        iHandleDestroyTimeout(lt);
    else
        mDestroyTimer->wait(Duration::seconds(15));
}

void ProxyEntity::handleDestroyTimeout()
{
    assert(mProxy);
    assert(mActive);

    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iHandleDestroyTimeout,
            this,livenessToken()),
            "ProxyEntity::iHandleDestroyTimeout");
}

void ProxyEntity::iHandleDestroyTimeout(Liveness::Token lt)
{
    if (!lt)
        return;
    
    SILOG(ogre, detailed, "Handling destruction timeout for ProxyEntity " << mProxy->getObjectReference().toString());

    assert(mProxy);
    assert(mActive);

    getScene()->downloadPlanner()->removeObject(mProxy);
    mActive = false;
    tryDelete();
}

void ProxyEntity::destroyed(ProxyObjectPtr ptr)
{
    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iDestroyed,
            this,ptr,livenessToken()),
            "ProxyEntity::iDestroyed");
}

void ProxyEntity::iDestroyed(ProxyObjectPtr ptr, Liveness::Token lt)
{
    if (!lt)
        return;
    
    assert(ptr == mProxy);

    // Not explicitly deleted here, we just mark it as a candidate for
    // destruction.
    mCanDestroy = true;
    tryDelete();
}

void ProxyEntity::extrapolateLocation(TemporalValue<Location>::Time current) {
    Location loc (getProxy().extrapolateLocation(current));
    setOgrePosition(loc.getPosition());
    setOgreOrientation(loc.getOrientation());
}

void ProxyEntity::onSetMesh (
    ProxyObjectPtr proxy, Transfer::URI const& meshFile,
    const SpaceObjectReference& sporef )
{
    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iOnSetMesh,this,
            proxy,meshFile,sporef,livenessToken()));
}


void ProxyEntity::iOnSetMesh (
    ProxyObjectPtr proxy, Transfer::URI const& meshFile,
    const SpaceObjectReference& sporef, Liveness::Token lt )
{
    if (! lt)
        return;
    assert(proxy == mProxy);
    getScene()->downloadPlanner()->updateObject(proxy);
}


void ProxyEntity::onSetScale (
    ProxyObjectPtr proxy, float32 scale,
    const SpaceObjectReference& sporef )
{
    mScene->renderStrand()->post(
        std::tr1::bind(&ProxyEntity::iOnSetScale,this,
            proxy,scale,sporef,livenessToken()));
}

void ProxyEntity::iOnSetScale (
    ProxyObjectPtr proxy, float32 scale,
    const SpaceObjectReference& sporef, Liveness::Token lt)
{
    if (!lt)
        return;
    
    assert(proxy == mProxy);
    updateScale(scale);
    getScene()->downloadPlanner()->updateObject(proxy);
}

void ProxyEntity::tryDelete() {
    if (!mActive && mCanDestroy) {
        static_cast<OgreSystem*>(getScene())->entityDestroyed(this);
        Provider<ProxyEntityListener*>::notify(&ProxyEntityListener::proxyEntityDestroyed, this);
        delete this;
    }
}

}
}
