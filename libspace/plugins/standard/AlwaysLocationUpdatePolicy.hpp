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

#include <sirikata/space/LocationService.hpp>
#include <sirikata/core/options/CommonOptions.hpp>

#include "Protocol_Loc.pbj.hpp"

#define ALWAYS_POLICY_OPTIONS      "always_location_update_policy"
#define LOC_MAX_PER_RESULT         "loc.max-per-result"

namespace Sirikata {

void InitAlwaysLocationUpdatePolicyOptions();

/** A LocationUpdatePolicy which always sends a location
 *  update message to all subscribers on any position update.
 */
class AlwaysLocationUpdatePolicy : public LocationUpdatePolicy {
public:
    AlwaysLocationUpdatePolicy(const String& args);
    virtual ~AlwaysLocationUpdatePolicy();

    virtual void subscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote);

    virtual void subscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote);

    virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval);
    virtual void localMeshUpdated(const UUID& uuid, bool agg, const String& newval);

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);

    virtual void service();

private:
    typedef Stream<SpaceObjectReference>::Ptr SSTStreamPtr;

    void tryCreateChildStream(SSTStreamPtr parent_stream, std::string* msg, int count);
    void locSubstreamCallback(int x, SSTStreamPtr substream, SSTStreamPtr parent_substream, std::string* msg, int count);

    bool trySend(const UUID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);
    bool trySend(const ServerID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);

    struct UpdateInfo {
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        BoundingSphere3f bounds;
        String mesh;
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

        typedef std::tr1::function<void(UpdateInfo&)> UpdateFunctor;
        // Generic version of an update - adds updates per-subscriber as
        // necessary and calls the UpdateFunctor to trigger the particular
        // update to values.
        void propertyUpdated(const UUID& uuid, LocationService* locservice, UpdateFunctor fup) {
            // Add the update to each subscribed object

            
            typename ObjectSubscribersMap::iterator obj_sub_it = mObjectSubscribers.find(uuid);
            if (obj_sub_it == mObjectSubscribers.end()) return;

            SubscriberSet* object_subscribers = obj_sub_it->second;
            
            for(typename SubscriberSet::iterator subscriber_it = object_subscribers->begin(); subscriber_it != object_subscribers->end(); subscriber_it++)
            {
                
                if (mSubscriptions.find(*subscriber_it) == mSubscriptions.end()) continue; // XXX FIXME
                assert(mSubscriptions.find(*subscriber_it) != mSubscriptions.end());
                SubscriberInfo* sub_info = mSubscriptions[*subscriber_it];
                if (sub_info->subscribedTo.find(uuid) == sub_info->subscribedTo.end()) continue; // XXX FIXME
                assert(sub_info->subscribedTo.find(uuid) != sub_info->subscribedTo.end());

                
                if (sub_info->outstandingUpdates.find(uuid) == sub_info->outstandingUpdates.end()) {
                    UpdateInfo new_ui;
                    new_ui.location = locservice->location(uuid);
                    new_ui.bounds = locservice->bounds(uuid);
                    new_ui.mesh = locservice->mesh(uuid);
                    new_ui.orientation = locservice->orientation(uuid);
                    sub_info->outstandingUpdates[uuid] = new_ui;
                }

                UpdateInfo& ui = sub_info->outstandingUpdates[uuid];
                fup(ui);
            }
        }

        static void setUILocation(UpdateInfo& ui, const TimedMotionVector3f& newval) {ui.location = newval; }
        static void setUIOrientation(UpdateInfo& ui, const TimedMotionQuaternion& newval) { ui.orientation = newval; }
        static void setUIBounds(UpdateInfo& ui, const BoundingSphere3f& newval) { ui.bounds = newval; }
        static void setUIMesh(UpdateInfo& ui, const String& newval) {ui.mesh = newval;}

        void locationUpdated(const UUID& uuid, const TimedMotionVector3f& newval, LocationService* locservice) {
            propertyUpdated(
                uuid, locservice,
                std::tr1::bind(&setUILocation, std::tr1::placeholders::_1, newval)
            );
        }

        void orientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval, LocationService* locservice) {
            propertyUpdated(
                uuid, locservice,
                std::tr1::bind(&setUIOrientation, std::tr1::placeholders::_1, newval)
            );
        }

        void boundsUpdated(const UUID& uuid, const BoundingSphere3f& newval, LocationService* locservice) {
            propertyUpdated(
                uuid, locservice,
                std::tr1::bind(&setUIBounds, std::tr1::placeholders::_1, newval)
            );
        }

        void meshUpdated(const UUID& uuid, const String& newval, LocationService* locservice) {
            propertyUpdated(
                uuid, locservice,
                std::tr1::bind(&setUIMesh, std::tr1::placeholders::_1, newval)
            );
        }


        void service() {
            uint32 max_updates = GetOptionValue<uint32>(ALWAYS_POLICY_OPTIONS, LOC_MAX_PER_RESULT);

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

                    Sirikata::Protocol::ITimedMotionVector location = update.mutable_location();
                    location.set_t(up_it->second.location.updateTime());
                    location.set_position(up_it->second.location.position());
                    location.set_velocity(up_it->second.location.velocity());

                    Sirikata::Protocol::ITimedMotionQuaternion orientation = update.mutable_orientation();
                    orientation.set_t(up_it->second.orientation.updateTime());
                    orientation.set_position(up_it->second.orientation.position());
                    orientation.set_velocity(up_it->second.orientation.velocity());

                    update.set_bounds(up_it->second.bounds);

                    update.set_mesh(up_it->second.mesh);

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
