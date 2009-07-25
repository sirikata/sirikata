
/*  Sirikata liboh -- Object Host
 *  ObjectHostProxyManager.hpp
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

#ifndef _SIRIKATA_ObjectHostProxyManager_HPP_
#define _SIRIKATA_ObjectHostProxyManager_HPP_

#include <oh/ProxyManager.hpp>

namespace Sirikata {

class SIRIKATA_OH_EXPORT ObjectHostProxyManager :public ProxyManager, public Noncopyable {
protected:
    struct ObjectHostProxyInfo {
        ProxyObjectPtr obj;
        int refCount;
        ObjectHostProxyInfo(const ProxyObjectPtr &obj)
            : obj(obj), refCount(0) {
        }
        inline bool operator<(const ObjectHostProxyInfo &other) const {
            return obj->getObjectReference() < other.obj->getObjectReference();
        }
        inline bool operator==(const ObjectHostProxyInfo &other) const {
            return obj->getObjectReference() == other.obj->getObjectReference();
        }
    };
    class QueryHasher {
        UUID::Hasher uuidHash;
    public:
        size_t operator()(const std::pair<UUID, uint32>&mypair) const{
            return uuidHash(mypair.first)*17 + mypair.second*19;
        }
    };
    typedef std::tr1::unordered_map<ObjectReference, ObjectHostProxyInfo, ObjectReference::Hasher> ProxyMap;
    typedef std::tr1::unordered_map<std::pair<UUID, uint32>, std::set<ProxyObjectPtr>, QueryHasher > QueryMap;
    ProxyMap mProxyMap;
    QueryMap mQueryMap; // indexed by {ObjectHost::mInternalObjectReference, ProxCall::query_id()}
    SpaceID mSpaceID;
public:
	~ObjectHostProxyManager();
    void initialize();
    void destroy();

    void createObject(const ProxyObjectPtr &newObj);
    void destroyObject(const ProxyObjectPtr &newObj);

    void createObjectProximity(const ProxyObjectPtr &newObj, const UUID &seeker, uint32 queryId);
    void destroyObjectProximity(const ProxyObjectPtr &newObj, const UUID &seeker, uint32 queryId);
    void destroyProximityQuery(const UUID &seeker, uint32 queryId);

    ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const;
};

}

#endif
