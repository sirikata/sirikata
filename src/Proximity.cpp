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

namespace CBR {

Proximity::Proximity() {
}

Proximity::~Proximity() {
    while(!mQueries.empty())
        removeQuery( mQueries.begin()->first );

    while(!mTrackedObjects.empty())
        removeObject( mTrackedObjects.begin()->first );
}

void Proximity::addObject(UUID obj, const MotionVector3f& loc) {
    assert(mTrackedObjects.find(obj) == mTrackedObjects.end());

    ObjectState* objstate = new ObjectState();
    objstate->location = loc;

    mTrackedObjects[obj] = objstate;
}

void Proximity::updateObject(UUID obj, const MotionVector3f& loc) {
    ObjectMap::iterator it = mTrackedObjects.find(obj);
    assert(it != mTrackedObjects.end());

    ObjectState* objstate = it->second;
    assert(objstate != NULL);
    objstate->location = loc;
}

void Proximity::removeObject(UUID obj) {
    ObjectMap::iterator it = mTrackedObjects.find(obj);
    assert(it != mTrackedObjects.end());

    ObjectState* objstate = it->second;
    delete objstate;

    mTrackedObjects.erase(it);
}

void Proximity::addQuery(UUID obj, float radius) {
    assert( mQueries.find(obj) == mQueries.end() );
    QueryState* qs = new QueryState;
    qs->radius = radius;
    mQueries[obj] = qs;
}

void Proximity::removeQuery(UUID obj) {
    QueryMap::iterator it = mQueries.find(obj);
    assert(it != mQueries.end());

    QueryState* qs = it->second;
    delete qs;

    mQueries.erase(it);
}

void Proximity::evaluate(const Time& t, std::queue<ProximityEvent>& events) {
    for(QueryMap::iterator query_it = mQueries.begin(); query_it != mQueries.end(); query_it++) {
        UUID query_id = query_it->first;
        QueryState* query_state = query_it->second;
        Vector3f query_pos = mTrackedObjects[query_id]->location.position(t);

        // generate new neighbor set
        ObjectSet new_neighbors;
        for(ObjectMap::iterator object_it = mTrackedObjects.begin(); object_it != mTrackedObjects.end(); object_it++) {
            UUID obj_id = object_it->first;
            ObjectState* obj_state = object_it->second;
            if (obj_id == query_id) continue;

            Vector3f obj_pos = obj_state->location.position(t);
            if ( (query_pos - obj_pos).lengthSquared() < query_state->radius*query_state->radius )
                new_neighbors.insert(obj_id);
        }

        // generate difference set
        ObjectSet added_objs;
        std::set_difference(
            new_neighbors.begin(), new_neighbors.end(),
            query_state->neighbors.begin(), query_state->neighbors.end(),
            std::inserter(added_objs, added_objs.begin())
        );

        ObjectSet removed_objs;
        std::set_difference(
            query_state->neighbors.begin(), query_state->neighbors.end(),
            new_neighbors.begin(), new_neighbors.end(),
            std::inserter(removed_objs, removed_objs.begin())
        );


        for(ObjectSet::iterator it = added_objs.begin(); it != added_objs.end(); it++)
            events.push(ProximityEvent(query_id, *it, ProximityEvent::Entered));

        for(ObjectSet::iterator it = removed_objs.begin(); it != removed_objs.end(); it++)
            events.push(ProximityEvent(query_id, *it, ProximityEvent::Exited));

        // update to new neighbor list
        query_state->neighbors = new_neighbors;
    }
}


} // namespace CBR
