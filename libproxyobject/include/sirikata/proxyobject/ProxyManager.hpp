/*  Sirikata Object Host -- Proxy Creation and Destruction manager
 *  ProxyManager.hpp
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

#ifndef _SIRIKATA_PROXY_MANAGER_HPP_
#define _SIRIKATA_PROXY_MANAGER_HPP_

#include <sirikata/core/util/ListenerProvider.hpp>

#include <sirikata/proxyobject/Defs.hpp>
#include "ProxyCreationListener.hpp"
#include <sirikata/core/util/PresenceProperties.hpp>

#include <sirikata/core/util/SerializationCheck.hpp>

namespace Sirikata {

/** An interface for a class that keeps track of proxy object references. */
class SIRIKATA_PROXYOBJECT_EXPORT ProxyManager
    : public SelfWeakPtr<ProxyManager>,
      public Provider<ProxyCreationListener*>,
      public Noncopyable,
      SerializationCheck
{
public:
    static ProxyManagerPtr construct(VWObjectPtr parent, const SpaceObjectReference& _id);
    virtual ~ProxyManager();

    const SpaceObjectReference& id() const { return mID; }

    VWObjectPtr parent() const { return mParent; }

    ///Called after providers attached
    virtual void initialize();
    ///Called before providers detatched
    virtual void destroy();

    ///Adds to internal ProxyObject map and calls creation listeners.
    virtual ProxyObjectPtr createObject(
        const SpaceObjectReference& id,
        const TimedMotionVector3f& tmv, const TimedMotionQuaternion& tmq, const BoundingSphere3f& bs,
        const Transfer::URI& meshuri, const String& phy, bool isAggregate, uint64 seqNo
    );

    ///Removes from internal ProxyObject map, calls destruction listeners, and calls newObj->destroy().
    virtual void destroyObject(const ProxyObjectPtr &newObj);

    /// Get the number of proxies held by this ProxyManager
    int32 size();
    /// Get the number of proxies held by this ProxyManager that are active,
    /// i.e. the ProxyManager is holding a strong reference to them rather than
    /// just tracking that they are alive.
    int32 activeSize();

    /// Ask for a proxy object by ID. Returns ProxyObjectPtr() if it doesn't exist.
    virtual ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const;

    virtual void getAllObjectReferences(std::vector<SpaceObjectReference>& allObjReferences) const;

    /// Resets all ProxyObjects. Use this after migration to ensure they are
    /// back in a clean state for fresh updates starting from a new base
    /// sequence number.
    void resetAllProxies();

private:
    friend class ProxyObject;

    ProxyManager(VWObjectPtr parent, const SpaceObjectReference& _id);

    // These track the *entire* lifetime of ProxyObjects. This allows
    // clients of ProxyManager to hold onto ProxyObjects beyond when
    // they are valid. This ProxyManager keeps track of weak copies of
    // those ProxyObjects and reuses them. This guarantees that there
    // is only one ProxyObject for any given identifier from this
    // ProxyManager at any time, and that it is guaranteed to receive
    // updates (so that, if the object is removed and the re-added to
    // the result set, clients holding references from the first
    // addition will continue to receive updates).
    void proxyDeleted(const ObjectReference& id);

    // Parent HostedObject
    VWObjectPtr mParent;
    // Presence identifier that runs this ProxyManager
    SpaceObjectReference mID;

    struct ProxyData {
        ProxyData(ProxyObjectPtr p)
         : ptr(p), wptr(p)
        {}

        ProxyObjectPtr ptr;
        ProxyObjectWPtr wptr;
    };
    typedef std::tr1::unordered_map<ObjectReference, ProxyData, ObjectReference::Hasher> ProxyMap;
    ProxyMap mProxyMap;
};

typedef std::tr1::shared_ptr<ProxyManager> ProxyManagerPtr;
typedef std::tr1::weak_ptr<ProxyManager> ProxyManagerWPtr;

}

#endif //_SIRIKATA_PROXY_MANAGER_HPP_
