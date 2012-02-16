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
    AlwaysLocationUpdatePolicy(SpaceContext* ctx, const String& args);
    virtual ~AlwaysLocationUpdatePolicy();

    virtual void start();
    virtual void stop();

    virtual void subscribe(ServerID remote, const UUID& uuid, SeqNoPtr seqno);
    virtual void unsubscribe(ServerID remote, const UUID& uuid);
    virtual void unsubscribe(ServerID remote);

    virtual void subscribe(const OHDP::NodeID& remote, const UUID& uuid);
    virtual void unsubscribe(const OHDP::NodeID& remote, const UUID& uuid);
    virtual void unsubscribe(const OHDP::NodeID& remote);

    virtual void subscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote, const UUID& uuid);
    virtual void unsubscribe(const UUID& remote);

    virtual void localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void localObjectRemoved(const UUID& uuid, bool agg);
    virtual void localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval);
    virtual void localOrientationUpdated(const UUID& uuid, bool agg, const TimedMotionQuaternion& newval);
    virtual void localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval);
    virtual void localMeshUpdated(const UUID& uuid, bool agg, const String& newval);
    virtual void localPhysicsUpdated(const UUID& uuid, bool agg, const String& newval);

    virtual void replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics);
    virtual void replicaObjectRemoved(const UUID& uuid);
    virtual void replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval);
    virtual void replicaOrientationUpdated(const UUID& uuid, const TimedMotionQuaternion& newval);
    virtual void replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval);
    virtual void replicaMeshUpdated(const UUID& uuid, const String& newval);
    virtual void replicaPhysicsUpdated(const UUID& uuid, const String& newval);

    virtual void service();

