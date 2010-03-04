/*  cbr
 *  ConnectedObjectTracker.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "ConnectedObjectTracker.hpp"
#include "ObjectHost.hpp"
#include "Object.hpp"

namespace CBR {

ConnectedObjectTracker::ConnectedObjectTracker(ObjectHost* parent)
 : mParent(parent),
   mLastRRObject(UUID::null())
{
    mParent->addListener(this);
}
ConnectedObjectTracker::~ConnectedObjectTracker() {
    mParent->removeListener(this);
}

Object* ConnectedObjectTracker::getObject(const UUID& objid) const {
    ObjectsByID::const_iterator it = mObjectsByID.find(objid);
    if (it == mObjectsByID.end()) return NULL;
    return it->second;
}

namespace {
typedef std::tr1::unordered_set<UUID, UUID::Hasher> ObjectIDSet;
UUID selectID(const ObjectIDSet& uuidMap) {
    ObjectIDSet::const_iterator id_it = uuidMap.begin();
    uint32 obj_num = rand() % uuidMap.size();
    for(uint32 index = 0; index < obj_num; index++,id_it++) {}
    const UUID& objid = *id_it;
    return objid;
}
}

Object* ConnectedObjectTracker::randomObject() {
    if (mObjectsByServer.size()==0)
        return NULL;

    // Pick a server
    int iteratorLevel = rand() % mObjectsByServer.size();
    ObjectsByServerMap::iterator server_it = mObjectsByServer.begin();
    for (int index=0; index<iteratorLevel; ++index,++server_it++) {}

    // Pick a UUID in that server
    const ObjectIDSet& uuidMap = server_it->second;
    if (uuidMap.empty())
        return NULL;
    return getObject(selectID(uuidMap));
}

Object* ConnectedObjectTracker::randomObject(ServerID whichServer) {
    if (whichServer==NullServerID)
        return randomObject();

    ObjectsByServerMap::iterator server_it = mObjectsByServer.find(whichServer);
    if (server_it == mObjectsByServer.end())
        return NULL;

    // Pick a UUID in that server
    const ObjectIDSet& uuidMap = server_it->second;
    if (uuidMap.empty())
        return NULL;
    return getObject(selectID(uuidMap));
}

Object* ConnectedObjectTracker::roundRobinObject() {
    if (mObjectsByID.size() == 0) return NULL;

    // Find last
    UUID myrand(mLastRRObject);
    ObjectsByID::iterator obj_it = mObjectsByID.end();

    // Move to next
    obj_it = mObjectsByID.find(myrand);
    if (obj_it != mObjectsByID.end()) ++obj_it;
    if (obj_it == mObjectsByID.end())
        obj_it = mObjectsByID.begin();

    // Update last and return
    if (obj_it == mObjectsByID.end())
        return NULL;

    mLastRRObject = obj_it->first;
    return obj_it->second;
}

Object* ConnectedObjectTracker::roundRobinObject(ServerID whichServer) {
    if (mObjectsByID.size() == 0) return NULL;

    const ObjectIDSet& server_objects = mObjectsByServer[whichServer];

    UUID next_rr_obj(mLastRRObject);
    {
        ObjectIDSet::const_iterator serv_obj_it = server_objects.find(next_rr_obj);
        if (serv_obj_it != server_objects.end()) serv_obj_it++;
        if (serv_obj_it == server_objects.end()) serv_obj_it = server_objects.begin();
        next_rr_obj = *serv_obj_it;
    }

    ObjectsByID::iterator obj_it = mObjectsByID.find(next_rr_obj);
    if (obj_it == mObjectsByID.end()) return NULL;
    mLastRRObject = next_rr_obj;
    return obj_it->second;
}

ServerID ConnectedObjectTracker::numServerIDs() const {
    return mObjectsByServer.size();
}

void ConnectedObjectTracker::objectHostConnectedObject(ObjectHost* oh, Object* obj, const ServerID& server) {
    mObjectsByServer[server].insert(obj->uuid());
    mObjectsByID[obj->uuid()] = obj;
}

void ConnectedObjectTracker::objectHostMigratedObject(ObjectHost* oh, const UUID& objid, const ServerID& from_server, const ServerID& to_server) {
    {
        ObjectIDSet& server_objects = mObjectsByServer[from_server];
        ObjectIDSet::iterator it = server_objects.find(objid);
        if (it != server_objects.end()) server_objects.erase(it);
    }
    mObjectsByServer[to_server].insert(objid);
}

void ConnectedObjectTracker::objectHostDisconnectedObject(ObjectHost* oh, const UUID& objid, const ServerID& server) {
    ObjectIDSet& server_objects = mObjectsByServer[server];
    ObjectIDSet::iterator it = server_objects.find(objid);
    if (it != server_objects.end()) server_objects.erase(it);

    mObjectsByID.erase(objid);
}

} // namespace CBR
