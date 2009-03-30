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
#include "MotionPath.hpp"
#include "ObjectFactory.hpp"

namespace CBR {

struct Event {
    static Event* read(std::istream& is, const ServerID& trace_server_id);

    Event()
     : time(0)
    {}
    virtual ~Event() {}

    Time time;
};

struct EventTimeComparator {
    bool operator()(const Event* lhs, const Event* rhs) const {
        return (lhs->time < rhs->time);
    }
};

struct ObjectEvent : public Event {
    UUID receiver;
    UUID source;
};

struct ProximityEvent : public ObjectEvent {
    bool entered;
    TimedMotionVector3f loc;
};

struct LocationEvent : public ObjectEvent {
    TimedMotionVector3f loc;
};

struct SubscriptionEvent : public ObjectEvent {
    bool started;
};


struct PacketEvent : public Event {
    ServerID source;
    ServerID dest;
    uint32 id;
    uint32 size;
};

struct PacketQueuedEvent : public PacketEvent {
};

struct PacketSentEvent : public PacketEvent {
};

struct PacketReceivedEvent : public PacketEvent {
};

Event* Event::read(std::istream& is, const ServerID& trace_server_id) {
    char tag;
    is.read( &tag, sizeof(tag) );

    if (!is) return NULL;

    Event* evt = NULL;

    switch(tag) {
      case Trace::ProximityTag:
          {
              ProximityEvent* pevt = new ProximityEvent;
              is.read( (char*)&pevt->time, sizeof(pevt->time) );
              is.read( (char*)&pevt->receiver, sizeof(pevt->receiver) );
              is.read( (char*)&pevt->source, sizeof(pevt->source) );
              is.read( (char*)&pevt->entered, sizeof(pevt->entered) );
              is.read( (char*)&pevt->loc, sizeof(pevt->loc) );
              evt = pevt;
          }
          break;
      case Trace::LocationTag:
          {
              LocationEvent* levt = new LocationEvent;
              is.read( (char*)&levt->time, sizeof(levt->time) );
              is.read( (char*)&levt->receiver, sizeof(levt->receiver) );
              is.read( (char*)&levt->source, sizeof(levt->source) );
              is.read( (char*)&levt->loc, sizeof(levt->loc) );
              evt = levt;
          }
          break;
      case Trace::SubscriptionTag:
          {
              SubscriptionEvent* sevt = new SubscriptionEvent;
              is.read( (char*)&sevt->time, sizeof(sevt->time) );
              is.read( (char*)&sevt->receiver, sizeof(sevt->receiver) );
              is.read( (char*)&sevt->source, sizeof(sevt->source) );
              is.read( (char*)&sevt->started, sizeof(sevt->started) );
              evt = sevt;
          }
          break;
      case Trace::PacketQueuedTag:
          {
              PacketQueuedEvent* pqevt = new PacketQueuedEvent;
              is.read( (char*)&pqevt->time, sizeof(pqevt->time) );
              pqevt->source = trace_server_id;
              is.read( (char*)&pqevt->dest, sizeof(pqevt->dest) );
              is.read( (char*)&pqevt->id, sizeof(pqevt->id) );
              is.read( (char*)&pqevt->size, sizeof(pqevt->size) );
              evt = pqevt;
          }
          break;
      case Trace::PacketSentTag:
          {
              PacketSentEvent* psevt = new PacketSentEvent;
              is.read( (char*)&psevt->time, sizeof(psevt->time) );
              psevt->source = trace_server_id;
              is.read( (char*)&psevt->dest, sizeof(psevt->dest) );
              is.read( (char*)&psevt->id, sizeof(psevt->id) );
              is.read( (char*)&psevt->size, sizeof(psevt->size) );
              evt = psevt;
          }
          break;
      case Trace::PacketReceivedTag:
          {
              PacketReceivedEvent* prevt = new PacketReceivedEvent;
              is.read( (char*)&prevt->time, sizeof(prevt->time) );
              is.read( (char*)&prevt->source, sizeof(prevt->source) );
              prevt->dest = trace_server_id;
              is.read( (char*)&prevt->id, sizeof(prevt->id) );
              is.read( (char*)&prevt->size, sizeof(prevt->size) );
              evt = prevt;
          }
          break;
      default:
        assert(false);
        break;
    }

    return evt;
}


LocationErrorAnalysis::LocationErrorAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;
            ObjectEvent* obj_evt = dynamic_cast<ObjectEvent*>(evt);
            if (obj_evt == NULL)
                continue;

