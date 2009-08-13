/*  cbr
 *  AlwaysLocationUpdatePolicy.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include "AlwaysLocationUpdatePolicy.hpp"
#include "Message.hpp"

namespace CBR {

AlwaysLocationUpdatePolicy::AlwaysLocationUpdatePolicy(ServerID sid, LocationService* locservice, MessageRouter* router)
 : LocationUpdatePolicy(sid, locservice, router)
{
}

AlwaysLocationUpdatePolicy::~AlwaysLocationUpdatePolicy() {
    for(ServerSubscriptionMap::iterator sub_it = mServerSubscriptions.begin(); sub_it != mServerSubscriptions.end(); sub_it++)
        delete sub_it->second;
    mServerSubscriptions.clear();

    for(ObjectSubscribersMap::iterator sub_it = mObjectSubscribers.begin(); sub_it != mObjectSubscribers.end(); sub_it++)
        delete sub_it->second;
    mObjectSubscribers.clear();
}

void AlwaysLocationUpdatePolicy::subscribe(ServerID remote, const UUID& uuid) {
    // Add object to server's subscription list
    ServerSubscriptionMap::iterator sub_it = mServerSubscriptions.find(remote);
    if (sub_it == mServerSubscriptions.end()) {
        mServerSubscriptions[remote] = new ServerSubscriberInfo;
        sub_it = mServerSubscriptions.find(remote);
    }
    ServerSubscriberInfo* subs = sub_it->second;
    subs->subscribedTo.insert(uuid);

    // Add server to object's subscribers list
    ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
    if (obj_sub_it == mObjectSubscribers.end()) {
        mObjectSubscribers[uuid] = new ServerIDSet();
        obj_sub_it = mObjectSubscribers.find(uuid);
    }
    ServerIDSet* obj_subs = obj_sub_it->second;
    obj_subs->insert(remote);
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote, const UUID& uuid) {
    // Remove object from server's list
    ServerSubscriptionMap::iterator sub_it = mServerSubscriptions.find(remote);
    if (sub_it != mServerSubscriptions.end()) {
        ServerSubscriberInfo* subs = sub_it->second;
        subs->subscribedTo.erase(uuid);
    }

    // Remove server from object's list
    ObjectSubscribersMap::iterator obj_it = mObjectSubscribers.find(uuid);
    if (obj_it != mObjectSubscribers.end()) {
        ServerIDSet* subs = obj_it->second;
        subs->erase(remote);
    }
}

void AlwaysLocationUpdatePolicy::unsubscribe(ServerID remote) {
    ServerSubscriptionMap::iterator sub_it = mServerSubscriptions.find(remote);
    if (sub_it == mServerSubscriptions.end())
        return;

    ServerSubscriberInfo* subs = sub_it->second;
    while(!subs->subscribedTo.empty())
        unsubscribe(remote, *(subs->subscribedTo.begin()));

    // Might have outstanding updates, so leave it in place and
    // potentially remove in the tick that actually sends updates.
}

void AlwaysLocationUpdatePolicy::localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    // Ignore, initial additions will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::localObjectRemoved(const UUID& uuid) {
    // Ignore, removals will be handled by a prox update
}

void AlwaysLocationUpdatePolicy::localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    // Add the update to each subscribed object
    ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
    if (obj_sub_it == mObjectSubscribers.end()) return;

    ServerIDSet* object_subscribers = obj_sub_it->second;

    for(ServerIDSet::iterator subscriber_it = object_subscribers->begin(); subscriber_it != object_subscribers->end(); subscriber_it++) {
        ServerSubscriberInfo* sub_info = mServerSubscriptions[*subscriber_it];
        assert(sub_info->subscribedTo.find(uuid) != sub_info->subscribedTo.end());

        if (sub_info->outstandingUpdates.find(uuid) == sub_info->outstandingUpdates.end()) {
            UpdateInfo new_ui;
            new_ui.location = mLocService->location(uuid);
            new_ui.bounds = mLocService->bounds(uuid);
            sub_info->outstandingUpdates[uuid] = new_ui;
        }

        UpdateInfo& ui = sub_info->outstandingUpdates[uuid];
        ui.location = newval;
    }
}

void AlwaysLocationUpdatePolicy::localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    // Add the update to each subscribed object
    ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
    if (obj_sub_it == mObjectSubscribers.end()) return;

    ServerIDSet* object_subscribers = obj_sub_it->second;

    for(ServerIDSet::iterator subscriber_it = object_subscribers->begin(); subscriber_it != object_subscribers->end(); subscriber_it++) {
        ServerSubscriberInfo* sub_info = mServerSubscriptions[*subscriber_it];
        assert(sub_info->subscribedTo.find(uuid) != sub_info->subscribedTo.end());

        if (sub_info->outstandingUpdates.find(uuid) == sub_info->outstandingUpdates.end()) {
            UpdateInfo new_ui;
            new_ui.location = mLocService->location(uuid);
            new_ui.bounds = mLocService->bounds(uuid);
            sub_info->outstandingUpdates[uuid] = new_ui;
        }

        UpdateInfo& ui = sub_info->outstandingUpdates[uuid];
        ui.bounds = newval;
    }
}


void AlwaysLocationUpdatePolicy::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
    // FIXME need to fill in when this is handling object subscriptions
}

void AlwaysLocationUpdatePolicy::replicaObjectRemoved(const UUID& uuid) {
    // FIXME need to fill in when this is handling object subscriptions
}

void AlwaysLocationUpdatePolicy::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    // FIXME need to fill in when this is handling object subscriptions
}

void AlwaysLocationUpdatePolicy::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    // FIXME need to fill in when this is handling object subscriptions
}

void AlwaysLocationUpdatePolicy::tick(const Time& t) {
    std::list<ServerID> to_delete;

    for(ServerSubscriptionMap::iterator server_it = mServerSubscriptions.begin(); server_it != mServerSubscriptions.end(); server_it++) {
        ServerID sid = server_it->first;
        ServerSubscriberInfo* sub_info = server_it->second;

        BulkLocationMessage* msg = new BulkLocationMessage(mID);

        for(std::map<UUID, UpdateInfo>::iterator up_it = sub_info->outstandingUpdates.begin(); up_it != sub_info->outstandingUpdates.end(); up_it++) {
            CBR::Protocol::Loc::ILocationUpdate update = msg->contents.add_update();
            update.set_object(up_it->first);
            CBR::Protocol::Loc::ITimedMotionVector location = update.mutable_location();
            location.set_t(up_it->second.location.updateTime());
            location.set_position(up_it->second.location.position());
            location.set_velocity(up_it->second.location.velocity());
            update.set_bounds(up_it->second.bounds);

        }
        mRouter->route(msg, sid);

        sub_info->outstandingUpdates.clear();

        if (sub_info->subscribedTo.empty()) {
            delete sub_info;
            to_delete.push_back(sid);
        }
    }

    for(std::list<ServerID>::iterator it = to_delete.begin(); it != to_delete.end(); it++)
        mServerSubscriptions.erase(*it);
}

} // namespace CBR
