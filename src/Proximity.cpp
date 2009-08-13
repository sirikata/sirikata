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

namespace CBR {

Proximity::Proximity(ServerID sid, LocationService* locservice, MessageRouter* router, MessageDispatcher* dispatcher)
 : mID(sid),
   mLastTime(Time::null()),
   mLocService(locservice),
   mServerQueries(),
   mLocalLocCache(NULL),
   mServerQueryHandler(NULL),
   mObjectQueries(),
   mGlobalLocCache(NULL),
   mObjectQueryHandler(NULL),
   mRouter(router),
   mDispatcher(dispatcher)
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

    mDispatcher->registerMessageRecipient(MESSAGE_TYPE_SERVER_PROX_QUERY, this);
    mDispatcher->registerMessageRecipient(MESSAGE_TYPE_SERVER_PROX_RESULT, this);
}

Proximity::~Proximity() {
    mDispatcher->unregisterMessageRecipient(MESSAGE_TYPE_SERVER_PROX_QUERY, this);
    mDispatcher->unregisterMessageRecipient(MESSAGE_TYPE_SERVER_PROX_RESULT, this);

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
    cseg->addListener(this);

    TimedMotionVector3f loc;

    BoundingBoxList bboxes = cseg->serverRegion(mID);
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);

    BoundingSphere3f bounds = bbox.toBoundingSphere();
    SolidAngle minAngle(SolidAngle::Max / 1000); // FIXME
    // FIXME this assumes that ServerIDs are simple sequence of IDs
    for(ServerID sid = 1; sid <= cseg->numServers(); sid++) {
        if (sid == mID) continue;
        ServerProximityQueryMessage* msg = new ServerProximityQueryMessage(mID);
        msg->contents.set_action(CBR::Protocol::Prox::ServerQuery::AddOrUpdate);
        CBR::Protocol::Prox::ITimedMotionVector msg_loc = msg->contents.mutable_location();
        msg_loc.set_t(loc.updateTime());
        msg_loc.set_position(loc.position());
        msg_loc.set_position(loc.velocity());
        msg->contents.set_bounds(bounds);
        msg->contents.set_min_angle(minAngle.asFloat());
        mRouter->route(msg, sid);
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
            addQuery(source_server, qloc, prox_query_msg->contents.bounds(), SolidAngle(prox_query_msg->contents.min_angle()));
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
        //printf("result_msg\n");
    }
}

void Proximity::addQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa) {
    assert( mServerQueries.find(sid) == mServerQueries.end() );
    Query* q = mServerQueryHandler->registerQuery(loc, bounds, sa);
    mServerQueries[sid] = q;
}

void Proximity::removeQuery(ServerID sid) {
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
}

void Proximity::removeQuery(UUID obj) {
    ObjectQueryMap::iterator it = mObjectQueries.find(obj);
    if (it == mObjectQueries.end()) return;

    Query* q = it->second;
    mObjectQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
}

void Proximity::evaluate(const Time& t, std::queue<ProximityEventInfo>& events) {
/*
    if ( ((uint32)(t-Time(0)).seconds()) - ((uint32)(mLastTime-Time(0)).seconds()) > 0)
        printf("Objects: %d, Queries: %d -- Objects: %d, Queries: %d\n", mServerQueryHandler->numObjects(), mServerQueryHandler->numQueries(), mObjectQueryHandler->numObjects(), mObjectQueryHandler->numQueries());
*/
    mServerQueryHandler->tick(t);
    mObjectQueryHandler->tick(t);
    mLastTime = t;

    typedef std::deque<QueryEvent> QueryEventList;

    // Output query events for servers

    for(ServerQueryMap::iterator query_it = mServerQueries.begin(); query_it != mServerQueries.end(); query_it++) {
        ServerID sid = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        if (evts.empty()) continue;

        ServerProximityResultMessage* result_msg = new ServerProximityResultMessage(mID);
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
        mRouter->route(result_msg, sid);
    }

    // Output QueryEvents for objects
    for(ObjectQueryMap::iterator query_it = mObjectQueries.begin(); query_it != mObjectQueries.end(); query_it++) {
        UUID query_id = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        for(QueryEventList::iterator evt_it = evts.begin(); evt_it != evts.end(); evt_it++) {
            if (evt_it->type() == QueryEvent::Added)
                events.push(ProximityEventInfo(query_id, evt_it->id(), mGlobalLocCache->location(evt_it->id()), ProximityEventInfo::Entered));
            else
                events.push(ProximityEventInfo(query_id, evt_it->id(), ProximityEventInfo::Exited));
        }
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
