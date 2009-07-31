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
#include "ObjectFactory.hpp"

#include <algorithm>

#include <prox/BruteForceQueryHandler.hpp>
#include <prox/RTreeQueryHandler.hpp>

namespace CBR {

Proximity::Proximity(LocationService* locservice)
 : mLastTime(0),
   mServerQueries(),
   mLocalLocCache(NULL),
   mServerQueryHandler(NULL),
   mObjectQueries(),
   mGlobalLocCache(NULL),
   mObjectQueryHandler(NULL)
{
    // Server Queries
    mLocalLocCache = new CBRLocationServiceCache(locservice, false);
    mServerQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mServerQueryHandler->initialize(mLocalLocCache);

    // Object Queries
    mGlobalLocCache = new CBRLocationServiceCache(locservice, true);
    mObjectQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mObjectQueryHandler->initialize(mGlobalLocCache);

    locservice->addListener(this);
}

Proximity::~Proximity() {
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


void Proximity::addQuery(ServerID sid, SolidAngle sa) {
    assert( mServerQueries.find(sid) == mServerQueries.end() );
    Query* q = NULL;
     // FIXME Need to get location and bounds from somewhere.
     // mServerQueryHandler->registerQuery(location, bounds, sa);
    mServerQueries[sid] = q;
}

void Proximity::removeQuery(ServerID sid) {
    ServerQueryMap::iterator it = mServerQueries.find(sid);
    if (it == mServerQueries.end()) return;

    Query* q = it->second;
    mServerQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
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

    // FIXME
    // Output query events for servers

    // Output QueryEvents for objects
    typedef std::deque<QueryEvent> QueryEventList;
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

} // namespace CBR
