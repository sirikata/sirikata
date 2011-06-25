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


namespace Sirikata {

ProxyObject::ProxyObject(ProxyManager *man, const SpaceObjectReference&id, VWObjectPtr vwobj, const SpaceObjectReference& owner_sor)
 :   SelfWeakPtr<ProxyObject>(),
     ProxyObjectProvider(),
     MeshProvider (),
     mID(id),
     mManager(man),
     mLoc(Time::null(), MotionVector3f(Vector3f::nil(), Vector3f::nil())),
     mOrientation(Time::null(), MotionQuaternion(Quaternion::identity(), Quaternion::identity())),
     mParent(vwobj),
     mMeshURI()
{
    assert(mParent);
    mDefaultPort = mParent->bindODPPort(owner_sor);

    reset();

    validate();
}


ProxyObject::~ProxyObject() {
    delete mDefaultPort;
}

void ProxyObject::reset() {
    memset(mUpdateSeqno, 0, LOC_NUM_PART * sizeof(uint64));
}

void ProxyObject::validate() {
    ProxyObjectProvider::notify(&ProxyObjectListener::validated);
}

void ProxyObject::invalidate() {
    ProxyObjectProvider::notify(&ProxyObjectListener::invalidated);
}

void ProxyObject::destroy() {
    ProxyObjectProvider::notify(&ProxyObjectListener::destroyed);
    PositionProvider::notify(&PositionListener::destroyed);
    //FIXME mManager->notify(&ProxyCreationListener::onDestroyProxy);
}



bool ProxyObject::sendMessage(const ODP::PortID& dest_port, MemoryReference message) const {
    ODP::Endpoint dest(mID.space(), mID.object(), dest_port);
    return mDefaultPort->send(dest, message);
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
    return mLoc.velocity() == Vector3f::nil() && mOrientation.velocity() == Quaternion::identity();
}


void ProxyObject::setLocation(const TimedMotionVector3f& reqloc, uint64 seqno, bool predictive) {
    if (seqno < mUpdateSeqno[LOC_POS_PART] && !predictive) return;

    if (!predictive) mUpdateSeqno[LOC_POS_PART] = seqno;

    // FIXME at some point we need to resolve these, but this might
    // require additional information from the space server, e.g. if
    // requests were actually accepted. This all gets very tricky
    // unless we track multiple outstanding update requests and can
    // figure out which one failed, even if the space server generates
    // other update while handling eht requests...
    if (predictive || reqloc.updateTime() > mLoc.updateTime()) {
        mLoc = reqloc;
        PositionProvider::notify(&PositionListener::updateLocation, mLoc, mOrientation, mBounds,mID);
    }
}

void ProxyObject::setOrientation(const TimedMotionQuaternion& reqorient, uint64 seqno, bool predictive) {
    if (seqno < mUpdateSeqno[LOC_ORIENT_PART] && !predictive) return;

    if (!predictive) mUpdateSeqno[LOC_ORIENT_PART] = seqno;

    // FIXME see relevant comment in setLocation
    if (predictive || reqorient.updateTime() > mOrientation.updateTime()) {
        mOrientation = reqorient;//TimedMotionQuaternion(reqorient.time(), MotionQuaternion(reqorient.position(), reqorient.velocity()));
        PositionProvider::notify(&PositionListener::updateLocation, mLoc, mOrientation, mBounds,mID);
    }
}

void ProxyObject::setBounds(const BoundingSphere3f& bnds, uint64 seqno, bool predictive) {
    if (seqno < mUpdateSeqno[LOC_BOUNDS_PART] && !predictive) return;

    if (!predictive) mUpdateSeqno[LOC_BOUNDS_PART] = seqno;
    mBounds = bnds;
    PositionProvider::notify(&PositionListener::updateLocation, mLoc, mOrientation, mBounds,mID);
    ProxyObjectPtr ptr = getSharedPtr();
    assert(ptr);
    MeshProvider::notify (&MeshListener::onSetScale, ptr, mBounds.radius(),mID);
}

ProxyObjectPtr ProxyObject::getParentProxy() const {
    return ProxyObjectPtr();
}

//you can set a camera's mesh as of now.
void ProxyObject::setMesh (Transfer::URI const& mesh, uint64 seqno, bool predictive) {


    if (seqno < mUpdateSeqno[LOC_MESH_PART] && !predictive) return;

    if (!predictive) mUpdateSeqno[LOC_MESH_PART] = seqno;

    mMeshURI = mesh;

    ProxyObjectPtr ptr = getSharedPtr();
    if (ptr) MeshProvider::notify ( &MeshListener::onSetMesh, ptr, mesh,mID);
}

//cameras may have meshes as of now.
Transfer::URI const& ProxyObject::getMesh () const
{
    return mMeshURI;
}


void ProxyObject::setPhysics (const String& rhs, uint64 seqno, bool predictive) {
    if (seqno < mUpdateSeqno[LOC_PHYSICS_PART] && !predictive) return;
    if (!predictive) mUpdateSeqno[LOC_PHYSICS_PART] = seqno;
    mPhysics = rhs;
    ProxyObjectPtr ptr = getSharedPtr();
    if (ptr) MeshProvider::notify ( &MeshListener::onSetPhysics, ptr, rhs,mID);
}

const String& ProxyObject::getPhysics () const {
    return mPhysics;
}

}