            ObjectEventListMap::iterator it = mEventLists.find( obj_evt->receiver );
            if (it == mEventLists.end()) {
                mEventLists[ obj_evt->receiver ] = new EventList;
                it = mEventLists.find( obj_evt->receiver );
            }
            assert( it != mEventLists.end() );

            EventList* evt_list = it->second;
            evt_list->push_back(obj_evt);
        }
    }

    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), EventTimeComparator());
    }
}

LocationErrorAnalysis::~LocationErrorAnalysis() {
    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        for(EventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++)
            delete *events_it;
    }
}

bool LocationErrorAnalysis::observed(const UUID& observer, const UUID& seen) const {
    EventList* events = getEventList(observer);
    if (events == NULL) return false;

    // they were observed if both a proximity entered event and a location update were received
    bool found_prox_entered = false;
    bool found_loc = false;

    for(EventList::iterator event_it = events->begin(); event_it != events->end(); event_it++) {
        ProximityEvent* prox = dynamic_cast<ProximityEvent*>(*event_it);
        if (prox != NULL && prox->entered)
            found_prox_entered = true;

        LocationEvent* loc = dynamic_cast<LocationEvent*>(*event_it);
        if (loc != NULL)
            found_loc = true;
    }

    return (found_prox_entered && found_loc);
}

struct AlwaysUpdatePredicate {
    static float64 maxDist;
    bool operator()(const MotionVector3f& lhs, const MotionVector3f& rhs) const {
        return (lhs.position() - rhs.position()).length() > maxDist;
    }
};

static bool event_matches_prox_entered(ObjectEvent* evt) {
    ProximityEvent* prox = dynamic_cast<ProximityEvent*>(evt);
    return (prox != NULL && prox->entered);
}

static bool event_matches_prox_exited(ObjectEvent* evt) {
    ProximityEvent* prox = dynamic_cast<ProximityEvent*>(evt);
    return (prox != NULL && !prox->entered);
}

static bool event_matches_loc(ObjectEvent* evt) {
    LocationEvent* loc = dynamic_cast<LocationEvent*>(evt);
    return (loc != NULL);
}


#define APPARENT_ERROR 1

// Return the average error in the approximation of an object over its observed period, sampled at the given rate.
double LocationErrorAnalysis::averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate, ObjectFactory* obj_factory) const {
    /* In this method we run through all the updates, tracking the real path along the way.
     * The main loop iterates over all the updates received, focusing on those dealing with
     * the specified target object.  We have 2 states we can be in
     * 1) Searching for a proximity update indicating the object has entered our region,
     *    which will also contain the initial location update
     * 2) Sampling.  In this mode, we're progressing forward in time and sampling.  At each event,
     *    we stop to see if we need to update the prediction or if the object has exited our proximity.
     */
    enum Mode {
        SEARCHING_PROX,
        SAMPLING
    };

    assert( observed(observer, seen) );

    MotionPath* true_path = obj_factory->motion(seen);
    TimedMotionVector3f true_motion = true_path->initial();
    const TimedMotionVector3f* next_true_motion = true_path->nextUpdate(true_motion.time());

    EventList* events = getEventList(observer);
    TimedMotionVector3f pred_motion;

#if APPARENT_ERROR
    MotionPath* observer_path = obj_factory->motion(observer);
    TimedMotionVector3f observer_motion = observer_path->initial();
    const TimedMotionVector3f* next_observer_motion = observer_path->nextUpdate(observer_motion.time());
#endif

    Time cur_time(0);
    Mode mode = SEARCHING_PROX;
    double error_sum = 0.0;
    uint32 sample_count = 0;
    for(EventList::iterator main_it = events->begin(); main_it != events->end(); main_it++) {
        ObjectEvent* cur_event = *main_it;
        if (!(cur_event->source == seen)) continue;

        if (mode == SEARCHING_PROX) {
            if (event_matches_prox_entered(cur_event)) {
                ProximityEvent* prox = dynamic_cast<ProximityEvent*>(cur_event);
                pred_motion = prox->loc;
                cur_time = prox->time;
                mode = SAMPLING;
            }
        }
        else if (mode == SAMPLING) {
            Time end_sampling_time = cur_event->time;

            // sample up to the current time
            while( cur_time < end_sampling_time ) {
                // update the true motion vector if necessary, get true position
                while(next_true_motion != NULL && next_true_motion->time() < cur_time) {
                    true_motion = *next_true_motion;
                    next_true_motion = true_path->nextUpdate(true_motion.time());
                }
                Vector3f true_pos = true_motion.extrapolate(cur_time).position();

                // get the predicted position
                Vector3f pred_pos = pred_motion.extrapolate(cur_time).position();

#if APPARENT_ERROR
                // update the observer position if necessary
                while(next_observer_motion != NULL && next_observer_motion->time() < cur_time) {
                    observer_motion = *next_observer_motion;
                    next_observer_motion = observer_path->nextUpdate(observer_motion.time());
                }
                Vector3f observer_pos = observer_motion.extrapolate(cur_time).position();

                Vector3f to_true = (true_pos - observer_pos).normal();
                Vector3f to_pred = (pred_pos - observer_pos).normal();
                float dot_val = to_true.dot(to_pred);
                if (dot_val > 1.0f) dot_val = 1.0f;
                if (dot_val < 1.0f) dot_val = -1.0f;
                error_sum += acos(dot_val);
#else
                error_sum += (true_pos - pred_pos).length();
#endif
                sample_count++;

                cur_time += sampling_rate;
            }

            if (event_matches_prox_exited(cur_event))
                mode = SEARCHING_PROX;
            else if (event_matches_loc(cur_event)) {
                LocationEvent* loc = dynamic_cast<LocationEvent*>(cur_event);
                if (loc->loc.time() >= pred_motion.time())
                    pred_motion = loc->loc;
            }

            cur_time = end_sampling_time;
        }
    }

    return (sample_count == 0) ? 0 : error_sum / sample_count;
}

