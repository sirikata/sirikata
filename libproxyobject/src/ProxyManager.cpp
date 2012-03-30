/*  Sirikata
 *  ProxyManager.cpp
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
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>

// Helper for checking serialization of data access to a ProxyManager. These
// don't necessarily cover all conflicts or uses, they just cover many parts of
// ProxyManager where we don't use locks directly (or data would escape mutex
// because it is returned) so we know problems could arise if they aren't
// properly protected by callers.
#define PROXYMAN_SERIALIZED() SerializationCheck::Scoped ___proxy_manager_serialization_check(const_cast<ProxyManager*>(this))

namespace Sirikata {

ProxyManagerPtr ProxyManager::construct(VWObjectPtr parent, const SpaceObjectReference& _id) {
    ProxyManagerPtr res(SelfWeakPtr<ProxyManager>::internalConstruct(new ProxyManager(parent, _id)));
    return res;
}

ProxyManager::ProxyManager(VWObjectPtr parent, const SpaceObjectReference& _id)
 : mParent(parent),
   mID(_id)
{}

ProxyManager::~ProxyManager() {
    destroy();
}

void ProxyManager::initialize() {
}

void ProxyManager::destroy() {
    PROXYMAN_SERIALIZED();

    for (ProxyMap::iterator iter = mProxyMap.begin();
         iter != mProxyMap.end();
         ++iter) {
        if (iter->second.ptr) {
            iter->second.ptr->destroy();
            notify(&ProxyCreationListener::onDestroyProxy,iter->second.ptr);
        }
    }
    mProxyMap.clear();
}

ProxyObjectPtr ProxyManager::createObject(
    const SpaceObjectReference& id,
    const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq, const BoundingSphere3f& bs,
    const Transfer::URI& meshuri, const String& phy, bool isAggregate, uint64 seqNo
)
{
    PROXYMAN_SERIALIZED();

    ProxyObjectPtr newObj;
    // Try to reuse an existing object, even if we only have a valid
    // weak pointer to it.
    assert(id.space() == mID.space());
    ProxyMap::iterator iter = mProxyMap.find(id.object());
    if (iter != mProxyMap.end()) {
        // From strong ref
        newObj = iter->second.ptr;
        if (!newObj) {
            // From weak ref
            newObj = iter->second.wptr.lock();

            // And either update the strong ref or clear out the entry
            // if its not even valid anymore.
            if (newObj)
                iter->second.ptr = newObj;
            else
                mProxyMap.erase(iter);
        }
    }

    // If we couldn't get a valid existing copy, create and insert a
    // new one.
    if (!newObj) {
        newObj = ProxyObject::construct(getSharedPtr(), id);
        std::pair<ProxyMap::iterator, bool> result = mProxyMap.insert(
            ProxyMap::value_type(
                newObj->getObjectReference().object(),
                ProxyData(newObj)
            )
        );
        iter = result.first;
    }

    assert(newObj);
    assert(newObj->getObjectReference() == id);
    assert(newObj->getOwner().get() == this);

    // This makes things simpler elsewhere: For new objects, we ensure
    // all the values are set properly so that when the notification
    // happens below, the proxy passed to listeners (for
    // onCreateProxy) will be completely setup, making it valid for
    // use. We don't need this for old ProxyObjects since they were
    // already initialized. The seqNo of 0 only updates something if it wasn't
    // set yet.
    newObj->setLocation(tmv, 0);
    newObj->setOrientation(tmq, 0);
    newObj->setBounds(bs, 0);
    if(meshuri)
        newObj->setMesh(meshuri, 0);
    if(phy.size() > 0)
        newObj->setPhysics(phy, 0);
    newObj->setIsAggregate(isAggregate, 0);

    // Notification of the proxy will have already occured, but
    // updates via, e.g., PositionListener or MeshListener, will go
    // out here, so the potentially invalid initial data automatically
    // filled when the object was created by createObject() shouldn't
    // matter.
    newObj->setLocation(tmv, seqNo);
    newObj->setOrientation(tmq, seqNo);
    newObj->setBounds(bs, seqNo);
    if(meshuri)
        newObj->setMesh(meshuri, seqNo);
    if(phy.size() > 0)
        newObj->setPhysics(phy, seqNo);
    newObj->setIsAggregate(isAggregate, seqNo);

    // Notification has to happen either way
    notify(&ProxyCreationListener::onCreateProxy, newObj);

    return newObj;
}

void ProxyManager::destroyObject(const ProxyObjectPtr &delObj) {
    PROXYMAN_SERIALIZED();

    ProxyMap::iterator iter = mProxyMap.find(delObj->getObjectReference().object());
    if (iter != mProxyMap.end()) {
        iter->second.ptr->destroy();
        notify(&ProxyCreationListener::onDestroyProxy,iter->second.ptr);
        // Here we only erase the strong reference, keeping the weak one so we
        // can recover it if its still in use and we get a re-addition. Be
        // careful not to use the iterator after this since this may trigger
        // destruction of the object which will call proxyDeleted and invalidate
        // it!
        iter->second.ptr.reset();
    }
}

void ProxyManager::proxyDeleted(const ObjectReference& id) {
    PROXYMAN_SERIALIZED();

    ProxyMap::iterator iter = mProxyMap.find(id);
    // It should either be in here, or we should be empty after a call
    // to destroy().
    assert(iter != mProxyMap.end() || mProxyMap.empty());
    if (iter == mProxyMap.end()) {
        assert(mProxyMap.empty());
        return;
    }

    assert(!(iter->second.ptr));
    // This is where it's actually safe to erase because everything has lost
    // references to it.
    mProxyMap.erase(iter);
}

ProxyObjectPtr ProxyManager::getProxyObject(const SpaceObjectReference &id) const {
    PROXYMAN_SERIALIZED();

    assert(id.space() == mID.space());

    ProxyMap::const_iterator iter = mProxyMap.find(id.object());
    if (iter != mProxyMap.end())
        return (*iter).second.ptr;

    return ProxyObjectPtr();
}



//runs through all object references held by this particular object host proxy
//manager and returns them in vecotr form.
void ProxyManager::getAllObjectReferences(std::vector<SpaceObjectReference>& allObjReferences) const
{
    PROXYMAN_SERIALIZED();

    ProxyMap::const_iterator iter;

    SpaceID space = mID.space();
    for (iter = mProxyMap.begin(); iter != mProxyMap.end(); ++iter) {
        if (iter->second.ptr)
            allObjReferences.push_back(SpaceObjectReference(space, iter->first));
    }
}

void ProxyManager::resetAllProxies() {
    PROXYMAN_SERIALIZED();

    for (ProxyMap::const_iterator iter = mProxyMap.begin(); iter != mProxyMap.end(); ++iter) {
        // Just try locking the weak pointer, if that doesn't work nothing
        // will. Note that we want to catch *everything*, even ones we don't
        // keep a strong ref to. This ensures that we if we reuse a proxy later
        // via the weak reference, we won't forget to reset it. Since resetting
        // only affects seqnos, this shouldn't have any adverse affects on those
        // still holding a reference.
        ProxyObjectPtr proxy = iter->second.wptr.lock();
        if (proxy) proxy->reset();
    }
}

int32 ProxyManager::size() {
    return mProxyMap.size();
}

int32 ProxyManager::activeSize() {
    int32 count = 0;
    for (ProxyMap::const_iterator iter = mProxyMap.begin(); iter != mProxyMap.end(); ++iter) {
        if (iter->second.ptr) count++;
    }
    return count;
}

}
