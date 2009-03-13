/*  cbr
 *  Analysis.cpp
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

#include "Analysis.hpp"
#include "Statistics.hpp"
#include "Options.hpp"

namespace CBR {

struct ObjectEvent {
    static ObjectEvent* read(std::istream& is);

    ObjectEvent()
     : time(0)
    {}

    Time time;
    UUID receiver;
    UUID source;
};

struct ObjectEventTimeComparator {
    bool operator()(const ObjectEvent* lhs, const ObjectEvent* rhs) const {
        return (lhs->time < rhs->time);
    }
};

struct ProximityEvent : public ObjectEvent {
    bool entered;
};

struct LocationEvent : public ObjectEvent {
    TimedMotionVector3f loc;
};

struct SubscriptionEvent : public ObjectEvent {
    bool started;
};


ObjectEvent* ObjectEvent::read(std::istream& is) {
    char tag;
    is.read( &tag, sizeof(tag) );

    if (!is) return NULL;

    ObjectEvent* evt = NULL;

    if (tag == ObjectTrace::ProximityTag) {
        ProximityEvent* pevt = new ProximityEvent;
        is.read( (char*)&pevt->time, sizeof(pevt->time) );
        is.read( (char*)&pevt->receiver, sizeof(pevt->receiver) );
        is.read( (char*)&pevt->source, sizeof(pevt->source) );
        is.read( (char*)&pevt->entered, sizeof(pevt->entered) );
        evt = pevt;
    }
    else if (tag == ObjectTrace::LocationTag) {
        LocationEvent* levt = new LocationEvent;
        is.read( (char*)&levt->time, sizeof(levt->time) );
        is.read( (char*)&levt->receiver, sizeof(levt->receiver) );
        is.read( (char*)&levt->source, sizeof(levt->source) );
        is.read( (char*)&levt->loc, sizeof(levt->loc) );
        evt = levt;
    }
    else if (tag == ObjectTrace::SubscriptionTag) {
        SubscriptionEvent* sevt = new SubscriptionEvent;
        is.read( (char*)&sevt->time, sizeof(sevt->time) );
        is.read( (char*)&sevt->receiver, sizeof(sevt->receiver) );
        is.read( (char*)&sevt->source, sizeof(sevt->source) );
        is.read( (char*)&sevt->started, sizeof(sevt->started) );
        evt = sevt;
    }
    else {
        assert(false);
    }

    return evt;
}


LocationErrorAnalysis::LocationErrorAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            ObjectEvent* evt = ObjectEvent::read(is);
            if (evt == NULL)
                break;

            ObjectEventListMap::iterator it = mEventLists.find( evt->receiver );
            if (it == mEventLists.end()) {
                mEventLists[ evt->receiver ] = new EventList;
                it = mEventLists.find( evt->receiver );
            }
            assert( it != mEventLists.end() );

            EventList* evt_list = it->second;
            evt_list->push_back(evt);
        }
    }

    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), ObjectEventTimeComparator());
    }
}

LocationErrorAnalysis::~LocationErrorAnalysis() {
    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        for(EventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++)
            delete *events_it;
    }
}


} // namespace CBR
