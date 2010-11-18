/*  Sirikata liboh -- Object Host
 *  ObjectHostProxyManager.cpp
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>
#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <vector>


namespace Sirikata {

void ObjectHostProxyManager::initialize() {
}
void ObjectHostProxyManager::destroy() {
    for (ProxyMap::iterator iter = mProxyMap.begin();
         iter != mProxyMap.end();
         ++iter) {
        iter->second.obj->destroy();
        notify(&ProxyCreationListener::onDestroyProxy,iter->second.obj);
    }
    mProxyMap.clear();
}
ObjectHostProxyManager::~ObjectHostProxyManager() {
	destroy();
}

void ObjectHostProxyManager::createObject(const ProxyObjectPtr &newObj) {
    ProxyMap::iterator iter = mProxyMap.find(newObj->getObjectReference().object());
    if (iter == mProxyMap.end()) {
        std::pair<ProxyMap::iterator, bool> result = mProxyMap.insert(
            ProxyMap::value_type(newObj->getObjectReference().object(), newObj));
        iter = result.first;
        notify(&ProxyCreationListener::onCreateProxy,newObj);
    }
}
void ObjectHostProxyManager::destroyObject(const ProxyObjectPtr &delObj) {
    ProxyMap::iterator iter = mProxyMap.find(delObj->getObjectReference().object());
    if (iter != mProxyMap.end()) {
        iter->second.obj->destroy();
        notify(&ProxyCreationListener::onDestroyProxy,iter->second.obj);
        mProxyMap.erase(iter);
    }
}

ProxyObjectPtr ObjectHostProxyManager::getProxyObject(const SpaceObjectReference &id) const {
    assert(id.space() == mSpaceID);

    ProxyMap::const_iterator iter = mProxyMap.find(id.object());
    if (iter != mProxyMap.end())
        return (*iter).second.obj;

    return ProxyObjectPtr();
}



//runs through all object references held by this particular object host proxy
//manager and returns them in vecotr form.
void ObjectHostProxyManager::getAllObjectReferences(std::vector<SpaceObjectReference>& allObjReferences) const
{
    ProxyMap::const_iterator iter;
    
    std::cout << "Size of Proxy map is " << mProxyMap.size() << "\n";


    for (iter = mProxyMap.begin(); iter != mProxyMap.end(); ++iter)
        allObjReferences.push_back(SpaceObjectReference(mSpaceID,iter->first));
}

void ObjectHostProxyManager::getAllObjectReferences(std::vector<SpaceObjectReference*>& allObjReferences) const
{
    ProxyMap::const_iterator iter;
    for (iter = mProxyMap.begin(); iter != mProxyMap.end(); ++iter)
    {
        //ObjectReference tmp = iter->first;
        SpaceObjectReference* toPush = new SpaceObjectReference(mSpaceID, iter->first);
        //allObjReferences.push_back(iter->first);
        allObjReferences.push_back(toPush);
    }
}


}
