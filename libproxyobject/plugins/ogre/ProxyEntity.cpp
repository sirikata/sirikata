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

using namespace Sirikata::Transfer;

namespace Sirikata {
namespace Graphics {

ProxyEntityListener::~ProxyEntityListener() {
}

ProxyEntity::ProxyEntity(OgreRenderer *scene, const ProxyObjectPtr &ppo)
 : Entity(scene, ppo->getObjectReference().toString()),
   mProxy()
{
    mDestroyTimer = Network::IOTimer::create(
        mScene->context()->ioService,
        std::tr1::bind(&ProxyEntity::handleDestroyTimeout, this)
    );
}

ProxyEntity::~ProxyEntity() {
    getProxy().MeshProvider::removeListener(this);

    getProxy().ProxyObjectProvider::removeListener(this);
    getProxy().PositionProvider::removeListener(this);
}

void ProxyEntity::initializeToProxy(const ProxyObjectPtr &ppo) {
    assert( ppo );
    assert( !mProxy || (mProxy->getObjectReference() == ppo->getObjectReference()) );

    if (mProxy) {
        mProxy->ProxyObjectProvider::removeListener(this);
        mProxy->PositionProvider::removeListener(this);
        mProxy->MeshProvider::removeListener(this);
    }
    mProxy = ppo;
    mProxy->ProxyObjectProvider::addListener(this);
    mProxy->PositionProvider::addListener(this);
    mProxy->MeshProvider::addListener(this);
}

BoundingSphere3f ProxyEntity::bounds() {
    return getProxy().getBounds();
}

float32 ProxyEntity::priority() {
    return mProxy->priority;
}

void ProxyEntity::tick(const Time& t, const Duration& deltaTime) {
    // Update location from proxy as well as doing normal updates
    Entity::tick(t, deltaTime);
    extrapolateLocation(t);
}

bool ProxyEntity::isDynamic() const {
    return Entity::isDynamic() || !getProxy().isStatic();
}

ProxyEntity *ProxyEntity::fromMovableObject(Ogre::MovableObject *movable) {
    return static_cast<ProxyEntity*>( Entity::fromMovableObject(movable) );
}

void ProxyEntity::updateLocation(const TimedMotionVector3f &newLocation, const TimedMotionQuaternion& newOrient, const BoundingSphere3f& newBounds,const SpaceObjectReference& sporef) {
    SILOG(ogre,detailed,"UpdateLocation "<<this<<" to "<<newLocation.position()<<"; "<<newOrient.position());
    setOgrePosition(Vector3d(newLocation.position()));
    setOgreOrientation(newOrient.position());
    updateScale( newBounds.radius() );
    checkDynamic();
}

void ProxyEntity::validated() {
    mDestroyTimer->cancel();

    // Because this could be a new ProxyEntity, created after a bunch
    // of updates have been received by the ProxyObject, we need to
    // refresh its important data
    updateLocation( mProxy->getTimedMotionVector(), mProxy->getTimedMotionQuaternion(), mProxy->getBounds(),SpaceObjectReference::null() );

    // And the final step is to update the mesh, kicking off the
    // download process.
    processMesh( mProxy->getMesh() );
}

void ProxyEntity::invalidated() {
    // To mask very quick removal/addition sequences, defer unloading
    mDestroyTimer->wait(Duration::seconds(1));
}

void ProxyEntity::handleDestroyTimeout() {
    unloadMesh();
}

void ProxyEntity::destroyed() {
    // FIXME this is triggered by the ProxyObjectListener interface. But we
    //actually don't want to delete it here, we want to *maybe* mask things for
    //awhile. For now, we just leak this, but clearly this needs to be fixed.
    //Provider<ProxyEntityListener*>::notify(&ProxyEntityListener::proxyEntityDestroyed, this);
    //delete this;
}

void ProxyEntity::extrapolateLocation(TemporalValue<Location>::Time current) {
    Location loc (getProxy().extrapolateLocation(current));
    setOgrePosition(loc.getPosition());
    setOgreOrientation(loc.getOrientation());
}

/////////////////////////////////////////////////////////////////////
// overrides from MeshListener
// MCB: integrate these with the MeshObject model class

void ProxyEntity::onSetMesh (ProxyObjectPtr proxy, Transfer::URI const& meshFile,const SpaceObjectReference& sporef )
{

}

void ProxyEntity::onSetScale (ProxyObjectPtr proxy, float32 scale,const SpaceObjectReference& sporef )
{
    updateScale(scale);
}


}
}
