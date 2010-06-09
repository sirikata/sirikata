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

namespace Sirikata {

ConnectedObjectTracker::ConnectedObjectTracker(ObjectHost* parent)
 : mParent(parent),
   mNumObjects(0),
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

typedef std::tr1::unordered_map<UUID, Object*, UUID::Hasher> ObjectsByID;
Object* selectID(const ObjectsByID& uuidMap) {
    ObjectsByID::const_iterator id_it = uuidMap.begin();
    uint32 obj_num = rand() % uuidMap.size();
    for(uint32 index = 0; index < obj_num; index++,id_it++) {}
    Object* obj = id_it->second;
    return obj;
}
}
size_t ConnectedObjectTracker::numObjectsConnected(ServerID sid) {
    ObjectsByServerMap::iterator iter=mObjectsByServer.find(sid);
    if (iter!=mObjectsByServer.end())
        return iter->second.size();
    return 0;
}
ServerID ConnectedObjectTracker::getServerID(int objectByServerMapNumber) {
    ObjectsByServerMap::iterator iter=mObjectsByServer.begin();
    for (int i=0;i<objectByServerMapNumber;++i,++iter) {

    }
    return iter->first;
}
Object* ConnectedObjectTracker::randomObject() {
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

    if (mObjectsByID.size()==0)
        return NULL;

    return selectID(mObjectsByID);
}

Object* ConnectedObjectTracker::randomObjectFromServer(ServerID whichServer) {
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

    assert (whichServer != NullServerID);

    ObjectsByServerMap::iterator server_it = mObjectsByServer.find(whichServer);
    if (server_it == mObjectsByServer.end())
        return NULL;

    // Pick a UUID in that server
    const ObjectIDSet& uuidMap = server_it->second;
    if (uuidMap.empty())
        return NULL;
    return getObject(selectID(uuidMap));
}

Object* ConnectedObjectTracker::randomObjectExcludingServer(ServerID whichServer) {
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

    assert (whichServer != NullServerID);

    if (mObjectsByID.size() == 0)
        return NULL;

    // Our approach is to select a server based on the CDF of num objects on
    // all servers except the one we're excluding, then select uniformly from
    // that one.

    // Figure out total number of objects on other servers so we can properly
    // evaluate the cdf
    uint32 num_objects_on_other_servers = mNumObjects;
    ObjectsByServerMap::iterator exclude_server_it = mObjectsByServer.find(whichServer);
    if (exclude_server_it != mObjectsByServer.end())
        num_objects_on_other_servers -= exclude_server_it->second.size();

    // Now find the server
    float server_sample = rand() / (float)RAND_MAX;
    float cdf_sum = 0.0;
    for(ObjectsByServerMap::iterator server_it = mObjectsByServer.begin(); server_it != mObjectsByServer.end(); server_it++) {
        if (server_it->first == whichServer) continue;

        float frac = server_it->second.size() / (float)num_objects_on_other_servers;
        float next_cdf_sum = cdf_sum + frac;
        if (server_sample > cdf_sum && server_sample <= next_cdf_sum)
            return getObject(selectID(server_it->second));
    }

    return NULL;
}

Object* ConnectedObjectTracker::roundRobinObject() {
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

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
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

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
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

    mNumObjects++;

    mObjectsByServer[server].insert(obj->uuid());
    mObjectsByID[obj->uuid()] = obj;
}

void ConnectedObjectTracker::objectHostMigratedObject(ObjectHost* oh, const UUID& objid, const ServerID& from_server, const ServerID& to_server) {
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

    {
        ObjectIDSet& server_data = mObjectsByServer[from_server];
        ObjectIDSet::iterator it = server_data.find(objid);
        if (it != server_data.end())
            server_data.erase(it);
    }

    mObjectsByServer[to_server].insert(objid);
}

void ConnectedObjectTracker::objectHostDisconnectedObject(ObjectHost* oh, const UUID& objid, const ServerID& server) {
    boost::unique_lock<boost::shared_mutex> lck(mMutex);

    mNumObjects--;

    ObjectIDSet& server_data = mObjectsByServer[server];
    ObjectIDSet::iterator it = server_data.find(objid);
    if (it != server_data.end())
        server_data.erase(it);

    mObjectsByID.erase(objid);
}

} // namespace Sirikata
