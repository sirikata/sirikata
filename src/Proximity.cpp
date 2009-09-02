/*  cbr
 *  Proximity.cpp
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

#include "Proximity.hpp"

#include <algorithm>

#include <prox/BruteForceQueryHandler.hpp>
#include <prox/RTreeQueryHandler.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

namespace CBR {

Proximity::Proximity(SpaceContext* ctx, LocationService* locservice)
 : mContext(ctx),
   mLocService(locservice),
   mCSeg(NULL),
   mServerQueries(),
   mLocalLocCache(NULL),
   mServerQueryHandler(NULL),
   mObjectQueries(),
   mGlobalLocCache(NULL),
   mObjectQueryHandler(NULL),
   mMinObjectQueryAngle(SolidAngle::Max)
{
    // Server Queries
    mLocalLocCache = new CBRLocationServiceCache(locservice, false);
    mServerQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mServerQueryHandler->initialize(mLocalLocCache);

    // Object Queries
    mGlobalLocCache = new CBRLocationServiceCache(locservice, true);
    mObjectQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mObjectQueryHandler->initialize(mGlobalLocCache);

    mLocService->addListener(this);

    mContext->dispatcher->registerMessageRecipient(MESSAGE_TYPE_SERVER_PROX_QUERY, this);
    mContext->dispatcher->registerMessageRecipient(MESSAGE_TYPE_SERVER_PROX_RESULT, this);
}

Proximity::~Proximity() {
    mContext->dispatcher->unregisterMessageRecipient(MESSAGE_TYPE_SERVER_PROX_QUERY, this);
    mContext->dispatcher->unregisterMessageRecipient(MESSAGE_TYPE_SERVER_PROX_RESULT, this);

    // Objects
    while(!mObjectQueries.empty())
        removeQuery( mObjectQueries.begin()->first );

    delete mObjectQueryHandler;
    delete mGlobalLocCache;

    // Servers
    while(!mServerQueries.empty())
        removeQuery( mServerQueries.begin()->first );

    delete mServerQueryHandler;
    delete mLocalLocCache;
}


void Proximity::initialize(CoordinateSegmentation* cseg) {
    mCSeg = cseg;

    mCSeg->addListener(this);

    sendQueryRequests();
}

void Proximity::sendQueryRequests() {
    TimedMotionVector3f loc;

    // FIXME avoid computing this so much
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id);
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);

    BoundingSphere3f bounds = bbox.toBoundingSphere();

    // FIXME this assumes that ServerIDs are simple sequence of IDs
    for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
        if (sid == mContext->id) continue;

        ServerProximityQueryMessage* msg = new ServerProximityQueryMessage(mContext->id);
        msg->contents.set_action(CBR::Protocol::Prox::ServerQuery::AddOrUpdate);
        CBR::Protocol::Prox::ITimedMotionVector msg_loc = msg->contents.mutable_location();
        msg_loc.set_t(loc.updateTime());
        msg_loc.set_position(loc.position());
        msg_loc.set_position(loc.velocity());
        msg->contents.set_bounds(bounds);
        msg->contents.set_min_angle(mMinObjectQueryAngle.asFloat());
        mContext->router->route(msg, sid);
    }
}

void Proximity::receiveMessage(Message* msg) {
    ServerProximityQueryMessage* prox_query_msg = dynamic_cast<ServerProximityQueryMessage*>(msg);
    if (prox_query_msg != NULL) {

        // FIXME we need to have a way to get the source server from the message without using this magic,
        // probably by saving the header and delivering it along with the message
        ServerID source_server = GetUniqueIDServerID(msg->id());

        if (prox_query_msg->contents.action() == CBR::Protocol::Prox::ServerQuery::AddOrUpdate) {
            assert(
                prox_query_msg->contents.has_location() &&
                prox_query_msg->contents.has_bounds() &&
                prox_query_msg->contents.has_min_angle()
            );

            CBR::Protocol::Prox::ITimedMotionVector msg_loc = prox_query_msg->contents.location();
            TimedMotionVector3f qloc(msg_loc.t(), MotionVector3f(msg_loc.position(), msg_loc.velocity()));
            SolidAngle minangle(prox_query_msg->contents.min_angle());

            updateQuery(source_server, qloc, prox_query_msg->contents.bounds(), minangle);
        }
        else if (prox_query_msg->contents.action() == CBR::Protocol::Prox::ServerQuery::Remove) {
            removeQuery(source_server);
        }
        else {
            assert(false);
        }
    }

    ServerProximityResultMessage* prox_result_msg = dynamic_cast<ServerProximityResultMessage*>(msg);
    if (prox_result_msg != NULL) {
        assert( prox_result_msg->contents.has_t() );
        Time t = prox_result_msg->contents.t();

        if (prox_result_msg->contents.addition_size() > 0) {
            for(uint32 idx = 0; idx < prox_result_msg->contents.addition_size(); idx++) {
                CBR::Protocol::Prox::ObjectAddition addition = prox_result_msg->contents.addition(idx);
                mLocService->addReplicaObject(
                    t,
                    addition.object(),
                    TimedMotionVector3f( addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()) ),
                    addition.bounds()
                );
            }
        }

        if (prox_result_msg->contents.removal_size() > 0) {
            for(uint32 idx = 0; idx < prox_result_msg->contents.removal_size(); idx++) {
                CBR::Protocol::Prox::ObjectRemoval removal = prox_result_msg->contents.removal(idx);
                mLocService->removeReplicaObject(t, removal.object());
            }
        }
    }
}

void Proximity::updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa) {
    ServerQueryMap::iterator it = mServerQueries.find(sid);

    if (it == mServerQueries.end()) {
        PROXLOG(debug,"Add server query from " << sid << ", min angle " << sa.asFloat());

        Query* q = mServerQueryHandler->registerQuery(loc, bounds, sa);
        mServerQueries[sid] = q;
    }
    else {
        PROXLOG(debug,"Update server query from " << sid << ", min angle " << sa.asFloat());

        Query* q = it->second;
        q->position(loc);
        q->bounds(bounds);
        q->angle(sa);
    }
}

void Proximity::removeQuery(ServerID sid) {
    PROXLOG(debug,"Remove server query from " << sid);

    ServerQueryMap::iterator it = mServerQueries.find(sid);
    if (it == mServerQueries.end()) return;

    Query* q = it->second;
    mServerQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.

    // Remove all location update subscriptions for this server
    mLocService->unsubscribe(sid);
}


void Proximity::addQuery(UUID obj, SolidAngle sa) {
    assert( mObjectQueries.find(obj) == mObjectQueries.end() );
    Query* q = mObjectQueryHandler->registerQuery(mLocalLocCache->location(obj), mLocalLocCache->bounds(obj), sa);
    mObjectQueries[obj] = q;

    // Update min query angle, and update remote queries if necessary
    if (sa < mMinObjectQueryAngle) {
        mMinObjectQueryAngle = sa;
        PROXLOG(debug,"Query removal initiated server query request.");
        sendQueryRequests();
    }
}

void Proximity::removeQuery(UUID obj) {
    ObjectQueryMap::iterator it = mObjectQueries.find(obj);
    if (it == mObjectQueries.end()) return;

    Query* q = it->second;
    SolidAngle sa = q->angle();
    mObjectQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.

    // Update min query angle, and update remote queries if necessary
    if (sa == mMinObjectQueryAngle) {
        for(ObjectQueryMap::iterator it = mObjectQueries.begin(); it != mObjectQueries.end(); it++)
            if (it->second->angle() < mMinObjectQueryAngle) mMinObjectQueryAngle = it->second->angle();
        PROXLOG(debug,"Query removal initiated server query request.");
        sendQueryRequests();
    }
}

void Proximity::service() {
/*
    if ( ((uint32)(t-Time(0)).seconds()) - ((uint32)(mLastTime-Time(0)).seconds()) > 0)
        printf("Objects: %d, Queries: %d -- Objects: %d, Queries: %d\n", mServerQueryHandler->numObjects(), mServerQueryHandler->numQueries(), mObjectQueryHandler->numObjects(), mObjectQueryHandler->numQueries());
*/
    Time t = mContext->time;
    mServerQueryHandler->tick(t);
    mObjectQueryHandler->tick(t);

    typedef std::deque<QueryEvent> QueryEventList;

    // Output query events for servers

    for(ServerQueryMap::iterator query_it = mServerQueries.begin(); query_it != mServerQueries.end(); query_it++) {
        ServerID sid = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        if (evts.empty()) continue;

        ServerProximityResultMessage* result_msg = new ServerProximityResultMessage(mContext->id);
        result_msg->contents.set_t(t);
        for(QueryEventList::iterator evt_it = evts.begin(); evt_it != evts.end(); evt_it++) {
            if (evt_it->type() == QueryEvent::Added) {
                mLocService->subscribe(sid, evt_it->id());

                CBR::Protocol::Prox::IObjectAddition addition = result_msg->contents.add_addition();
                addition.set_object( evt_it->id() );

                TimedMotionVector3f loc = mLocalLocCache->location(evt_it->id());
                CBR::Protocol::Prox::ITimedMotionVector msg_loc = addition.mutable_location();
                msg_loc.set_t(loc.updateTime());
                msg_loc.set_position(loc.position());
                msg_loc.set_velocity(loc.velocity());

                addition.set_bounds( mLocalLocCache->bounds(evt_it->id()) );
            }
            else {
                mLocService->unsubscribe(sid, evt_it->id());
                CBR::Protocol::Prox::IObjectRemoval removal = result_msg->contents.add_removal();
                removal.set_object( evt_it->id() );
            }
        }

        PROXLOG(insane,"Reporting " << result_msg->contents.addition_size() << " additions, " << result_msg->contents.removal_size() << " removals to server " << sid);

        mContext->router->route(result_msg, sid);
    }

    // Output QueryEvents for objects
    for(ObjectQueryMap::iterator query_it = mObjectQueries.begin(); query_it != mObjectQueries.end(); query_it++) {
        UUID query_id = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        CBR::Protocol::Prox::ProximityResults prox_results;
        prox_results.set_t(mContext->time);
        for(QueryEventList::iterator evt_it = evts.begin(); evt_it != evts.end(); evt_it++) {
            if (evt_it->type() == QueryEvent::Added) {
                CBR::Protocol::Prox::IObjectAddition addition = prox_results.add_addition();
                addition.set_object( evt_it->id() );

                CBR::Protocol::Prox::ITimedMotionVector motion = addition.mutable_location();
                TimedMotionVector3f loc = mLocService->location(evt_it->id());
                motion.set_t(loc.updateTime());
                motion.set_position(loc.position());
                motion.set_velocity(loc.velocity());

                addition.set_bounds( mLocService->bounds(evt_it->id()) );
            }
            else {
                CBR::Protocol::Prox::IObjectRemoval removal = prox_results.add_removal();
                removal.set_object( evt_it->id() );
            }
        }

        CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
        obj_msg->set_source_object(UUID::null());
        obj_msg->set_source_port(OBJECT_PORT_PROXIMITY);
        obj_msg->set_dest_object(query_id);
        obj_msg->set_dest_port(OBJECT_PORT_PROXIMITY);
        obj_msg->set_unique(GenerateUniqueID(mContext->id));
        obj_msg->set_payload( serializePBJMessage(prox_results) );

        mContext->router->route(obj_msg, false);
    }
}

void Proximity::queryHasEvents(Query* query) {
    // Currently we don't use this directly, we just always iterate over all queries and check them.
    // FIXME we could store this information and only check the ones we get callbacks for here
}


// Note: LocationServiceListener interface is only used in order to get updates on objects which have
// registered queries, allowing us to update those queries as appropriate.  All updating of objects
// in the prox data structure happens via the LocationServiceCache
void Proximity::localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
}
void Proximity::localObjectRemoved(const UUID& uuid) {
}
void Proximity::localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    ObjectQueryMap::iterator it = mObjectQueries.find(uuid);
    if (it == mObjectQueries.end()) return;

    Query* query = it->second;
    query->position(newval);
}
void Proximity::localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    ObjectQueryMap::iterator it = mObjectQueries.find(uuid);
    if (it == mObjectQueries.end()) return;

    Query* query = it->second;
    query->bounds(newval);
}
void Proximity::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
}
void Proximity::replicaObjectRemoved(const UUID& uuid) {
}
void Proximity::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
}
void Proximity::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
}

void Proximity::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg) {
}

} // namespace CBR