double LocationErrorAnalysis::globalAverageError(const Duration& sampling_rate, ObjectFactory* obj_factory) const {
    double total_error = 0.0;
    uint32 total_pairs = 0;
    for(ObjectFactory::iterator observer_it = obj_factory->begin(); observer_it != obj_factory->end(); observer_it++) {
        for(ObjectFactory::iterator seen_it = obj_factory->begin(); seen_it != obj_factory->end(); seen_it++) {
            if (*observer_it == *seen_it) continue;
            if (observed(*observer_it, *seen_it)) {
                double error = averageError(*observer_it, *seen_it, sampling_rate, obj_factory);
                total_error += error;
                total_pairs++;
            }
        }
    }
    return (total_pairs == 0) ? 0 : total_error / total_pairs;
}

LocationErrorAnalysis::EventList* LocationErrorAnalysis::getEventList(const UUID& observer) const {
    ObjectEventListMap::const_iterator event_lists_it = mEventLists.find(observer);
    if (event_lists_it == mEventLists.end()) return NULL;

    return event_lists_it->second;
}








BandwidthAnalysis::BandwidthAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;
            PacketEvent* packet_evt = dynamic_cast<PacketEvent*>(evt);
            if (packet_evt == NULL)
                continue;

            // put it in the source queue
            ServerEventListMap::iterator source_it = mEventLists.find( packet_evt->source );
            if (source_it == mEventLists.end()) {
                mEventLists[ packet_evt->source ] = new EventList;
                source_it = mEventLists.find( packet_evt->source );
            }
            assert( source_it != mEventLists.end() );

            EventList* evt_list = source_it->second;
            evt_list->push_back(packet_evt);

            // put it in the dest queue
            ServerEventListMap::iterator dest_it = mEventLists.find( packet_evt->dest );
            if (dest_it == mEventLists.end()) {
                mEventLists[ packet_evt->dest ] = new EventList;
                dest_it = mEventLists.find( packet_evt->dest );
            }
            assert( dest_it != mEventLists.end() );

            evt_list = dest_it->second;
            evt_list->push_back(packet_evt);
        }
    }

    for(ServerEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), EventTimeComparator());
    }

}

BandwidthAnalysis::~BandwidthAnalysis() {
    for(ServerEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        EventList* event_list = event_lists_it->second;
        for(EventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }
}

BandwidthAnalysis::EventList* BandwidthAnalysis::getEventList(const ServerID& server) const {
    ServerEventListMap::const_iterator event_lists_it = mEventLists.find(server);
    if (event_lists_it == mEventLists.end()) return NULL;

    return event_lists_it->second;
}

void BandwidthAnalysis::computeSendRate(const ServerID& sender, const ServerID& receiver) const {
    computeRate<PacketSentEvent>(sender, receiver, sender);
}

void BandwidthAnalysis::computeReceiveRate(const ServerID& sender, const ServerID& receiver) const {
    computeRate<PacketReceivedEvent>(sender, receiver, receiver);
}

void BandwidthAnalysis::computeWindowedSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time) const {
    computeWindowedRate<PacketSentEvent>(sender, receiver, sender, window, sample_rate, start_time, end_time);
}

void BandwidthAnalysis::computeWindowedReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time) const {
    computeWindowedRate<PacketReceivedEvent>(sender, receiver, receiver, window, sample_rate, start_time, end_time);
}

} // namespace CBR
