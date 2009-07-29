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

Proximity::Proximity(ObjectFactory* objfactory, LocationService* locservice)
 : mLastTime(0),
   mObjectFactory(objfactory),
   mLocCache(NULL),
   mHandler(NULL)
{
    mLocCache = new CBRLocationServiceCache(locservice);
    Prox::BruteForceQueryHandler<ProxSimulationTraits>* bhandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mHandler = bhandler;
    mHandler->initialize(mLocCache);
}

Proximity::~Proximity() {
    while(!mQueries.empty())
        removeQuery( mQueries.begin()->first );

    delete mHandler;
}

void Proximity::addQuery(UUID obj, const TimedMotionVector3f& loc, SolidAngle sa) {
    assert( mQueries.find(obj) == mQueries.end() );
    Query* q = mHandler->registerQuery(loc, sa);
    mQueries[obj] = q;
}

void Proximity::removeQuery(UUID obj) {
    QueryMap::iterator it = mQueries.find(obj);
    assert(it != mQueries.end());

    Query* q = it->second;
    mQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
}

void Proximity::evaluate(const Time& t, std::queue<ProximityEventInfo>& events) {
    if ( ((uint32)(t-Time(0)).seconds()) - ((uint32)(mLastTime-Time(0)).seconds()) > 0)
        printf("Objects: %d, Queries: %d\n", mHandler->numObjects(), mHandler->numQueries());
    mHandler->tick(t);
    mLastTime = t;

    typedef std::deque<QueryEvent> QueryEventList;

    for(QueryMap::iterator query_it = mQueries.begin(); query_it != mQueries.end(); query_it++) {
        UUID query_id = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        for(QueryEventList::iterator evt_it = evts.begin(); evt_it != evts.end(); evt_it++) {
            if (evt_it->type() == QueryEvent::Added)
                events.push(ProximityEventInfo(query_id, evt_it->id(), mLocCache->location(evt_it->id()), ProximityEventInfo::Entered));
            else
                events.push(ProximityEventInfo(query_id, evt_it->id(), ProximityEventInfo::Exited));
        }
    }
}

void Proximity::queryHasEvents(Query* query) {
    // Currently we don't use this directly, we just always iterate over all queries and check them.
    // FIXME we could store this information and only check the ones we get callbacks for here
}

} // namespace CBR
