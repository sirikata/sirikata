
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <vector>


namespace Sirikata {

class HostedObject;

class SIRIKATA_OH_EXPORT ObjectHostProxyManager :public ProxyManager, public Noncopyable {
protected:
    struct ObjectHostProxyInfo {
        ProxyObjectPtr obj;
        ObjectHostProxyInfo(const ProxyObjectPtr &obj)
            : obj(obj) {
        }
        inline bool operator<(const ObjectHostProxyInfo &other) const {
            return obj->getObjectReference() < other.obj->getObjectReference();
        }
        inline bool operator==(const ObjectHostProxyInfo &other) const {
            return obj->getObjectReference() == other.obj->getObjectReference();
        }
    };
    typedef std::tr1::unordered_map<ObjectReference, ObjectHostProxyInfo, ObjectReference::Hasher> ProxyMap;
    ProxyMap mProxyMap;
    SpaceID mSpaceID;
public:

    ObjectHostProxyManager(const SpaceID& space)
        : mSpaceID(space)
    {}


    ~ObjectHostProxyManager();
    void initialize();
    void destroy();


    //bftm
    void getAllObjectReferences(std::vector<SpaceObjectReference>& allObjReferences) const;
    void getAllObjectReferences(std::vector<SpaceObjectReference*>& allObjReferences) const;

    void createObject(const ProxyObjectPtr &newObj);
    void destroyObject(const ProxyObjectPtr &delObj);


    ProxyObjectPtr getProxyObject(const SpaceObjectReference &id) const;
};

typedef std::tr1::shared_ptr<ObjectHostProxyManager> ObjectHostProxyManagerPtr;
typedef std::tr1::weak_ptr<ObjectHostProxyManager> ObjectHostProxyManagerWPtr;

}

#endif
