/*  Sirikata Object Host
 *  ProxyObject.cpp
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

#include <sirikata/proxyobject/Platform.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/core/util/Extrapolation.hpp>
#include <sirikata/proxyobject/PositionListener.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>

#include <sirikata/proxyobject/MeshListener.hpp>

// Helper for checking serialization of data access to a ProxyObject. These
// don't necessarily cover all conflicts or uses, they just cover many parts of
// ProxyObject where we don't use locks directly (or data would escape mutex
// because it is returned) so we know problems could arise if they aren't
// properly protected by callers.
#define PROXY_SERIALIZED() SerializationCheck::Scoped ___proxy_serialization_check(const_cast<ProxyObject*>(this))

namespace Sirikata {

ProxyObjectPtr ProxyObject::construct(ProxyManagerPtr man, const SpaceObjectReference& id) {
    ProxyObjectPtr res(SelfWeakPtr<ProxyObject>::internalConstruct(new ProxyObject(man, id)));
    res->validate();
    return res;
}

ProxyObject::ProxyObject(ProxyManagerPtr man, const SpaceObjectReference& id)
 :   SelfWeakPtr<ProxyObject>(),
     ProxyObjectProvider(),
     MeshProvider (),
     mID(id),
     mParent(man),
     mValid(true)
{  
    assert(mParent);

    reset();        
    // Validate is forced in ProxyObject::construct
}


ProxyObject::~ProxyObject() {
    mParent->proxyDeleted(mID.object());
}

void ProxyObject::reset() {
    SequencedPresenceProperties::reset();    
}

void ProxyObject::validate() {
  mValid = true;
  ProxyObjectPtr ptr = getSharedPtr();
  assert(ptr);
  ProxyObjectProvider::notify(&ProxyObjectListener::validated, ptr);
}

void ProxyObject::invalidate(bool permanent) {
    mValid = false;
    ProxyObjectPtr ptr = getSharedPtr();
    assert(ptr);
    ProxyObjectProvider::notify(&ProxyObjectListener::invalidated, ptr, permanent);
}

void ProxyObject::destroy() {
    ProxyObjectPtr ptr = getSharedPtr();
    assert(ptr);
    ProxyObjectProvider::notify(&ProxyObjectListener::destroyed, ptr);
}

bool ProxyObject::UpdateNeeded::operator() (
    const Location&updatedValue,
    const Location&predictedValue) const {
    Vector3f ux,uy,uz,px,py,pz;
    updatedValue.getOrientation().toAxes(ux,uy,uz);
    predictedValue.getOrientation().toAxes(px,py,pz);
    return (updatedValue.getPosition()-predictedValue.getPosition()).lengthSquared()>1.0 ||
           ux.dot(px)<.9||uy.dot(py)<.9||uz.dot(pz)<.9;
}

bool ProxyObject::isStatic() const {
    PROXY_SERIALIZED();
    return mLoc.velocity() == Vector3f::zero() && mOrientation.velocity() == Quaternion::identity();
}


TimedMotionVector3f ProxyObject::location() const{
    PROXY_SERIALIZED();
    if (!isPresence())
        return SequencedPresenceProperties::location();
    SequencedPresencePropertiesPtr req = mParent->parent()->presenceRequestedLocation(getObjectReference());
    uint64 latest_epoch = mParent->parent()->presenceLatestEpoch(getObjectReference());
    if (!req || latest_epoch >= req->getUpdateSeqNo(LOC_POS_PART))
        return SequencedPresenceProperties::location();
    return req->location();
}

TimedMotionQuaternion ProxyObject::orientation() const {
    PROXY_SERIALIZED();
    if (!isPresence())
        return SequencedPresenceProperties::orientation();
    SequencedPresencePropertiesPtr req = mParent->parent()->presenceRequestedLocation(getObjectReference());
    uint64 latest_epoch = mParent->parent()->presenceLatestEpoch(getObjectReference());
    if (!req || latest_epoch >= req->getUpdateSeqNo(LOC_ORIENT_PART))
        return SequencedPresenceProperties::orientation();
    return req->orientation();
}

BoundingSphere3f ProxyObject::bounds() const {
    PROXY_SERIALIZED();
    if (!isPresence())
        return SequencedPresenceProperties::bounds();
    SequencedPresencePropertiesPtr req = mParent->parent()->presenceRequestedLocation(getObjectReference());
    uint64 latest_epoch = mParent->parent()->presenceLatestEpoch(getObjectReference());
    if (!req || latest_epoch >= req->getUpdateSeqNo(LOC_BOUNDS_PART))
        return SequencedPresenceProperties::bounds();
    return req->bounds();
}

Transfer::URI ProxyObject::mesh() const {
    PROXY_SERIALIZED();
    if (!isPresence())
        return SequencedPresenceProperties::mesh();
    SequencedPresencePropertiesPtr req = mParent->parent()->presenceRequestedLocation(getObjectReference());
    uint64 latest_epoch = mParent->parent()->presenceLatestEpoch(getObjectReference());
    if (!req || latest_epoch >= req->getUpdateSeqNo(LOC_MESH_PART))
        return SequencedPresenceProperties::mesh();
    return req->mesh();
}

String ProxyObject::physics() const {
    PROXY_SERIALIZED();
    if (!isPresence())
        return SequencedPresenceProperties::physics();
    SequencedPresencePropertiesPtr req = mParent->parent()->presenceRequestedLocation(getObjectReference());
    uint64 latest_epoch = mParent->parent()->presenceLatestEpoch(getObjectReference());
    if (!req || latest_epoch >= req->getUpdateSeqNo(LOC_PHYSICS_PART))
        return SequencedPresenceProperties::physics();
    return req->physics();
}


TimedMotionVector3f ProxyObject::verifiedLocation() const {
    PROXY_SERIALIZED();
    return SequencedPresenceProperties::location();
}

TimedMotionQuaternion ProxyObject::verifiedOrientation() const {
    PROXY_SERIALIZED();
    return SequencedPresenceProperties::orientation();
}

BoundingSphere3f ProxyObject::verifiedBounds() const {
    PROXY_SERIALIZED();
    return SequencedPresenceProperties::bounds();
}

Transfer::URI ProxyObject::verifiedMesh() const {
    PROXY_SERIALIZED();
    return SequencedPresenceProperties::mesh();
}

String ProxyObject::verifiedPhysics() const {
    PROXY_SERIALIZED();
    return SequencedPresenceProperties::physics();
}



void ProxyObject::setLocation(const TimedMotionVector3f& reqloc, uint64 seqno) {
    PROXY_SERIALIZED();
    if (SequencedPresenceProperties::setLocation(reqloc, seqno)) {
        ProxyObjectPtr ptr = getSharedPtr();
        assert(ptr);
        PositionProvider::notify(&PositionListener::updateLocation, ptr, mLoc, mOrientation, mBounds, mID);
    }
}

void ProxyObject::setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno) {
    PROXY_SERIALIZED();
    if (SequencedPresenceProperties::setOrientation(reqorient, seqno)) {
        ProxyObjectPtr ptr = getSharedPtr();
        assert(ptr);
        PositionProvider::notify(&PositionListener::updateLocation, ptr, mLoc, mOrientation, mBounds, mID);
    }
}

void ProxyObject::setBounds(const BoundingSphere3f& bnds, uint64 seqno) {
    PROXY_SERIALIZED();
    if (SequencedPresenceProperties::setBounds(bnds, seqno)) {
        ProxyObjectPtr ptr = getSharedPtr();
        assert(ptr);
        PositionProvider::notify(&PositionListener::updateLocation, ptr, mLoc, mOrientation, mBounds, mID);
        MeshProvider::notify (&MeshListener::onSetScale, ptr, mBounds.radius(), mID);
    }
}

//you can set a camera's mesh as of now.
void ProxyObject::setMesh (Transfer::URI const& mesh, uint64 seqno) {
    PROXY_SERIALIZED();
    if (SequencedPresenceProperties::setMesh(mesh, seqno)) {
        ProxyObjectPtr ptr = getSharedPtr();
        assert(ptr);
        if (ptr) MeshProvider::notify ( &MeshListener::onSetMesh, ptr, mesh, mID);
    }
}

void ProxyObject::setPhysics (const String& rhs, uint64 seqno) {
    PROXY_SERIALIZED();
    if (SequencedPresenceProperties::setPhysics(rhs, seqno)) {
        ProxyObjectPtr ptr = getSharedPtr();
        assert(ptr);
        if (ptr) MeshProvider::notify ( &MeshListener::onSetPhysics, ptr, rhs, mID);
    }
}

}
