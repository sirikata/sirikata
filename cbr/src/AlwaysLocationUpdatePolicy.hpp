/*  Sirikata
 *  AlwaysLocationUpdatePolicy.hpp
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

#ifndef _ALWAYS_LOCATION_UPDATE_POLICY_HPP_
#define _ALWAYS_LOCATION_UPDATE_POLICY_HPP_

#include "LocationService.hpp"
#include "Options.hpp"
#include <sirikata/cbrcore/Options.hpp>

#include "CBR_Loc.pbj.hpp"

namespace Sirikata {

/** A LocationUpdatePolicy which always sends a location
 *  update message to all subscribers on any position update.
 */
class AlwaysLocationUpdatePolicy : public LocationUpdatePolicy {
public:
    AlwaysLocationUpdatePolicy(LocationService* locservice);
    virtual ~AlwaysLocationUpdatePolicy();

    virtual void subscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote);

    virtual void subscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote);

    virtual void localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void localObjectRemoved(const UUID& uuid);
    virtual void localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);

    virtual void service();

private:
    bool trySend(const UUID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);
    bool trySend(const ServerID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);

    struct UpdateInfo {
        TimedMotionVector3f location;
        BoundingSphere3f bounds;
    };

    template<typename SubscriberType>
    struct SubscriberIndex {
        AlwaysLocationUpdatePolicy* parent;

        typedef std::set<UUID> UUIDSet;
        typedef std::set<SubscriberType> SubscriberSet;

        struct SubscriberInfo {
            UUIDSet subscribedTo;
            std::map<UUID, UpdateInfo> outstandingUpdates;
        };

        // Forward index: Subscriber -> Objects + Updates
        typedef std::map<SubscriberType, SubscriberInfo*> SubscriberMap;
        SubscriberMap mSubscriptions;
        // Reverse index: Objects -> Subscribers
        typedef std::map<UUID, SubscriberSet*> ObjectSubscribersMap;
        ObjectSubscribersMap mObjectSubscribers;

        SubscriberIndex(AlwaysLocationUpdatePolicy* p)
         : parent(p)
        {
        }

        ~SubscriberIndex() {
            for(typename SubscriberMap::iterator sub_it = mSubscriptions.begin(); sub_it != mSubscriptions.end(); sub_it++)
                delete sub_it->second;
            mSubscriptions.clear();

            for(typename ObjectSubscribersMap::iterator sub_it = mObjectSubscribers.begin(); sub_it != mObjectSubscribers.end(); sub_it++)
                delete sub_it->second;
            mObjectSubscribers.clear();
        }

        void subscribe(const SubscriberType& remote, const UUID& uuid) {
            // Add object to server's subscription list
            typename SubscriberMap::iterator sub_it = mSubscriptions.find(remote);
            if (sub_it == mSubscriptions.end()) {
                mSubscriptions[remote] = new SubscriberInfo;
                sub_it = mSubscriptions.find(remote);
            }
            SubscriberInfo* subs = sub_it->second;
            subs->subscribedTo.insert(uuid);

            // Add server to object's subscribers list
            typename ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
            if (obj_sub_it == mObjectSubscribers.end()) {
                mObjectSubscribers[uuid] = new SubscriberSet();
                obj_sub_it = mObjectSubscribers.find(uuid);
            }
            SubscriberSet* obj_subs = obj_sub_it->second;
            obj_subs->insert(remote);
        }

        void unsubscribe(const SubscriberType& remote, const UUID& uuid) {
            // Remove object from server's list
            typename SubscriberMap::iterator sub_it = mSubscriptions.find(remote);
            if (sub_it != mSubscriptions.end()) {
                SubscriberInfo* subs = sub_it->second;
                subs->subscribedTo.erase(uuid);
            }

            // Remove server from object's list
            typename ObjectSubscribersMap::iterator obj_it = mObjectSubscribers.find(uuid);
            if (obj_it != mObjectSubscribers.end()) {
                SubscriberSet* subs = obj_it->second;
                subs->erase(remote);
            }
        }

        void unsubscribe(const SubscriberType& remote) {
            typename SubscriberMap::iterator sub_it = mSubscriptions.find(remote);
            if (sub_it == mSubscriptions.end())
                return;

            SubscriberInfo* subs = sub_it->second;

            while(!subs->subscribedTo.empty()) {
                UUID tmp=*(subs->subscribedTo.begin());
                unsubscribe(remote, tmp);

            }

            // Might have outstanding updates, so leave it in place and
            // potentially remove in the tick that actually sends updates.
        }


        void locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval, LocationService* locservice) {
            // Add the update to each subscribed object
            typename ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
            if (obj_sub_it == mObjectSubscribers.end()) return;

            SubscriberSet* object_subscribers = obj_sub_it->second;

            for(typename SubscriberSet::iterator subscriber_it = object_subscribers->begin(); subscriber_it != object_subscribers->end(); subscriber_it++) {
                if (mSubscriptions.find(*subscriber_it) == mSubscriptions.end()) continue; // XXX FIXME
                assert(mSubscriptions.find(*subscriber_it) != mSubscriptions.end());
                SubscriberInfo* sub_info = mSubscriptions[*subscriber_it];
                if (sub_info->subscribedTo.find(uuid) == sub_info->subscribedTo.end()) continue; // XXX FIXME
                assert(sub_info->subscribedTo.find(uuid) != sub_info->subscribedTo.end());

                if (sub_info->outstandingUpdates.find(uuid) == sub_info->outstandingUpdates.end()) {
                    UpdateInfo new_ui;
                    new_ui.location = locservice->location(uuid);
                    new_ui.bounds = locservice->bounds(uuid);
                    sub_info->outstandingUpdates[uuid] = new_ui;
                }

                UpdateInfo& ui = sub_info->outstandingUpdates[uuid];
                ui.location = newval;
            }
        }

        void boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval, LocationService* locservice) {
            // Add the update to each subscribed object
            typename ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
            if (obj_sub_it == mObjectSubscribers.end()) return;

            SubscriberSet* object_subscribers = obj_sub_it->second;

            for(typename SubscriberSet::iterator subscriber_it = object_subscribers->begin(); subscriber_it != object_subscribers->end(); subscriber_it++) {
                if (mSubscriptions.find(*subscriber_it) == mSubscriptions.end()) continue; // XXX FIXME
                assert(mSubscriptions.find(*subscriber_it) != mSubscriptions.end());
                SubscriberInfo* sub_info = mSubscriptions[*subscriber_it];
                if (sub_info->subscribedTo.find(uuid) == sub_info->subscribedTo.end()) continue; // XXX FIXME
                assert(sub_info->subscribedTo.find(uuid) != sub_info->subscribedTo.end());

                if (sub_info->outstandingUpdates.find(uuid) == sub_info->outstandingUpdates.end()) {
                    UpdateInfo new_ui;
                    new_ui.location = locservice->location(uuid);
                    new_ui.bounds = locservice->bounds(uuid);
                    sub_info->outstandingUpdates[uuid] = new_ui;
                }

                UpdateInfo& ui = sub_info->outstandingUpdates[uuid];
                ui.bounds = newval;
            }
        }

        void service() {
            uint32 max_updates = GetOption(LOC_MAX_PER_RESULT)->as<uint32>();

            std::list<SubscriberType> to_delete;

            for(typename SubscriberMap::iterator server_it = mSubscriptions.begin(); server_it != mSubscriptions.end(); server_it++) {
                SubscriberType sid = server_it->first;
                SubscriberInfo* sub_info = server_it->second;

                Sirikata::Protocol::Loc::BulkLocationUpdate bulk_update;

                bool send_failed = false;
                std::map<UUID, UpdateInfo>::iterator last_shipped = sub_info->outstandingUpdates.begin();
                for(std::map<UUID, UpdateInfo>::iterator up_it = sub_info->outstandingUpdates.begin(); up_it != sub_info->outstandingUpdates.end(); up_it++) {
                    Sirikata::Protocol::Loc::ILocationUpdate update = bulk_update.add_update();
                    update.set_object(up_it->first);
                    Sirikata::Protocol::Loc::ITimedMotionVector location = update.mutable_location();
                    location.set_t(up_it->second.location.updateTime());
                    location.set_position(up_it->second.location.position());
                    location.set_velocity(up_it->second.location.velocity());
                    update.set_bounds(up_it->second.bounds);

                    // If we hit the limit for this update, try to send it out
                    if (bulk_update.update_size() > (int32)max_updates) {
                        bool sent = parent->trySend(sid, bulk_update);
                        if (!sent) {
                            send_failed = true;
                            break;
                        }
                        else {
                            bulk_update = Sirikata::Protocol::Loc::BulkLocationUpdate(); // clear it out
                            last_shipped = up_it;
                        }
                    }
                }

                // Try to send the last few if necessary/possible
                if (!send_failed && bulk_update.update_size() > 0) {
                    bool sent = parent->trySend(sid, bulk_update);
                    if (sent)
                        last_shipped = sub_info->outstandingUpdates.end();
                }

                // Finally clear out any entries successfully sent out
                sub_info->outstandingUpdates.erase( sub_info->outstandingUpdates.begin(), last_shipped);

                if (sub_info->subscribedTo.empty() && sub_info->outstandingUpdates.empty()) {
                    delete sub_info;
                    to_delete.push_back(sid);
                }
            }

            for(typename std::list<SubscriberType>::iterator it = to_delete.begin(); it != to_delete.end(); it++)
                mSubscriptions.erase(*it);
        }

    };

    typedef SubscriberIndex<ServerID> ServerSubscriberIndex;
    ServerSubscriberIndex mServerSubscriptions;

    typedef SubscriberIndex<UUID> ObjectSubscriberIndex;
    ObjectSubscriberIndex mObjectSubscriptions;
}; // class AlwaysLocationUpdatePolicy

} // namespace Sirikata

#endif //_ALWAYS_LOCATION_UPDATE_POLICY_HPP_