private:
    void reportStats();

    void tryCreateChildStream(const UUID& dest, ODPSST::Stream::Ptr parent_stream, std::string* msg, int count);
    void objectLocSubstreamCallback(int x, ODPSST::Stream::Ptr substream, const UUID& dest, ODPSST::Stream::Ptr parent_substream, std::string* msg, int count);
    void tryCreateChildStream(const OHDP::NodeID& dest, OHDPSST::Stream::Ptr parent_stream, std::string* msg, int count);
    void ohLocSubstreamCallback(int x, OHDPSST::Stream::Ptr substream, const OHDP::NodeID& dest, OHDPSST::Stream::Ptr parent_substream, std::string* msg, int count);

    bool validSubscriber(const UUID& dest);
    bool validSubscriber(const OHDP::NodeID& dest);
    bool validSubscriber(const ServerID& dest);

    bool isSelfSubscriber(const UUID& sid, const UUID& observed);
    bool isSelfSubscriber(const OHDP::NodeID& sid, const UUID& observed);
    bool isSelfSubscriber(const ServerID& sid, const UUID& observed);

    bool trySend(const UUID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);
    bool trySend(const OHDP::NodeID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);
    bool trySend(const ServerID& dest, const Sirikata::Protocol::Loc::BulkLocationUpdate& blu);

    struct UpdateInfo {
        uint64 epoch;
        TimedMotionVector3f location;
        TimedMotionQuaternion orientation;
        BoundingSphere3f bounds;
        String mesh;
        String physics;
    };

    template<typename SubscriberType>
    struct SubscriberIndex {
        AlwaysLocationUpdatePolicy* parent;
        AtomicValue<uint32>& sent_count;

        typedef std::set<UUID> UUIDSet;
        typedef std::set<SubscriberType> SubscriberSet;

        struct SubscriberInfo {
            SubscriberInfo(SeqNoPtr seq_number_ptr )
             : seqnoPtr(seq_number_ptr),
               numOutstandingMessages(0)
            {}

            SeqNoPtr seqnoPtr;
            UUIDSet subscribedTo;
            std::map<UUID, UpdateInfo> outstandingUpdates;
            // Sometimes a subscriber may stall or hang, leaving the underlying
            // connection open but not handling loc update substreams. In this
            // case, we can end up generating a ton of update streams that fail
            // and eat up a bunch of our processor just looping for
            // retries. With objects moving, we could get arbitarily many
            // outstanding updates since once the substream request is started
            // it frees up the spot in the outstandingUpdates map above,
            // allowing more for the same object to be sent. To protect against
            // this, we track how many loc update messages are outstanding and
            // stall updates while we're waiting for them to return (or fail!).
            uint8 numOutstandingMessages;
        };

        // Forward index: Subscriber -> Objects + Updates
        typedef std::map<SubscriberType, SubscriberInfo*> SubscriberMap;
        SubscriberMap mSubscriptions;
        // Reverse index: Objects -> Subscribers
        typedef std::map<UUID, SubscriberSet*> ObjectSubscribersMap;
        ObjectSubscribersMap mObjectSubscribers;

        SubscriberIndex(AlwaysLocationUpdatePolicy* p, AtomicValue<uint32>& _sent_count)
         : parent(p),
           sent_count(_sent_count)
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

        void subscribe(const SubscriberType& remote, const UUID& uuid, SeqNoPtr seqnoPtr) {
            // Add object to server's subscription list
            typename SubscriberMap::iterator sub_it = mSubscriptions.find(remote);
            if (sub_it == mSubscriptions.end()) {
                mSubscriptions[remote] = new SubscriberInfo(seqnoPtr);

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

            // Force an update. This is necessary because the subscription comes
            // in asynchronously from Proximity, so its possible the data sent
            // with the origin subscription is out of date by the time this
            // subscription occurs. Forcing an extra update handles this case.
            propertyUpdatedForSubscriber(uuid, parent->mLocService, remote, NULL);
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
                propertyUpdatedForSubscriber(uuid, locservice, *subscriber_it, fup);
            }
        }

        // Update of location information for individual subscriber. Note that
        // this should only be used in special cases -- mainly to handle when a
        // new subscriber is added.  Otherwise its just a utility for the normal
        // update method above.
        void propertyUpdatedForSubscriber(const UUID& uuid, LocationService* locservice, SubscriberType sub, UpdateFunctor fup) {
            if (mSubscriptions.find(sub) == mSubscriptions.end()) return; // XXX FIXME
            assert(mSubscriptions.find(sub) != mSubscriptions.end());
            SubscriberInfo* sub_info = mSubscriptions[sub];
            if (sub_info->subscribedTo.find(uuid) == sub_info->subscribedTo.end()) return; // XXX FIXME
            assert(sub_info->subscribedTo.find(uuid) != sub_info->subscribedTo.end());

            if (sub_info->outstandingUpdates.find(uuid) == sub_info->outstandingUpdates.end()) {
                UpdateInfo new_ui;
                new_ui.epoch = locservice->epoch(uuid);
                new_ui.location = locservice->location(uuid);
                new_ui.bounds = locservice->bounds(uuid);
                new_ui.mesh = locservice->mesh(uuid);
                new_ui.orientation = locservice->orientation(uuid);
                new_ui.physics = locservice->physics(uuid);
                sub_info->outstandingUpdates[uuid] = new_ui;
            }
            else
                UpdateInfo& ui = sub_info->outstandingUpdates[uuid];


            UpdateInfo& ui = sub_info->outstandingUpdates[uuid];
            if (fup)
                fup(ui);
        }

        static void setUILocation(UpdateInfo& ui, const TimedMotionVector3f& newval) {ui.location = newval; }
        static void setUIOrientation(UpdateInfo& ui, const TimedMotionQuaternion& newval) { ui.orientation = newval; }
        static void setUIBounds(UpdateInfo& ui, const BoundingSphere3f& newval) { ui.bounds = newval; }
        static void setUIMesh(UpdateInfo& ui, const String& newval) {ui.mesh = newval;}
        static void setUIPhysics(UpdateInfo& ui, const String& newval) {ui.physics = newval;}

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

        void physicsUpdated(const UUID& uuid, const String& newval, LocationService* locservice) {
            propertyUpdated(
                uuid, locservice,
                std::tr1::bind(&setUIPhysics, std::tr1::placeholders::_1, newval)
            );
        }


        void service() {
            uint32 max_updates = GetOptionValue<uint32>(ALWAYS_POLICY_OPTIONS, LOC_MAX_PER_RESULT);

            std::list<SubscriberType> to_delete;

            for(typename SubscriberMap::iterator server_it = mSubscriptions.begin(); server_it != mSubscriptions.end(); server_it++) {
                SubscriberType sid = server_it->first;
                SubscriberInfo* sub_info = server_it->second;

                // We can end up with leftover updates after a subscriber has
                // already disconnected. We need to ignore them if we're not
                // even going to be able to send the messages.
                if (!parent->validSubscriber(sid)) {
                    sub_info->outstandingUpdates.clear();
                    if (sub_info->subscribedTo.empty()) {
                        delete sub_info;
                        to_delete.push_back(sid);
                    }
                    continue;
                }

                Sirikata::Protocol::Loc::BulkLocationUpdate bulk_update;

                bool send_failed = false;
                std::map<UUID, UpdateInfo>::iterator last_shipped = sub_info->outstandingUpdates.begin();
                for(std::map<UUID, UpdateInfo>::iterator up_it = sub_info->outstandingUpdates.begin();
                    sub_info->numOutstandingMessages < 25 && up_it != sub_info->outstandingUpdates.end();
                    up_it++)
                {
                    Sirikata::Protocol::Loc::ILocationUpdate update = bulk_update.add_update();
                    update.set_object(up_it->first);

                    //write and update sequence number
                    update.set_seqno( (*(sub_info->seqnoPtr)) ++ );

                    if (parent->isSelfSubscriber(sid, up_it->first))
                        update.set_epoch(up_it->second.epoch);

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
                    update.set_physics(up_it->second.physics);

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
                            sent_count++;
                            sub_info->numOutstandingMessages++;
                        }
                    }
                }

                // Try to send the last few if necessary/possible
                if (!send_failed && bulk_update.update_size() > 0) {
                    bool sent = parent->trySend(sid, bulk_update);
                    if (sent) {
                        last_shipped = sub_info->outstandingUpdates.end();
                        sent_count++;
                        sub_info->numOutstandingMessages++;
                    }
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

        void decrementOutstandingMessageCount(SubscriberType dest) {
            typename SubscriberMap::iterator sub_it = mSubscriptions.find(dest);
            if (sub_it == mSubscriptions.end()) return;
            assert(sub_it->second->numOutstandingMessages > 0);
            sub_it->second->numOutstandingMessages--;
        }

    };

    Poller mStatsPoller;
    Time mLastStatsTime;
    const String mTimeSeriesServerUpdatesName;
    AtomicValue<uint32> mServerUpdatesPerSecond;
    const String mTimeSeriesOHUpdatesName;
    AtomicValue<uint32> mOHUpdatesPerSecond;
    const String mTimeSeriesObjectUpdatesName;
    AtomicValue<uint32> mObjectUpdatesPerSecond;

    typedef SubscriberIndex<ServerID> ServerSubscriberIndex;
    ServerSubscriberIndex mServerSubscriptions;

    typedef SubscriberIndex<OHDP::NodeID> OHSubscriberIndex;
    OHSubscriberIndex mOHSubscriptions;

    typedef SubscriberIndex<UUID> ObjectSubscriberIndex;
    ObjectSubscriberIndex mObjectSubscriptions;
}; // class AlwaysLocationUpdatePolicy

} // namespace Sirikata

#endif //_ALWAYS_LOCATION_UPDATE_POLICY_HPP_
