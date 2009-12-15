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
#include "AnalysisEvents.hpp"
#include "Utility.hpp"
#include "RecordedMotionPath.hpp"
#include "OSegLookupTraceToken.hpp"
#include <algorithm>

namespace CBR {



Event* Event::read(std::istream& is, const ServerID& trace_server_id) {
    char tag;
    is.read( &tag, sizeof(tag) );

    if (!is)
    {
      return NULL;
    }

    Event* evt = NULL;

    switch(tag) {
      case Trace::SegmentationChangeTag:
          {
  	      SegmentationChangeEvent* sevt = new SegmentationChangeEvent;
	      is.read( (char*)&sevt->time, sizeof(sevt->time) );
	      is.read( (char*)&sevt->bbox, sizeof(sevt->bbox) );
	      is.read( (char*)&sevt->server, sizeof(sevt->server) );
	      evt = sevt;
          }
	  break;
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
      case Trace::ObjectLocationTag:
          {
              LocationEvent* levt = new LocationEvent;
              is.read( (char*)&levt->time, sizeof(levt->time) );
              is.read( (char*)&levt->receiver, sizeof(levt->receiver) );
              is.read( (char*)&levt->source, sizeof(levt->source) );
              is.read( (char*)&levt->loc, sizeof(levt->loc) );
              evt = levt;
          }
          break;
      case Trace::ObjectGeneratedLocationTag:
          {
              GeneratedLocationEvent* levt = new GeneratedLocationEvent;
              is.read( (char*)&levt->time, sizeof(levt->time) );
              levt->receiver = UUID::null();
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
      case Trace::ObjectPingTag:
          {
              PingEvent *pevt = new PingEvent;
              is.read((char*)&pevt->sentTime, sizeof(pevt->sentTime));
              is.read((char*)&pevt->source, sizeof(pevt->source));
              is.read((char*)&pevt->time, sizeof(pevt->time));
              is.read((char*)&pevt->receiver, sizeof(pevt->receiver));
              is.read((char*)&pevt->id,sizeof(pevt->id));
              is.read((char*)&pevt->distance,sizeof(pevt->distance));
              is.read((char*)&pevt->uid,sizeof(pevt->uid));
              evt=pevt;
          }
        break;
      case Trace::MessageTimestampTag:
          {
              MessageTimestampEvent *pevt = new MessageTimestampEvent;
              is.read((char*)&pevt->time, sizeof(pevt->time));
              is.read((char*)&pevt->uid, sizeof(pevt->uid));
              is.read((char*)&pevt->path, sizeof(pevt->path));
              is.read((char*)&pevt->srcport, sizeof(pevt->srcport));
              is.read((char*)&pevt->dstport,sizeof(pevt->dstport));
              evt=pevt;
          }
        break;
      case Trace::ServerDatagramQueueInfoTag:
          {
              ServerDatagramQueueInfoEvent* pqievt = new ServerDatagramQueueInfoEvent;
              is.read( (char*)&pqievt->time, sizeof(pqievt->time) );
              pqievt->source = trace_server_id;
              is.read( (char*)&pqievt->dest, sizeof(pqievt->dest) );
              is.read( (char*)&pqievt->send_size, sizeof(pqievt->send_size) );
              is.read( (char*)&pqievt->send_queued, sizeof(pqievt->send_queued) );
              is.read( (char*)&pqievt->send_weight, sizeof(pqievt->send_weight) );
              is.read( (char*)&pqievt->receive_size, sizeof(pqievt->receive_size) );
              is.read( (char*)&pqievt->receive_queued, sizeof(pqievt->receive_queued) );
              is.read( (char*)&pqievt->receive_weight, sizeof(pqievt->receive_weight) );
              evt = pqievt;
          }
          break;
      case Trace::ServerDatagramQueuedTag:
          {
              ServerDatagramQueuedEvent* pqevt = new ServerDatagramQueuedEvent;
              is.read( (char*)&pqevt->time, sizeof(pqevt->time) );
              pqevt->source = trace_server_id;
              is.read( (char*)&pqevt->dest, sizeof(pqevt->dest) );
              is.read( (char*)&pqevt->id, sizeof(pqevt->id) );
              is.read( (char*)&pqevt->size, sizeof(pqevt->size) );
              evt = pqevt;
          }
          break;

      case Trace::OSegCumulativeTraceAnalysisTag:
        {
          OSegCumulativeEvent* cumevt = new OSegCumulativeEvent;
          is.read((char*)&cumevt->time, sizeof(cumevt->time));
          is.read((char*)&cumevt->traceToken, sizeof(cumevt->traceToken));
          evt = cumevt;
        }
        break;
      case Trace::ServerDatagramSentTag:
          {
              ServerDatagramSentEvent* psevt = new ServerDatagramSentEvent;
              is.read( (char*)&psevt->time, sizeof(psevt->time) );
              psevt->source = trace_server_id;
              is.read( (char*)&psevt->dest, sizeof(psevt->dest) );
              is.read( (char*)&psevt->id, sizeof(psevt->id) );
              is.read( (char*)&psevt->size, sizeof(psevt->size) );
              is.read( (char*)&psevt->weight, sizeof(psevt->weight) );
              is.read( (char*)&psevt->_start_time, sizeof(psevt->_start_time) );
              is.read( (char*)&psevt->_end_time, sizeof(psevt->_end_time) );
              evt = psevt;
          }
          break;
      case Trace::ServerDatagramReceivedTag:
          {
              ServerDatagramReceivedEvent* prevt = new ServerDatagramReceivedEvent;
              is.read( (char*)&prevt->time, sizeof(prevt->time) );
              is.read( (char*)&prevt->source, sizeof(prevt->source) );
              prevt->dest = trace_server_id;
              is.read( (char*)&prevt->id, sizeof(prevt->id) );
              is.read( (char*)&prevt->size, sizeof(prevt->size) );
              is.read( (char*)&prevt->_start_time, sizeof(prevt->_start_time) );
              is.read( (char*)&prevt->_end_time, sizeof(prevt->_end_time) );
              evt = prevt;
          }
          break;
      case Trace::PacketQueueInfoTag:
          {
              PacketQueueInfoEvent* pqievt = new PacketQueueInfoEvent;
              is.read( (char*)&pqievt->time, sizeof(pqievt->time) );
              pqievt->source = trace_server_id;
              is.read( (char*)&pqievt->dest, sizeof(pqievt->dest) );
              is.read( (char*)&pqievt->send_size, sizeof(pqievt->send_size) );
              is.read( (char*)&pqievt->send_queued, sizeof(pqievt->send_queued) );
              is.read( (char*)&pqievt->send_weight, sizeof(pqievt->send_weight) );
              is.read( (char*)&pqievt->receive_size, sizeof(pqievt->receive_size) );
              is.read( (char*)&pqievt->receive_queued, sizeof(pqievt->receive_queued) );
              is.read( (char*)&pqievt->receive_weight, sizeof(pqievt->receive_weight) );
              evt = pqievt;
          }
          break;
      case Trace::PacketSentTag:
          {
              PacketSentEvent* psevt = new PacketSentEvent;
              is.read( (char*)&psevt->time, sizeof(psevt->time) );
              psevt->source = trace_server_id;
              is.read( (char*)&psevt->dest, sizeof(psevt->dest) );
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
              is.read( (char*)&prevt->size, sizeof(prevt->size) );
              evt = prevt;
          }
          break;
      case Trace::ObjectBeginMigrateTag:
        {
          ObjectBeginMigrateEvent* objBegMig_evt = new ObjectBeginMigrateEvent;
          is.read( (char*)&objBegMig_evt->time, sizeof(objBegMig_evt->time) );
          is.read((char*) & objBegMig_evt->mObjID, sizeof(objBegMig_evt->mObjID));
          is.read((char*) & objBegMig_evt->mMigrateFrom, sizeof(objBegMig_evt->mMigrateFrom));
          is.read((char*) & objBegMig_evt->mMigrateTo, sizeof(objBegMig_evt->mMigrateTo));

          evt = objBegMig_evt;
        }
        break;


      case Trace::ObjectAcknowledgeMigrateTag:
        {
          ObjectAcknowledgeMigrateEvent* objAckMig_evt = new ObjectAcknowledgeMigrateEvent;
          is.read( (char*)&objAckMig_evt->time, sizeof(objAckMig_evt->time) );

          is.read((char*) &objAckMig_evt->mObjID, sizeof(objAckMig_evt->mObjID) );

          is.read((char*) & objAckMig_evt->mAcknowledgeFrom, sizeof(objAckMig_evt->mAcknowledgeFrom));
          is.read((char*) & objAckMig_evt->mAcknowledgeTo, sizeof(objAckMig_evt->mAcknowledgeTo));

          evt = objAckMig_evt;
        }
        break;

      case Trace::RoundTripMigrationTimeAnalysisTag:
        {
          ObjectMigrationRoundTripEvent* rdTripMig_evt = new ObjectMigrationRoundTripEvent;
          is.read((char*)&rdTripMig_evt->time,sizeof(rdTripMig_evt->time));
          is.read((char*)&rdTripMig_evt->obj_id,sizeof(rdTripMig_evt->obj_id));
          is.read((char*)&rdTripMig_evt->sID_migratingFrom, sizeof(rdTripMig_evt->sID_migratingFrom));
          is.read((char*)&rdTripMig_evt->sID_migratingTo,sizeof(rdTripMig_evt->sID_migratingTo));
          is.read((char*)&rdTripMig_evt->numMill,sizeof(rdTripMig_evt->numMill));

          evt = rdTripMig_evt;
        }
        break;

      case Trace::OSegTrackedSetResultAnalysisTag:
        {
          OSegTrackedSetResultsEvent* trackedSetResults_evt = new OSegTrackedSetResultsEvent;
          is.read((char*)&trackedSetResults_evt->time, sizeof(trackedSetResults_evt->time));
          is.read((char*)&trackedSetResults_evt->obj_id, sizeof(trackedSetResults_evt->obj_id));
          is.read((char*)&trackedSetResults_evt->sID_migratingTo, sizeof(trackedSetResults_evt->sID_migratingTo));
          is.read((char*)&trackedSetResults_evt->numMill, sizeof(trackedSetResults_evt->numMill));

          evt = trackedSetResults_evt;
        }

        break;

      case Trace::OSegCacheResponseTag:
        {
          OSegCacheResponseEvent* cachedResponse_evt = new OSegCacheResponseEvent;
          is.read((char*)&cachedResponse_evt->time,sizeof(cachedResponse_evt->time));
          is.read((char*)&cachedResponse_evt->cacheResponseID, sizeof(cachedResponse_evt->cacheResponseID));
          is.read((char*)&cachedResponse_evt->obj_id, sizeof(cachedResponse_evt->obj_id));
          evt = cachedResponse_evt;
        }
        break;

      case Trace::OSegShutdownEventTag:
        {
          OSegShutdownEvent* shutdown_evt = new OSegShutdownEvent;
          is.read((char*)&shutdown_evt->time, sizeof(shutdown_evt->time));
          is.read((char*)&shutdown_evt->sID, sizeof(shutdown_evt->sID));
          is.read((char*)&shutdown_evt->numLookups, sizeof(shutdown_evt->numLookups));
          is.read((char*)&shutdown_evt->numOnThisServer,sizeof(shutdown_evt->numOnThisServer));
          is.read((char*)&shutdown_evt->numCacheHits, sizeof(shutdown_evt->numCacheHits));
          is.read((char*)&shutdown_evt->numCraqLookups, sizeof(shutdown_evt->numCraqLookups));
          is.read((char*)&shutdown_evt->numTimeElapsedCacheEviction, sizeof(shutdown_evt->numTimeElapsedCacheEviction));
          is.read((char*)&shutdown_evt->numMigrationNotCompleteYet, sizeof(shutdown_evt->numMigrationNotCompleteYet));

          evt = shutdown_evt;

        }
        break;

      case Trace::ServerLocationTag:
          {
              ServerLocationEvent* levt = new ServerLocationEvent;
              is.read( (char*)&levt->time, sizeof(levt->time) );
              is.read( (char*)&levt->source, sizeof(levt->source) );
              is.read( (char*)&levt->dest, sizeof(levt->dest) );
              is.read( (char*)&levt->object, sizeof(levt->object) );
              is.read( (char*)&levt->loc, sizeof(levt->loc) );
              evt = levt;
          }
          break;
      case Trace::ServerObjectEventTag:
          {
              ServerObjectEventEvent* levt = new ServerObjectEventEvent;
              is.read( (char*)&levt->time, sizeof(levt->time) );
              is.read( (char*)&levt->source, sizeof(levt->source) );
              is.read( (char*)&levt->dest, sizeof(levt->dest) );
              is.read( (char*)&levt->object, sizeof(levt->object) );
              uint8 raw_added = 0;
              is.read( (char*)&raw_added, sizeof(raw_added) );
              levt->added = (raw_added > 0);
              is.read( (char*)&levt->loc, sizeof(levt->loc) );
              evt = levt;
          }
          break;

      case Trace::ObjectSegmentationCraqLookupRequestAnalysisTag:
        {
          ObjectCraqLookupEvent* obj_lookupReq_evt = new ObjectCraqLookupEvent;
          is.read( (char*)&obj_lookupReq_evt->time, sizeof(obj_lookupReq_evt->time)  );
          is.read( (char*)&obj_lookupReq_evt->mObjID,sizeof(obj_lookupReq_evt->mObjID)  );
          is.read( (char*)&obj_lookupReq_evt->mID_lookup, sizeof(obj_lookupReq_evt->mID_lookup) );

          evt = obj_lookupReq_evt;
        }
        break;

      case Trace::OSegLookupNotOnServerAnalysisTag:
        {
          ObjectLookupNotOnServerEvent* obj_lookup_not_on_server_evt = new ObjectLookupNotOnServerEvent;
          is.read( (char*)&obj_lookup_not_on_server_evt->time, sizeof(obj_lookup_not_on_server_evt->time)  );
          is.read( (char*)&obj_lookup_not_on_server_evt->mObjID,sizeof(obj_lookup_not_on_server_evt->mObjID)  );
          is.read( (char*)&obj_lookup_not_on_server_evt->mID_lookup, sizeof(obj_lookup_not_on_server_evt->mID_lookup) );

          evt = obj_lookup_not_on_server_evt;
        }
        break;

      case Trace::ObjectSegmentationProcessedRequestAnalysisTag:
        {
          ObjectLookupProcessedEvent* obj_lookupProc_evt = new ObjectLookupProcessedEvent;
          is.read( (char* )&obj_lookupProc_evt->time, sizeof(obj_lookupProc_evt->time)  );
          is.read( (char* )&obj_lookupProc_evt->mObjID, sizeof(obj_lookupProc_evt->mObjID));
          is.read( (char* )&obj_lookupProc_evt->mID_processor, sizeof(obj_lookupProc_evt->mID_processor) );
          is.read( (char* )&obj_lookupProc_evt->mID_objectOn, sizeof(obj_lookupProc_evt->mID_objectOn) );
          is.read( (char* )&obj_lookupProc_evt->deltaTime, sizeof(obj_lookupProc_evt->deltaTime) );
          is.read( (char* )&obj_lookupProc_evt->stillInQueue, sizeof(obj_lookupProc_evt->stillInQueue) );
          evt = obj_lookupProc_evt;
        }
        break;


      default:

        std::cout<<"\n*****I got an unknown tag in analysis.cpp.  Value:  "<<(uint32)tag<<"\n";
        assert(false);
        break;
    }

    return evt;
}




template<typename EventType, typename IteratorType>
class EventIterator {
public:
    typedef EventType event_type;
    typedef IteratorType iterator_type;

    EventIterator(const ServerID& sender, const ServerID& receiver, IteratorType begin, IteratorType end)
     : mSender(sender), mReceiver(receiver), mRangeCurrent(begin), mRangeEnd(end)
    {
        if (mRangeCurrent != mRangeEnd && !currentMatches())
            next();
    }

    EventType* current() {
        if (mRangeCurrent == mRangeEnd)
            return NULL;

        EventType* event = dynamic_cast<EventType*>(*mRangeCurrent);
        assert(event != NULL);
        return event;
    }

    EventType* next() {
        if (mRangeCurrent == mRangeEnd) return NULL;
        do {
            mRangeCurrent++;
        } while(mRangeCurrent != mRangeEnd && !currentMatches());
        return current();
    }

private:
    bool currentMatches() {
        EventType* event = dynamic_cast<EventType*>(*mRangeCurrent);
        if (event == NULL) return false;
        if (event->source != mSender || event->dest != mReceiver) return false;
        return true;
    }

    ServerID mSender;
    ServerID mReceiver;
    IteratorType mRangeCurrent;
    IteratorType mRangeEnd;
};


template<typename EventListType, typename EventListMapType>
void sort_events(EventListMapType& lists) {
    for(typename EventListMapType::iterator event_lists_it = lists.begin(); event_lists_it != lists.end(); event_lists_it++) {
        EventListType* event_list = event_lists_it->second;
        std::sort(event_list->begin(), event_list->end(), EventTimeComparator());
    }
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
            ProximityEvent* pe = dynamic_cast<ProximityEvent*>(evt);
            LocationEvent* le = dynamic_cast<LocationEvent*>(evt);
            ServerObjectEventEvent* sobj_evt = dynamic_cast<ServerObjectEventEvent*>(evt);
            ServerLocationEvent* sloc_evt = dynamic_cast<ServerLocationEvent*>(evt);

            if (obj_evt != NULL && (pe != NULL || le != NULL)) {
                ObjectEventListMap::iterator it = mEventLists.find( obj_evt->receiver );
                if (it == mEventLists.end()) {
                    mEventLists[ obj_evt->receiver ] = new EventList;
                    it = mEventLists.find( obj_evt->receiver );
                }
                assert( it != mEventLists.end() );

                EventList* evt_list = it->second;
                evt_list->push_back(obj_evt);
            }
            else if (sobj_evt != NULL) {
                ServerEventListMap::iterator it = mServerEventLists.find( sobj_evt->dest );
                if (it == mServerEventLists.end()) {
                    mServerEventLists[ sobj_evt->dest ] = new EventList;
                    it = mServerEventLists.find( sobj_evt->dest );
                }
                assert( it != mServerEventLists.end() );

                EventList* evt_list = it->second;
                evt_list->push_back(sobj_evt);
            }
            else if (sloc_evt != NULL) {
                ServerEventListMap::iterator it = mServerEventLists.find( sloc_evt->dest );
                if (it == mServerEventLists.end()) {
                    mServerEventLists[ sloc_evt->dest ] = new EventList;
                    it = mServerEventLists.find( sloc_evt->dest );
                }
                assert( it != mServerEventLists.end() );

                EventList* evt_list = it->second;
                evt_list->push_back(sloc_evt);
            }
            else {
                delete evt;
                continue;
            }

        }
    }

    sort_events<EventList, ObjectEventListMap>(mEventLists);
    sort_events<EventList, ServerEventListMap>(mServerEventLists);
}

LocationErrorAnalysis::~LocationErrorAnalysis() {
    for(ObjectEventListMap::iterator event_lists_it = mEventLists.begin(); event_lists_it != mEventLists.end(); event_lists_it++) {
        EventList* event_list = event_lists_it->second;
        for(EventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++)
            delete *events_it;
    }

    for(ServerEventListMap::iterator event_lists_it = mServerEventLists.begin(); event_lists_it != mServerEventLists.end(); event_lists_it++) {
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
double LocationErrorAnalysis::averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate) const {
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

    MotionPath* true_path = new RecordedMotionPath( *(getEventList(seen)) );
    TimedMotionVector3f true_motion = true_path->initial();
    const TimedMotionVector3f* next_true_motion = true_path->nextUpdate(true_motion.time());

    EventList* events = getEventList(observer);
    TimedMotionVector3f pred_motion;

#if APPARENT_ERROR
    MotionPath* observer_path = new RecordedMotionPath( *(getEventList(observer)) );
    TimedMotionVector3f observer_motion = observer_path->initial();
    const TimedMotionVector3f* next_observer_motion = observer_path->nextUpdate(observer_motion.time());
#endif

    Time cur_time(Time::null());
    Mode mode = SEARCHING_PROX;
    double error_sum = 0.0;
    uint32 sample_count = 0;
    for(EventList::iterator main_it = events->begin(); main_it != events->end(); main_it++) {
        ObjectEvent* cur_event = dynamic_cast<ObjectEvent*>(*main_it);
        assert(cur_event != NULL);

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

    delete true_path;

    return (sample_count == 0) ? 0 : error_sum / sample_count;
}

double LocationErrorAnalysis::globalAverageError(const Duration& sampling_rate) const {
    double total_error = 0.0;
    uint32 total_pairs = 0;
    for(ObjectEventListMap::const_iterator observer_it = mEventLists.begin(); observer_it != mEventLists.end(); observer_it++) {
        UUID observer = observer_it->first;
        for(ObjectEventListMap::const_iterator seen_it = mEventLists.begin(); seen_it != mEventLists.end(); seen_it++) {
            UUID seen = seen_it->first;
            if (observer == seen) continue;
            if (observed(observer, seen)) {
                double error = averageError(observer, seen, sampling_rate);
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

LocationErrorAnalysis::EventList* LocationErrorAnalysis::getEventList(const ServerID& observer) const {
    ServerEventListMap::const_iterator event_lists_it = mServerEventLists.find(observer);
    if (event_lists_it == mServerEventLists.end()) return NULL;

    return event_lists_it->second;
}





template<typename EventType, typename EventListType, typename EventListMapType>
void insert_event(EventType* evt, EventListMapType& lists) {
    assert(evt != NULL);

    // put it in the source queue
    typename EventListMapType::iterator source_it = lists.find( evt->source );
    if (source_it == lists.end()) {
        lists[ evt->source ] = new EventListType;
        source_it = lists.find( evt->source );
    }
    assert( source_it != lists.end() );

    EventListType* evt_list = source_it->second;
    evt_list->push_back(evt);

    // put it in the dest queue
    typename EventListMapType::iterator dest_it = lists.find( evt->dest );
    if (dest_it == lists.end()) {
        lists[ evt->dest ] = new EventListType;
        dest_it = lists.find( evt->dest );
    }
    assert( dest_it != lists.end() );

    evt_list = dest_it->second;
    evt_list->push_back(evt);
}

BandwidthAnalysis::BandwidthAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    mNumberOfServers = nservers;

    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;

            bool used = false;

            ServerDatagramEvent* datagram_evt = dynamic_cast<ServerDatagramEvent*>(evt);
            if (datagram_evt != NULL) {
                used = true;
                insert_event<ServerDatagramEvent, DatagramEventList, ServerDatagramEventListMap>(datagram_evt, mDatagramEventLists);
            }

            PacketEvent* packet_evt = dynamic_cast<PacketEvent*>(evt);
            if (packet_evt != NULL) {
                used = true;
                insert_event<PacketEvent, PacketEventList, ServerPacketEventListMap>(packet_evt, mPacketEventLists);
            }

            ServerDatagramQueueInfoEvent* datagram_qi_evt = dynamic_cast<ServerDatagramQueueInfoEvent*>(evt);
            if (datagram_qi_evt != NULL) {
                used = true;
                insert_event<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(datagram_qi_evt, mDatagramQueueInfoEventLists);
            }

            PacketQueueInfoEvent* packet_qi_evt = dynamic_cast<PacketQueueInfoEvent*>(evt);
            if (packet_qi_evt != NULL) {
                used = true;
                insert_event<PacketQueueInfoEvent, PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(packet_qi_evt, mPacketQueueInfoEventLists);
            }

            if (!used) delete evt;
        }
    }

    // Sort all lists of events by time
    sort_events<DatagramEventList, ServerDatagramEventListMap>(mDatagramEventLists);
    sort_events<PacketEventList, ServerPacketEventListMap>(mPacketEventLists);

    sort_events<DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(mDatagramQueueInfoEventLists);
    sort_events<PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(mPacketQueueInfoEventLists);
}

BandwidthAnalysis::~BandwidthAnalysis() {
    for(ServerDatagramEventListMap::iterator event_lists_it = mDatagramEventLists.begin(); event_lists_it != mDatagramEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        DatagramEventList* event_list = event_lists_it->second;
        for(DatagramEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }

    for(ServerPacketEventListMap::iterator event_lists_it = mPacketEventLists.begin(); event_lists_it != mPacketEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        PacketEventList* event_list = event_lists_it->second;
        for(PacketEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }
}

const BandwidthAnalysis::DatagramEventList* BandwidthAnalysis::getDatagramEventList(const ServerID& server) const {
    ServerDatagramEventListMap::const_iterator event_lists_it = mDatagramEventLists.find(server);
    if (event_lists_it == mDatagramEventLists.end()) return &mEmptyDatagramEventList;

    return event_lists_it->second;
}

const BandwidthAnalysis::PacketEventList* BandwidthAnalysis::getPacketEventList(const ServerID& server) const {
    ServerPacketEventListMap::const_iterator event_lists_it = mPacketEventLists.find(server);
    if (event_lists_it == mPacketEventLists.end()) return &mEmptyPacketEventList;

    return event_lists_it->second;
}


const BandwidthAnalysis::DatagramQueueInfoEventList* BandwidthAnalysis::getDatagramQueueInfoEventList(const ServerID& server) const {
    ServerDatagramQueueInfoEventListMap::const_iterator event_lists_it = mDatagramQueueInfoEventLists.find(server);
    if (event_lists_it == mDatagramQueueInfoEventLists.end()) return &mEmptyDatagramQueueInfoEventList;

    return event_lists_it->second;
}

const BandwidthAnalysis::PacketQueueInfoEventList* BandwidthAnalysis::getPacketQueueInfoEventList(const ServerID& server) const {
    ServerPacketQueueInfoEventListMap::const_iterator event_lists_it = mPacketQueueInfoEventLists.find(server);
    if (event_lists_it == mPacketQueueInfoEventLists.end()) return &mEmptyPacketQueueInfoEventList;

    return event_lists_it->second;
}

BandwidthAnalysis::DatagramEventList::const_iterator BandwidthAnalysis::datagramBegin(const ServerID& server) const {
    return getDatagramEventList(server)->begin();
}

BandwidthAnalysis::DatagramEventList::const_iterator BandwidthAnalysis::datagramEnd(const ServerID& server) const {
    return getDatagramEventList(server)->end();
}

BandwidthAnalysis::PacketEventList::const_iterator BandwidthAnalysis::packetBegin(const ServerID& server) const {
    return getPacketEventList(server)->begin();
}

BandwidthAnalysis::PacketEventList::const_iterator BandwidthAnalysis::packetEnd(const ServerID& server) const {
    return getPacketEventList(server)->end();
}

BandwidthAnalysis::DatagramQueueInfoEventList::const_iterator BandwidthAnalysis::datagramQueueInfoBegin(const ServerID& server) const {
    return getDatagramQueueInfoEventList(server)->begin();
}

BandwidthAnalysis::DatagramQueueInfoEventList::const_iterator BandwidthAnalysis::datagramQueueInfoEnd(const ServerID& server) const {
    return getDatagramQueueInfoEventList(server)->end();
}

BandwidthAnalysis::PacketQueueInfoEventList::const_iterator BandwidthAnalysis::packetQueueInfoBegin(const ServerID& server) const {
    return getPacketQueueInfoEventList(server)->begin();
}

BandwidthAnalysis::PacketQueueInfoEventList::const_iterator BandwidthAnalysis::packetQueueInfoEnd(const ServerID& server) const {
    return getPacketQueueInfoEventList(server)->end();
}

template<typename EventType, typename EventIteratorType>
void computeRate(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end) {
    uint64 total_bytes = 0;
    uint32 last_bytes = 0;
    Duration last_duration;
    Time last_time(Time::null());
    double max_bandwidth = 0;
    for(EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end); event_it.current() != NULL; event_it.next() ) {
        EventType* p_evt = event_it.current();

        total_bytes += p_evt->size;

        if (p_evt->time != last_time) {
            double bandwidth = (double)last_bytes / last_duration.toSeconds();
            if (bandwidth > max_bandwidth)
                max_bandwidth = bandwidth;

            last_bytes = 0;
            last_duration = p_evt->time - last_time;
            last_time = p_evt->time;
        }

        last_bytes += p_evt->size;
    }

    printf("%d to %d: %ld total, %f max\n", sender, receiver, total_bytes, max_bandwidth);
}


void BandwidthAnalysis::computeSendRate(const ServerID& sender, const ServerID& receiver) const {
    if (getDatagramEventList(sender) == NULL ) {
        printf("%d to %d: unknown total, unknown max\n", sender, receiver);
	return;
    }

    computeRate<ServerDatagramSentEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(sender), datagramEnd(sender));
}

void BandwidthAnalysis::computeReceiveRate(const ServerID& sender, const ServerID& receiver) const {
    if (getDatagramEventList(receiver) == NULL ) {
        printf("%d to %d: unknown total, unknown max\n", sender, receiver);
	return;
    }

    computeRate<ServerDatagramReceivedEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(receiver), datagramEnd(receiver));
}

// note: swap_sender_receiver optionally swaps order for sake of graphing code, generally will be used when collecting stats for "receiver" side
template<typename EventType, typename EventIteratorType>
void computeWindowedRate(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out, bool swap_sender_receiver) {
    EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end);
    std::queue<EventType*> window_events;

    uint64 bytes = 0;
    uint64 total_bytes = 0;
    double max_bandwidth = 0;

    for(Time window_center = start_time; window_center < end_time; window_center += sample_rate) {
        Time window_end = window_center + window / 2.f;

        // add in any new packets that now fit in the window
        uint32 last_packet_partial_size = 0;
        while(true) {
            EventType* evt = event_it.current();
            if (evt == NULL) break;
            if (evt->end_time() > window_end) {
                if (evt->begin_time() + window < window_end) {
                    double packet_frac = (window_end - evt->begin_time()).toSeconds() / (evt->end_time() - evt->begin_time()).toSeconds();
                    last_packet_partial_size = evt->size * packet_frac;
                }
                break;
            }
            bytes += evt->size;
            total_bytes += evt->size;
            window_events.push(evt);
            event_it.next();
        }

        // subtract out any packets that have fallen out of the window
        // note we use event_time + window < window_end because subtracting could underflow the time
        uint32 first_packet_partial_size = 0;
        while(!window_events.empty()) {
            EventType* pevt = window_events.front();
            if (pevt->begin_time() + window >= window_end) break;

            bytes -= pevt->size;
            window_events.pop();

            if (pevt->end_time() + window >= window_end) {
                // note the order of the numerator is important to avoid underflow
                double packet_frac = (pevt->end_time() + window - window_end).toSeconds() / (pevt->end_time() - pevt->begin_time()).toSeconds();
                first_packet_partial_size = pevt->size * packet_frac;
                break;
            }
        }

        // finally compute the current bandwidth
        double bandwidth = (first_packet_partial_size + bytes + last_packet_partial_size) / window.toSeconds();
        if (bandwidth > max_bandwidth)
            max_bandwidth = bandwidth;

        // optionally swap order for sake of graphing code, generally will be used when collecting stats for "receiver" side
        if (swap_sender_receiver)
            detail_out << receiver << " " << sender << " ";
        else
            detail_out << sender << " " << receiver << " ";
        detail_out << (window_center-Time::null()).toMilliseconds() << " " << bandwidth << std::endl;
    }

    // optionally swap order for sake of graphing code, generally will be used when collecting stats for "receiver" side
    if (swap_sender_receiver)
        summary_out << receiver << " from " << sender << ": ";
    else
        summary_out << sender << " to " << receiver << ": ";
    summary_out << total_bytes << " total, " << max_bandwidth << " max" << std::endl;
}

template<typename EventType, typename EventIteratorType>
void BandwidthAnalysis::computeJFI(const ServerID& sender, const ServerID& filter) const {
      uint64 total_bytes = 0;

      float sum = 0;
      float sum_of_squares = 0;

      for (uint32 receiver = 1; receiver <= mNumberOfServers; receiver++) {
        if (receiver != sender) {
          total_bytes = 0;
          float weight = 0;
          if (getDatagramEventList(receiver) == NULL) continue;

          for (EventIterator<EventType, EventIteratorType> event_it(sender, receiver, datagramBegin(receiver), datagramEnd(receiver)); event_it.current() != NULL; event_it.next() )
            {
              EventType* p_evt = event_it.current();

              total_bytes += p_evt->size;

              weight = p_evt->weight;


            }

          sum += total_bytes/weight;
          sum_of_squares += (total_bytes/weight) * (total_bytes/weight);
        }
      }

      if ( (mNumberOfServers-1)*sum_of_squares != 0) {
          printf("JFI for sender %d is %f\n", sender, (sum*sum/((mNumberOfServers-1)*sum_of_squares) ) );
      }
      else {
          printf("JFI for sender %d cannot be computed\n", sender);
      }
}



void BandwidthAnalysis::computeWindowedDatagramSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<ServerDatagramSentEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(sender), datagramEnd(sender), window, sample_rate, start_time, end_time, summary_out, detail_out, false);
}

void BandwidthAnalysis::computeWindowedDatagramReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<ServerDatagramReceivedEvent, DatagramEventList::const_iterator>(sender, receiver, datagramBegin(receiver), datagramEnd(receiver), window, sample_rate, start_time, end_time, summary_out, detail_out, true);
}

void BandwidthAnalysis::computeWindowedPacketSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<PacketSentEvent, PacketEventList::const_iterator>(sender, receiver, packetBegin(sender), packetEnd(sender), window, sample_rate, start_time, end_time, summary_out, detail_out, false);
}

void BandwidthAnalysis::computeWindowedPacketReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    computeWindowedRate<PacketReceivedEvent, PacketEventList::const_iterator>(sender, receiver, packetBegin(receiver), packetEnd(receiver), window, sample_rate, start_time, end_time, summary_out, detail_out, true);
}

void BandwidthAnalysis::computeJFI(const ServerID& sender) const {
  computeJFI<ServerDatagramSentEvent, DatagramEventList::const_iterator>(sender, sender);
}


template<typename EventType, typename EventIteratorType>
void dumpQueueInfo(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end, std::ostream& summary_out, std::ostream& detail_out) {
    EventType* q_evt = NULL;
    EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end);

    while((q_evt = event_it.current()) != NULL) {
        detail_out << sender << " " << receiver << " " << (q_evt->time-Time::null()).toMilliseconds() << " " << q_evt->send_size << " " << q_evt->send_queued << " " << q_evt->send_weight << " " << q_evt->receive_size << " " << q_evt->receive_queued << " " << q_evt->receive_weight << std::endl;
        event_it.next();
    }
    //summary_out << std::endl;
}

void BandwidthAnalysis::dumpDatagramQueueInfo(const ServerID& sender, const ServerID& receiver, std::ostream& summary_out, std::ostream& detail_out) {
    dumpQueueInfo<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList::const_iterator>(sender, receiver, datagramQueueInfoBegin(sender), datagramQueueInfoEnd(sender), summary_out, detail_out);
}

void BandwidthAnalysis::dumpPacketQueueInfo(const ServerID& sender, const ServerID& receiver, std::ostream& summary_out, std::ostream& detail_out) {
    dumpQueueInfo<PacketQueueInfoEvent, PacketQueueInfoEventList::const_iterator>(sender, receiver, packetQueueInfoBegin(sender), packetQueueInfoEnd(sender), summary_out, detail_out);
}


// note: swap_sender_receiver optionally swaps order for sake of graphing code, generally will be used when collecting stats for "receiver" side
template<typename EventType, typename EventIteratorType, typename ValueFunctor>
void windowedQueueInfo(const ServerID& sender, const ServerID& receiver, const EventIteratorType& filter_begin, const EventIteratorType& filter_end, const ValueFunctor& value_func, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out, bool swap_sender_receiver) {
    EventIterator<EventType, EventIteratorType> event_it(sender, receiver, filter_begin, filter_end);
    std::queue<EventType*> window_events;

    uint64 window_queued_bytes = 0;
    float window_weight = 0.f;

    for(Time window_center = start_time; window_center < end_time; window_center += sample_rate) {
        Time window_end = window_center + window / 2.f;

        // add in any new packets that now fit in the window
        while(true) {
            EventType* evt = event_it.current();
            if (evt == NULL) break;
            if (evt->end_time() > window_end)
                break;
            window_queued_bytes += value_func.queued(evt);
            window_weight += value_func.weight(evt);
            window_events.push(evt);
            event_it.next();
        }

        // subtract out any packets that have fallen out of the window
        // note we use event_time + window < window_end because subtracting could underflow the time
        while(!window_events.empty()) {
            EventType* pevt = window_events.front();
            if (pevt->begin_time() + window >= window_end) break;

            window_queued_bytes -= value_func.queued(pevt);
            window_weight -= value_func.weight(pevt);
            window_events.pop();
        }

        // finally compute the current values
        uint64 window_queued_avg = (window_events.size() == 0) ? 0 : window_queued_bytes / window_events.size();
        float window_weight_avg = (window_events.size() == 0) ? 0 : window_weight / window_events.size();

        // optionally swap order for sake of graphing code, generally will be used when collecting stats for "receiver" side
        if (swap_sender_receiver)
            detail_out << receiver << " " << sender << " ";
        else
            detail_out << sender << " " << receiver << " ";
        detail_out << (window_center-Time::null()).toMilliseconds() << " " << window_queued_avg << " " << window_weight_avg << std::endl;
    }

    // FIXME we should probably output *something* for summary_out
}


template<typename EventType>
struct SendQueueFunctor {
    uint32 size(const EventType* evt) const {
        return evt->send_size;
    }
    uint32 queued(const EventType* evt) const {
        return evt->send_queued;
    }
    float weight(const EventType* evt) const {
        return evt->send_weight;
    }
};

template<typename EventType>
struct ReceiveQueueFunctor {
    uint32 size(const EventType* evt) const {
        return evt->receive_size;
    }
    uint32 queued(const EventType* evt) const {
        return evt->receive_queued;
    }
    float weight(const EventType* evt) const {
        return evt->receive_weight;
    }
};


void BandwidthAnalysis::windowedDatagramSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList::const_iterator, SendQueueFunctor<ServerDatagramQueueInfoEvent> >(
        sender, receiver,
        datagramQueueInfoBegin(sender), datagramQueueInfoEnd(sender),
        SendQueueFunctor<ServerDatagramQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        false
    );
}

void BandwidthAnalysis::windowedDatagramReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList::const_iterator, ReceiveQueueFunctor<ServerDatagramQueueInfoEvent> >(
        sender, receiver,
        datagramQueueInfoBegin(receiver), datagramQueueInfoEnd(receiver),
        ReceiveQueueFunctor<ServerDatagramQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        true
    );
}

void BandwidthAnalysis::windowedPacketSendQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<PacketQueueInfoEvent, PacketQueueInfoEventList::const_iterator, SendQueueFunctor<PacketQueueInfoEvent> >(
        sender, receiver,
        packetQueueInfoBegin(sender), packetQueueInfoEnd(sender),
        SendQueueFunctor<PacketQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        false
    );
}

void BandwidthAnalysis::windowedPacketReceiveQueueInfo(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time, std::ostream& summary_out, std::ostream& detail_out) {
    windowedQueueInfo<PacketQueueInfoEvent, PacketQueueInfoEventList::const_iterator, ReceiveQueueFunctor<PacketQueueInfoEvent> >(
        sender, receiver,
        packetQueueInfoBegin(receiver), packetQueueInfoEnd(receiver),
        ReceiveQueueFunctor<PacketQueueInfoEvent>(),
        window, sample_rate, start_time, end_time,
        summary_out, detail_out,
        true
    );
}







LatencyAnalysis::PacketData::PacketData()
 :_send_start_time(Time::null()),
  _send_end_time(Time::null()),
  _receive_start_time(Time::null()),
  _receive_end_time(Time::null())
{
    mSize=0;
    mId=0;
}
void LatencyAnalysis::PacketData::addPacketSentEvent(ServerDatagramQueuedEvent*sde) {
    mSize=sde->size;
    mId=sde->id;
    source=sde->source;
    dest=sde->dest;
    if (_send_start_time==Time::null()||_send_start_time>=sde->begin_time()) {
        _send_start_time=sde->time;
        _send_end_time=sde->time;
    }
}
void LatencyAnalysis::PacketData::addPacketReceivedEvent(ServerDatagramReceivedEvent*sde) {
    mSize=sde->size;
    mId=sde->id;
    source=sde->source;
    dest=sde->dest;
    if (_receive_end_time==Time::null()||_receive_end_time<=sde->end_time()) {
        _receive_start_time=sde->begin_time();
        _receive_end_time=sde->end_time();
    }
}

ObjectLatencyAnalysis::ObjectLatencyAnalysis(const char*opt_name, const uint32 nservers) {
    mNumberOfServers = nservers;
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;

            {
                PingEvent* ping_evt = dynamic_cast<PingEvent*>(evt);
                if (ping_evt != NULL) {
                    mLatency.insert(
                        std::multimap<double,Duration>::value_type(ping_evt->distance,ping_evt->end_time()-ping_evt->begin_time()));
                }
            }
            delete evt;
        }
    }

}

void ObjectLatencyAnalysis::histogramDistanceData(double bucketWidth, std::map<int, Average > &retval){
    int bucket=-1;
    int bucketSamples=0;
    for (std::multimap<double,Duration>::iterator i=mLatency.begin(),ie=mLatency.end();
         i!=ie;++i) {
        int curBucket=(int)floor(i->first/bucketWidth);
        if (curBucket!=bucket) {
            if (bucketSamples) {
                retval.find(bucket)->second.time/=(double)bucketSamples;
                retval.find(bucket)->second.numSamples=(double)bucketSamples;
            }
            bucket=curBucket;
            bucketSamples=1;
            retval.insert(std::map<int,Average>::value_type(bucket,Average(i->second)));
        }else {
            bucketSamples++;
            retval.find(bucket)->second.time+=i->second;
        }
    }
    if (bucketSamples) {
        retval.find(bucket)->second.time/=(double)bucketSamples;
        retval.find(bucket)->second.numSamples=bucketSamples;
    }
}
ObjectLatencyAnalysis::~ObjectLatencyAnalysis(){}
void ObjectLatencyAnalysis::printHistogramDistanceData(std::ostream&out, double bucketWidth){
    std::map<int,Average> histogram;
    histogramDistanceData(bucketWidth,histogram);
    for (std::map<int,Average>::iterator i=histogram.begin(),ie=histogram.end();i!=ie;++i) {
        out<<i->first*bucketWidth<<", ";
        if(i->second.time.toMicroseconds()>9999) {
            out<<i->second.time;
        }else {
            out<<i->second.time.toMicroseconds()<<"us";
        }
        out <<", ";
        out << i->second.time.toMicroseconds();
        out << ", "<<i->second.numSamples<<'\n';
    }
}


LatencyAnalysis::LatencyAnalysis(const char* opt_name, const uint32 nservers) {
    // read in all our data
    mNumberOfServers = nservers;
    std::tr1::unordered_map<uint64,PacketData> packetFlow;
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;


            {
                ServerDatagramReceivedEvent* datagram_evt = dynamic_cast<ServerDatagramReceivedEvent*>(evt);
                if (datagram_evt != NULL) {
                    packetFlow[datagram_evt->id].addPacketReceivedEvent(datagram_evt);
                }
            }
            {
                ServerDatagramQueuedEvent* datagram_evt = dynamic_cast<ServerDatagramQueuedEvent*>(evt);
                if (datagram_evt != NULL) {
                    packetFlow[datagram_evt->id].addPacketSentEvent(datagram_evt);
                }
            }
            PacketEvent* packet_evt = dynamic_cast<PacketEvent*>(evt);
            if (packet_evt != NULL) {
                //insert_event<PacketEvent, PacketEventList, ServerPacketEventListMap>(packet_evt, mPacketEventLists);
            }
            ServerDatagramQueueInfoEvent* datagram_qi_evt = dynamic_cast<ServerDatagramQueueInfoEvent*>(evt);
            if (datagram_qi_evt != NULL) {
                //insert_event<ServerDatagramQueueInfoEvent, DatagramQueueInfoEventList, ServerDatagramQueueInfoEventListMap>(datagram_qi_evt, mDatagramQueueInfoEventLists);
            }

            PacketQueueInfoEvent* packet_qi_evt = dynamic_cast<PacketQueueInfoEvent*>(evt);
            if (packet_qi_evt != NULL) {
                //insert_event<PacketQueueInfoEvent, PacketQueueInfoEventList, ServerPacketQueueInfoEventListMap>(packet_qi_evt, mPacketQueueInfoEventLists);
            }

            delete evt;
        }
    }

    class LatencyStats {
    public:
        LatencyStats()
         : finished(0), unfinished(0), latency(Duration::microseconds(0))
        {}

        void sample(Duration dt) {
            latency += dt;
            finished++;
        }

        Duration avg() const {
            if (finished > 0)
                return latency / (double)finished;
            else
                return Duration::microseconds(0);
        }

        uint32 finished;
        uint32 unfinished;
    private:
        Duration latency;
    };

    struct ServerLatencyInfo {
        ServerLatencyInfo() : to(NULL) {}
        ~ServerLatencyInfo() { delete to; }

        void init(uint32 nservers)
        { to = new LatencyStats[nservers+1]; }

        LatencyStats in;
        LatencyStats out;

        LatencyStats* to;
    };

    ServerLatencyInfo* server_latencies = new ServerLatencyInfo[nservers+1];
    for(uint32 i = 0; i < nservers+1; i++)
        server_latencies[i].init(nservers+1);

    Time epoch(Time::null());

    for (std::tr1::unordered_map<uint64,PacketData>::iterator i=packetFlow.begin(),ie=packetFlow.end();i!=ie;++i) {
        PacketData data = i->second;

        if (data._receive_end_time != epoch && data._send_start_time != epoch) {
            Duration delta = data._receive_end_time-data._send_start_time;
            if (delta>Duration::seconds(0.0f)) {

                // From one to the other
                server_latencies[data.source].to[data.dest].sample(delta);
                // And the totals
                server_latencies[data.source].out.sample(delta);
                server_latencies[data.dest].in.sample(delta);
            }else if (delta<Duration::seconds(0.0f)){
                //printf ("Packet with id %ld, negative duration %f (%f %f)->(%f %f) %d (%f %f) (%f %f)\n",data.mId,delta.toSeconds(),0.,(data._send_end_time-data._send_start_time).toSeconds(),(data._receive_start_time-data._send_start_time).toSeconds(),(data._receive_end_time-data._send_start_time).toSeconds(),data.mSize, (data._send_start_time-Time::null()).toSeconds(),(data._send_end_time-Time::null()).toSeconds(),(data._receive_start_time-Time::null()).toSeconds(),(data._receive_end_time-Time::null()).toSeconds());
            }
        }else{
            server_latencies[data.source].to[data.dest].unfinished++;
            if (data._receive_end_time == epoch && data._send_start_time == epoch) {
                printf ("Packet with uninitialized duration\n");
            }
        }
    }
    packetFlow.clear();


    for(uint32 source_id = 1; source_id <= nservers; source_id++) {
        for(uint32 dest_id = 1; dest_id <= nservers; dest_id++) {
            std::cout << "Server " << source_id << " to " << dest_id << " : "
                      << server_latencies[source_id].to[dest_id].avg() << " ("
                      << server_latencies[source_id].to[dest_id].finished << ","
                      << server_latencies[source_id].to[dest_id].unfinished << ")" << std::endl;
        }
    }

    for(uint32 serv_id = 1; serv_id <= nservers; serv_id++) {
        std::cout << "Server " << serv_id
                  << " In: " << server_latencies[serv_id].in.avg()
                  << " (" << server_latencies[serv_id].in.finished << ")" << std::endl;

        std::cout << "Server " << serv_id
                  << " Out: " << server_latencies[serv_id].out.avg()
                  << " (" << server_latencies[serv_id].out.finished << ")" << std::endl;
    }

}

LatencyAnalysis::~LatencyAnalysis() {
/*
    for(ServerDatagramEventListMap::iterator event_lists_it = mDatagramEventLists.begin(); event_lists_it != mDatagramEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        DatagramEventList* event_list = event_lists_it->second;
        for(DatagramEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }

    for(ServerPacketEventListMap::iterator event_lists_it = mPacketEventLists.begin(); event_lists_it != mPacketEventLists.end(); event_lists_it++) {
        ServerID server_id = event_lists_it->first;
        PacketEventList* event_list = event_lists_it->second;
        for(PacketEventList::iterator events_it = event_list->begin(); events_it != event_list->end(); events_it++) {
            // each event is put in both the source and the dest server event lists,
            // to avoid double deleting, only delete if this is the event's source list
            if ((*events_it)->source == server_id)
                delete *events_it;
        }
    }
*/
}



  //object move messages
  ObjectSegmentationAnalysis::ObjectSegmentationAnalysis(const char* opt_name, const uint32 nservers)
  {

    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;


        ObjectBeginMigrateEvent* obj_mig_evt = dynamic_cast<ObjectBeginMigrateEvent*>(evt);
        if (obj_mig_evt != NULL)
        {
          objectBeginMigrateTimes.push_back(obj_mig_evt->time);

          objectBeginMigrateID.push_back(obj_mig_evt->mObjID);
          objectBeginMigrateMigrateFrom.push_back(obj_mig_evt->mMigrateFrom);
          objectBeginMigrateMigrateTo.push_back(obj_mig_evt->mMigrateTo);
          delete evt;
          continue;
        }

        ObjectAcknowledgeMigrateEvent* obj_ack_mig_evt = dynamic_cast<ObjectAcknowledgeMigrateEvent*> (evt);
        if (obj_ack_mig_evt != NULL)
        {
          objectAcknowledgeMigrateTimes.push_back(obj_ack_mig_evt->time);

          objectAcknowledgeMigrateID.push_back(obj_ack_mig_evt->mObjID);
          objectAcknowledgeAcknowledgeFrom.push_back(obj_ack_mig_evt->mAcknowledgeFrom);
          objectAcknowledgeAcknowledgeTo.push_back(obj_ack_mig_evt->mAcknowledgeTo);
          delete evt;
          continue;
        }

        delete evt;
      } //end while(is)

    }//end for
  }//end constructor



  /*
    Nothing to populate in destructor.
  */

  ObjectSegmentationAnalysis::~ObjectSegmentationAnalysis()
  {
  }

  bool ObjectSegmentationAnalysis::compareObjectBeginMigrateEvts(ObjectBeginMigrateEvent A, ObjectBeginMigrateEvent B)
  {
    return A.time < B.time;
  }


  bool ObjectSegmentationAnalysis::compareObjectAcknowledgeMigrateEvts(ObjectAcknowledgeMigrateEvent A, ObjectAcknowledgeMigrateEvent B)
  {
    return A.time < B.time;
  }


  void ObjectSegmentationAnalysis::convertToEvtsAndSort(std::vector<ObjectBeginMigrateEvent> &sortedBeginMigrateEvents, std::vector<ObjectAcknowledgeMigrateEvent> &sortedAcknowledgeMigrateEvents)
  {
    //begin migrate events
    ObjectBeginMigrateEvent obme;

    for (int s= 0; s < (int) objectBeginMigrateTimes.size(); ++s)
    {
      obme.time           =        objectBeginMigrateTimes[s];
      obme.mObjID         =           objectBeginMigrateID[s];
      obme.mMigrateFrom   =  objectBeginMigrateMigrateFrom[s];
      obme.mMigrateTo     =    objectBeginMigrateMigrateTo[s];

      sortedBeginMigrateEvents.push_back(obme);
    }

    std::sort(sortedBeginMigrateEvents.begin(),sortedBeginMigrateEvents.end(), compareObjectBeginMigrateEvts );

    //acknowledge events
    ObjectAcknowledgeMigrateEvent oame;

    for (int s =0; s < (int) objectAcknowledgeMigrateTimes.size(); ++s)
    {
      oame.time             =     objectAcknowledgeMigrateTimes[s];
      oame.mObjID           =        objectAcknowledgeMigrateID[s];
      oame.mAcknowledgeFrom =  objectAcknowledgeAcknowledgeFrom[s];
      oame.mAcknowledgeTo   =    objectAcknowledgeAcknowledgeTo[s];

      sortedAcknowledgeMigrateEvents.push_back(oame);
    }

    std::sort(sortedAcknowledgeMigrateEvents.begin(), sortedAcknowledgeMigrateEvents.end(), compareObjectAcknowledgeMigrateEvts);

  }




  void ObjectSegmentationAnalysis::printData(std::ostream &fileOut, bool sortedByTime)
  {
    if (sortedByTime)
    {
      std::vector<ObjectBeginMigrateEvent>               sortedBeginMigrateEvents;
      std::vector<ObjectAcknowledgeMigrateEvent>   sortedAcknowledgeMigrateEvents;

      convertToEvtsAndSort(sortedBeginMigrateEvents, sortedAcknowledgeMigrateEvents);

      fileOut<<"\n\nSize of objectMigrateTimes:               "<<sortedBeginMigrateEvents.size()<<"\n\n";
      fileOut<<"\n\nSize of objectAcknowledgeMigrateTimes:    "<<sortedAcknowledgeMigrateEvents.size()<<"\n\n";

      fileOut << "\n\n*******************Begin Migrate Messages*************\n\n\n";

      for (int s=0; s < (int)sortedBeginMigrateEvents.size(); ++s)
      {
        fileOut << sortedBeginMigrateEvents[s].time.raw() << "\n";
        fileOut << "          obj id:       " << sortedBeginMigrateEvents[s].mObjID.toString() << "\n";
        fileOut << "          migrate from: " << sortedBeginMigrateEvents[s].mMigrateFrom      << "\n";
        fileOut << "          migrate to:   " << sortedBeginMigrateEvents[s].mMigrateTo        << "\n";
        fileOut << "\n\n";
      }

      fileOut << "\n\n\n\n*******************Begin Migrate Acknowledge Messages*************\n\n\n";

      for (int s=0; s < (int) sortedAcknowledgeMigrateEvents.size(); ++s)
      {
        fileOut << sortedAcknowledgeMigrateEvents[s].time.raw() << "\n";
        fileOut << "          obj id:           " << sortedAcknowledgeMigrateEvents[s].mObjID.toString() << "\n";
        fileOut << "          acknowledge from: " << sortedAcknowledgeMigrateEvents[s].mAcknowledgeFrom  << "\n";
        fileOut << "          acknowledge to:   " << sortedAcknowledgeMigrateEvents[s].mAcknowledgeTo    << "\n";
        fileOut << "\n\n";
      }
    }
    else
    {
      fileOut<<"\n\nSize of objectMigrateTimes:               "<<objectBeginMigrateTimes.size()<<"\n\n";
      fileOut<<"\n\nSize of objectAcknowledgeMigrateTimes:    "<<(int)objectAcknowledgeMigrateTimes.size()<<"\n\n";

      fileOut << "\n\n*******************Begin Migrate Messages*************\n\n\n";

      for (int s=0; s < (int)objectBeginMigrateTimes.size(); ++s)
      {
        fileOut << objectBeginMigrateTimes[s].raw() << "\n";
        fileOut << "          obj id:       " << objectBeginMigrateID[s].toString() << "\n";
        fileOut << "          migrate from: " << objectBeginMigrateMigrateFrom[s] << "\n";
        fileOut << "          migrate to:   " << objectBeginMigrateMigrateTo[s] << "\n";
        fileOut << "\n\n";
      }

      fileOut << "\n\n\n\n*******************Begin Migrate Acknowledge Messages*************\n\n\n";

      for (int s=0; s < (int)objectAcknowledgeMigrateTimes.size(); ++s)
      {
        fileOut << objectAcknowledgeMigrateTimes[s].raw() << "\n";
        fileOut << "          obj id:           " << objectAcknowledgeMigrateID[s].toString() << "\n";
        fileOut << "          acknowledge from: " << objectAcknowledgeAcknowledgeFrom[s] << "\n";
        fileOut << "          acknowledge to:   " << objectAcknowledgeAcknowledgeTo[s] << "\n";
        fileOut << "\n\n";
      }
    }
  }





  ////ObjectSegCraqLookupReqAnalysis
  ObjectSegmentationCraqLookupRequestsAnalysis::ObjectSegmentationCraqLookupRequestsAnalysis(const char* opt_name, const uint32 nservers)
  {
    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;

        ObjectCraqLookupEvent* obj_lookup_evt = dynamic_cast<ObjectCraqLookupEvent*> (evt);
        if (obj_lookup_evt != NULL)
        {
          times.push_back(obj_lookup_evt->time);
          obj_ids.push_back(obj_lookup_evt->mObjID);
          sID_lookup.push_back(obj_lookup_evt->mID_lookup);
          delete evt;
          continue;
        }

        delete evt;
      }
    }
  }

  void ObjectSegmentationCraqLookupRequestsAnalysis::printData(std::ostream &fileOut, bool sortByTime)
  {

    if (sortByTime)
    {
      std::vector<ObjectCraqLookupEvent> sortedEvents;
      convertToEvtsAndSort(sortedEvents);

      fileOut << "\n\n*******************Begin Craq Lookup Requests Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< sortedEvents.size() <<"  \n\n";


      for (int s= 0; s < (int) sortedEvents.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:   "<<sortedEvents[s].mID_lookup<<"\n";
        fileOut<< "\tTime at:           "<<sortedEvents[s].time.raw()<<"\n";
        fileOut<< "\tObj id:            "<<sortedEvents[s].mObjID.toString()<<"\n";
      }

      fileOut<<"\n\n\n\nEND\n";
    }
    else
    {

      fileOut<<"\n\n\n\nEND\n";


      fileOut << "\n\n*******************Begin Craq Lookup Requests Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< times.size() <<"  \n\n";


      for (int s= 0; s < (int) times.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:   "<<sID_lookup[s]<<"\n";
        fileOut<< "\tTime at:           "<<times[s].raw()<<"\n";
        fileOut<< "\tObj id:            "<<obj_ids[s].toString()<<"\n";
      }

      fileOut<<"\n\n\n\nEND\n";
    }

  }

  //want to sort by times.
  //probably a bad way to do this.  I could have just read them in correctly in the first place.
  void ObjectSegmentationCraqLookupRequestsAnalysis::convertToEvtsAndSort(std::vector<ObjectCraqLookupEvent> &sortedEvents)
  {
    ObjectCraqLookupEvent ole;

    for (int s= 0; s < (int) times.size(); ++s)
    {
      ole.time = times[s];
      ole.mObjID = obj_ids[s];
      ole.mID_lookup = sID_lookup[s];

      sortedEvents.push_back(ole);
    }

    std::sort(sortedEvents.begin(),sortedEvents.end(), compareEvts );
  }

  bool ObjectSegmentationCraqLookupRequestsAnalysis::compareEvts(ObjectCraqLookupEvent A, ObjectCraqLookupEvent B)
  {
    return A.time < B.time;
  }

  ObjectSegmentationCraqLookupRequestsAnalysis::~ObjectSegmentationCraqLookupRequestsAnalysis()
  {

  }


  //oseg lookup not on server


  ObjectSegmentationLookupNotOnServerRequestsAnalysis::ObjectSegmentationLookupNotOnServerRequestsAnalysis(const char* opt_name, const uint32 nservers)
  {
    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;

        ObjectLookupNotOnServerEvent* obj_lookup_evt = dynamic_cast<ObjectLookupNotOnServerEvent*> (evt);
        if (obj_lookup_evt != NULL)
        {
          times.push_back(obj_lookup_evt->time);
          obj_ids.push_back(obj_lookup_evt->mObjID);
          sID_lookup.push_back(obj_lookup_evt->mID_lookup);
          delete evt;
          continue;
        }

        delete evt;
      }
    }
  }

  void ObjectSegmentationLookupNotOnServerRequestsAnalysis::printData(std::ostream &fileOut, bool sortByTime)
  {

    if (sortByTime)
    {
      std::vector<ObjectLookupNotOnServerEvent> sortedEvents;
      convertToEvtsAndSort(sortedEvents);

      fileOut << "\n\n*******************Begin Lookup Requests Not On Server Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< sortedEvents.size() <<"  \n\n";


      for (int s= 0; s < (int) sortedEvents.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:   "<<sortedEvents[s].mID_lookup<<"\n";
        fileOut<< "\tTime at:           "<<sortedEvents[s].time.raw()<<"\n";
        fileOut<< "\tObj id:            "<<sortedEvents[s].mObjID.toString()<<"\n";
      }

      fileOut<<"\n\n\n\nEND\n";
    }
    else
    {

      fileOut<<"\n\n\n\nEND\n";


      fileOut << "\n\n*******************Begin Craq Lookup Requests Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< times.size() <<"  \n\n";


      for (int s= 0; s < (int) times.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:   "<<sID_lookup[s]<<"\n";
        fileOut<< "\tTime at:           "<<times[s].raw()<<"\n";
        fileOut<< "\tObj id:            "<<obj_ids[s].toString()<<"\n";
      }

      fileOut<<"\n\n\n\nEND\n";
    }

  }

  //want to sort by times.
  //probably a bad way to do this.  I could have just read them in correctly in the first place.
  void ObjectSegmentationLookupNotOnServerRequestsAnalysis::convertToEvtsAndSort(std::vector<ObjectLookupNotOnServerEvent> &sortedEvents)
  {
    ObjectLookupNotOnServerEvent ole;

    for (int s= 0; s < (int) times.size(); ++s)
    {
      ole.time = times[s];
      ole.mObjID = obj_ids[s];
      ole.mID_lookup = sID_lookup[s];

      sortedEvents.push_back(ole);
    }

    std::sort(sortedEvents.begin(),sortedEvents.end(), compareEvts );
  }

  bool ObjectSegmentationLookupNotOnServerRequestsAnalysis::compareEvts(ObjectLookupNotOnServerEvent A, ObjectLookupNotOnServerEvent B)
  {
    return A.time < B.time;
  }

  ObjectSegmentationLookupNotOnServerRequestsAnalysis::~ObjectSegmentationLookupNotOnServerRequestsAnalysis()
  {

  }

  //end oseg lookup not on server



  ///ObjectSegProcessAnalysis
  ObjectSegmentationProcessedRequestsAnalysis::ObjectSegmentationProcessedRequestsAnalysis (const char* opt_name, const uint32 nservers)
  {
    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;

        ObjectLookupProcessedEvent* obj_lookup_proc_evt = dynamic_cast<ObjectLookupProcessedEvent*> (evt);

        if (obj_lookup_proc_evt != NULL)
        {
          times.push_back(obj_lookup_proc_evt->time);
          obj_ids.push_back(obj_lookup_proc_evt->mObjID);
          sID_processor.push_back(obj_lookup_proc_evt->mID_processor);
          sID_objectOn.push_back(obj_lookup_proc_evt->mID_objectOn);
          dTimes.push_back(obj_lookup_proc_evt->deltaTime);
          stillInQueues.push_back(obj_lookup_proc_evt->stillInQueue);
          delete evt;
          continue;
        }
        delete evt;
      }
    }
  }


  void ObjectSegmentationProcessedRequestsAnalysis::printDataCSV(std::ostream &fileOut, bool sortedByTime, int processAfter)
  {
    double processAfterInMicro = processAfter* OSEG_SECOND_TO_RAW_CONVERSION_FACTOR; //convert seconds of processAfter to microseconds for comparison to time.raw().
    double totalLatency = 0;
    int maxLatency = 0;
    double numCountedLatency  = 0;

    if (sortedByTime)
    {
      std::vector<ObjectLookupProcessedEvent> sortedEvts;
      convertToEvtsAndSort(sortedEvts);

      for (int s=0; s < (int) sortedEvts.size(); ++s)
      {
        if (sortedEvts[s].time.raw() > processAfterInMicro)
        {
          fileOut <<sortedEvts[s].deltaTime<<",";
        }
      }
    }
    else
    {
      fileOut << "\n\n*******************Begin Lookup Processed Requests Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< times.size() <<"  \n\n";

      for (int s= 0; s < (int) times.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:            "<<sID_processor[s]<<"\n";
        fileOut<< "\tTime at:                    "<<times[s].raw()<<"\n";
        fileOut<< "\tID Lookup:                  "<<obj_ids[s].toString()<<"\n";
        fileOut<< "\tObject on:                  "<<sID_objectOn[s]<<"\n";
        fileOut<< "\tObjects still in queue:     "<<stillInQueues[s]<<"\n";
        fileOut<< "\tTime taken:                 "<<dTimes[s]<<"\n";


        if (times[s].raw() > processAfterInMicro)
        {
          ++numCountedLatency;
          totalLatency = totalLatency + (int) dTimes[s];

          if ((int)dTimes[s] > maxLatency)
          {
            maxLatency = dTimes[s];
          }
        }
      }



      fileOut<<"\n\n************************************\n";
      fileOut<<"\tAvg latency:  "<< totalLatency/(numCountedLatency)<<"\n\n";
      fileOut<<"\tMax latency:  "<< maxLatency<<"\n\n";
      fileOut<<"\tCounted:      "<< numCountedLatency<<"\n\n";
      fileOut<<"\n\n\n\nEND\n";
    }
  }




  //processAfter is in seconds
  void ObjectSegmentationProcessedRequestsAnalysis::printData(std::ostream &fileOut, bool sortedByTime, int processAfter)
  {
    double processAfterInMicro = processAfter* OSEG_SECOND_TO_RAW_CONVERSION_FACTOR; //convert seconds of processAfter to microseconds for comparison to time.raw().
    double totalLatency = 0;
    int maxLatency = 0;
    double numCountedLatency  = 0;

    if (sortedByTime)
    {
      std::vector<ObjectLookupProcessedEvent> sortedEvts;
      convertToEvtsAndSort(sortedEvts);

      fileOut << "\n\n*******************Begin Lookup Processed Requests Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< sortedEvts.size() <<"  \n\n";

      for (int s= 0; s < (int) sortedEvts.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:           "<<sortedEvts[s].mID_processor<<"\n";
        fileOut<< "\tTime at:                   "<<sortedEvts[s].time.raw()<<"\n";
        fileOut<< "\tID Lookup:                 "<<sortedEvts[s].mObjID.toString()<<"\n";
        fileOut<< "\tObject on:                 "<<sortedEvts[s].mID_objectOn<<"\n";
        fileOut<< "\tObjects still in queues:   "<<sortedEvts[s].stillInQueue<<"\n";
        fileOut<< "\tTime taken:                "<<sortedEvts[s].deltaTime<<"\n";

        if (sortedEvts[s].time.raw() > processAfterInMicro)
        {
          ++numCountedLatency;
          totalLatency = totalLatency + (int)sortedEvts[s].deltaTime;

          if ((int)sortedEvts[s].deltaTime > maxLatency)
          {
            maxLatency = (int)sortedEvts[s].deltaTime;
          }
        }
      }

     fileOut<<"\n\n************************************\n";
     fileOut<<"\tAvg latency:  "<< totalLatency/(numCountedLatency)<<"\n\n";
     fileOut<<"\tMax latency:  "<< maxLatency<<"\n\n";
     fileOut<<"\tCounted:      "<< numCountedLatency<<"\n\n";
     fileOut<<"\n\n\n\nEND\n";
    }
    else
    {
      fileOut << "\n\n*******************Begin Lookup Processed Requests Messages*************\n\n\n";
      fileOut << "\n\n Basic statistics:   "<< times.size() <<"  \n\n";

      for (int s= 0; s < (int) times.size(); ++s)
      {
        fileOut<< "\n\n********************************\n";
        fileOut<< "\tRegistered from:            "<<sID_processor[s]<<"\n";
        fileOut<< "\tTime at:                    "<<times[s].raw()<<"\n";
        fileOut<< "\tID Lookup:                  "<<obj_ids[s].toString()<<"\n";
        fileOut<< "\tObject on:                  "<<sID_objectOn[s]<<"\n";
        fileOut<< "\tObjects still in queue:     "<<stillInQueues[s]<<"\n";
        fileOut<< "\tTime taken:                 "<<dTimes[s]<<"\n";


        if (times[s].raw() > processAfterInMicro)
        {
          ++numCountedLatency;
          totalLatency = totalLatency + (int) dTimes[s];

          if ((int)dTimes[s] > maxLatency)
          {
            maxLatency = dTimes[s];
          }
        }
      }



      fileOut<<"\n\n************************************\n";
      fileOut<<"\tAvg latency:  "<< totalLatency/(numCountedLatency)<<"\n\n";
      fileOut<<"\tMax latency:  "<< maxLatency<<"\n\n";
      fileOut<<"\tCounted:      "<< numCountedLatency<<"\n\n";
      fileOut<<"\n\n\n\nEND\n";
    }
  }

  void ObjectSegmentationProcessedRequestsAnalysis::convertToEvtsAndSort(std::vector<ObjectLookupProcessedEvent>&sortedEvts)
  {
    ObjectLookupProcessedEvent olpe;

    for (int s= 0; s < (int) times.size(); ++s)
    {
      olpe.time             = times[s];
      olpe.mObjID           = obj_ids[s];
      olpe.mID_processor    = sID_processor[s];
      olpe.mID_objectOn     = sID_objectOn[s];
      olpe.deltaTime        = dTimes[s];
      olpe.stillInQueue      = stillInQueues[s];

      sortedEvts.push_back(olpe);
    }
    std::sort(sortedEvts.begin(),sortedEvts.end(), compareEvts );
  }

  bool ObjectSegmentationProcessedRequestsAnalysis::compareEvts(ObjectLookupProcessedEvent A, ObjectLookupProcessedEvent B)
  {
    return A.time < B.time;
  }

  ObjectSegmentationProcessedRequestsAnalysis::~ObjectSegmentationProcessedRequestsAnalysis ()
  {
  }

  ////ObjectMigrationRoundTripAnalysis
  ObjectMigrationRoundTripAnalysis::ObjectMigrationRoundTripAnalysis(const char* opt_name, const uint32 nservers)
  {
    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;

        ObjectMigrationRoundTripEvent* obj_rdt_evt = dynamic_cast<ObjectMigrationRoundTripEvent*> (evt);

        if (obj_rdt_evt != NULL)
        {
          ObjectMigrationRoundTripEvent rdt_evt = (*obj_rdt_evt);
          allRoundTripEvts.push_back(rdt_evt);
          delete evt;
          continue;
        }
        delete evt;
      }
    }
  }


  ObjectMigrationRoundTripAnalysis::~ObjectMigrationRoundTripAnalysis()
  {
    //destructor
  }

  //processedAfter is in seconds
  void ObjectMigrationRoundTripAnalysis::printData(std::ostream &fileOut, int processedAfter)
  {
    double processedAfterInMicro = processedAfter*OSEG_SECOND_TO_RAW_CONVERSION_FACTOR; //convert to microseconds

    std::sort(allRoundTripEvts.begin(), allRoundTripEvts.end(), compareEvts);

    fileOut << "\n\n*******************Begin Object Migration Round Trip Analysis*************\n\n\n";
    fileOut << "\n\n Basic statistics:   "<< allRoundTripEvts.size() <<"  \n\n";

    double totalTimes         =  0;
    double numProcessedAfter  =  0;

    for (int s=0; s < (int) allRoundTripEvts.size(); ++s)
    {
      fileOut<< "\n\n********************************\n";
      fileOut<< "\tObject id          " << allRoundTripEvts[s].obj_id.toString() <<"\n";
      fileOut<< "\tTime at:           " << allRoundTripEvts[s].time.raw()        <<"\n";
      fileOut<< "\tMigrating from:    " << allRoundTripEvts[s].sID_migratingFrom <<"\n";
      fileOut<< "\tMigrating to:      " << allRoundTripEvts[s].sID_migratingTo   <<"\n";
      fileOut<< "\tTime taken:        " << allRoundTripEvts[s].numMill           <<"\n";

      if (allRoundTripEvts[s].time.raw() > processedAfterInMicro)
      {
        totalTimes = totalTimes + ((double) allRoundTripEvts[s].numMill);
        ++numProcessedAfter;
      }

    }

    //    double avgTime = totalTimes/((double) allRoundTripEvts.size());
    double avgTime = totalTimes/((double) numProcessedAfter);

    fileOut<< "\n\n Avg Migrate Time:    "<<avgTime<<"\n";
    fileOut<< "Num processed after  " << processedAfter<<" seconds:       "<<numProcessedAfter<<"\n\n";
    fileOut<< "END*******\n\n";
  }


  bool ObjectMigrationRoundTripAnalysis::compareEvts (ObjectMigrationRoundTripEvent A, ObjectMigrationRoundTripEvent B)
  {
    return A.time < B.time;
  }




  //osegtrackedsetresultsanalysis
  OSegTrackedSetResultsAnalysis::OSegTrackedSetResultsAnalysis(const char* opt_name, const uint32 nservers)
  {
    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;

        OSegTrackedSetResultsEvent* oseg_tracked_evt = dynamic_cast<OSegTrackedSetResultsEvent*> (evt);

        if (oseg_tracked_evt != NULL)
        {
          OSegTrackedSetResultsEvent oseg_tracked_evter = (*oseg_tracked_evt);
          allTrackedSetResultsEvts.push_back(oseg_tracked_evter);
          delete evt;
          continue;
        }
        delete evt;
      }
    }
  }

  void OSegTrackedSetResultsAnalysis::printData(std::ostream &fileOut, int processAfter)
  {
    double processedAfterInMicro = processAfter*OSEG_SECOND_TO_RAW_CONVERSION_FACTOR; //convert from seconds to microseconds


    fileOut<<"\n\n*********************************\n";
    fileOut<<"\tNum tracked: "<<allTrackedSetResultsEvts.size()<<"\n\n\n\n";

    double averager = 0;
    double numProcessedAfter = 0;

    for(int s=0;s < (int) allTrackedSetResultsEvts.size(); ++s)
    {
      fileOut<<"\n\n******************\n";
      fileOut<<"\tobj_id:      "<<allTrackedSetResultsEvts[s].obj_id.toString()<<"\n";
      fileOut<<"\ttime at:     "<<allTrackedSetResultsEvts[s].time.raw()<<"\n";
      fileOut<<"\tmig_to:      "<<allTrackedSetResultsEvts[s].sID_migratingTo<<"\n";
      fileOut<<"\tnum ms:      "<<allTrackedSetResultsEvts[s].numMill<<"\n\n";


      if (allTrackedSetResultsEvts[s].time.raw() > processedAfterInMicro)
      {
        ++numProcessedAfter;
        averager = averager + allTrackedSetResultsEvts[s].numMill;
      }
    }

    if (allTrackedSetResultsEvts.size() != 0)
    {
      fileOut<<"\n\n\nAvg:  "<<averager/(numProcessedAfter)<<"\n";
      fileOut<<"num proc after "<< processAfter << "seconds:   "<< numProcessedAfter<<"\n\n";
    }

    fileOut<<"\n\n\tEND****";

  }
  OSegTrackedSetResultsAnalysis::~OSegTrackedSetResultsAnalysis()
  {

  }
  bool OSegTrackedSetResultsAnalysis::compareEvts(OSegTrackedSetResultsEvent A, OSegTrackedSetResultsEvent B)
  {
    return A.time < B.time;
  }
  //still need to fill in above




  OSegShutdownAnalysis::OSegShutdownAnalysis(const char* opt_name, const uint32 nservers)
  {
    for(uint32 server_id = 1; server_id <= nservers; server_id++)
    {
      String loc_file = GetPerServerFile(opt_name, server_id);
      std::ifstream is(loc_file.c_str(), std::ios::in);

      while(is)
      {
        Event* evt = Event::read(is, server_id);
        if (evt == NULL)
          break;

        OSegShutdownEvent* oseg_shutdown_evt = dynamic_cast<OSegShutdownEvent*> (evt);

        if (oseg_shutdown_evt != NULL)
        {
          OSegShutdownEvent oseg_shutdown_evter = (*oseg_shutdown_evt);
          allShutdownEvts.push_back(oseg_shutdown_evter);
          delete evt;
          continue;
        }
        delete evt;
      }
    }
  }
  OSegShutdownAnalysis::~OSegShutdownAnalysis()
  {

  }


  void OSegShutdownAnalysis::printData(std::ostream &fileOut)
  {
    fileOut << "\n\n*******************OSegShutdown Analysis*************\n\n\n";
    fileOut << "\n\n Basic statistics:   "<< allShutdownEvts.size() <<"  \n\n";

    for(int s=0; s < (int) allShutdownEvts.size(); ++s)
    {
      fileOut << "\n\n\n";
      fileOut << "ServerID:            "<<allShutdownEvts[s].sID<<"\n";
      fileOut << "\tnumLookups:        "<<allShutdownEvts[s].numLookups<<"\n";
      fileOut << "\tnumOnThisServer:   "<<allShutdownEvts[s].numOnThisServer<<"\n";
      fileOut << "\tnumCacheHits:      "<<allShutdownEvts[s].numCacheHits<<"\n";
      fileOut << "\tnumCraqLookups:    "<<allShutdownEvts[s].numCraqLookups<<"\n";
      fileOut << "\tnumTimeElapsedCacheEviction:   "<<allShutdownEvts[s].numTimeElapsedCacheEviction  << "\n";
      fileOut << "\tnumMigrationNotCompleteYet:    "<<allShutdownEvts[s].numMigrationNotCompleteYet   << "\n";

    }

    fileOut<<"\n\n\n\n\n";
    fileOut<<"\tEND*****";


  }



OSegCacheResponseAnalysis::OSegCacheResponseAnalysis(const char* opt_name, const uint32 nservers)
{
  for(uint32 server_id = 1; server_id <= nservers; server_id++)
  {
    String loc_file = GetPerServerFile(opt_name, server_id);
    std::ifstream is(loc_file.c_str(), std::ios::in);

    while(is)
    {
      Event* evt = Event::read(is, server_id);
      if (evt == NULL)
        break;

      OSegCacheResponseEvent* oseg_cache_evt = dynamic_cast<OSegCacheResponseEvent*> (evt);

      if (oseg_cache_evt != NULL)
      {
        OSegCacheResponseEvent oseg_cache_evter = (*oseg_cache_evt);
        allCacheResponseEvts.push_back(oseg_cache_evter);
        delete evt;
        continue;
      }
      delete evt;
    }
  }
}


OSegCacheResponseAnalysis::~OSegCacheResponseAnalysis()
{

}


void OSegCacheResponseAnalysis::printData(std::ostream &fileOut, int processAfter)
{
  double numProcessedAfter     = processAfter * OSEG_SECOND_TO_RAW_CONVERSION_FACTOR;
  double numCacheRespCataloged =                      0;

  std::sort(allCacheResponseEvts.begin(), allCacheResponseEvts.end(), compareEvts);

  fileOut << "\n\n*******************OSegCache Analysis*************\n\n\n";
  fileOut << "\n\n Basic statistics:   "<< allCacheResponseEvts.size() <<"  \n\n";


  for (int s=0; s < (int)allCacheResponseEvts.size(); ++s)
  {
    fileOut << "\n\n\n";
    fileOut << "Time:            "<<allCacheResponseEvts[s].time.raw()<<"\n";
    fileOut << "\tobj_id:        "<<allCacheResponseEvts[s].obj_id.toString()<<"\n";
    fileOut << "\tserver_id:     "<<allCacheResponseEvts[s].cacheResponseID<<"\n";

    if (allCacheResponseEvts[s].time.raw() > numProcessedAfter)
    {
      ++numCacheRespCataloged;
    }

  }

  fileOut<<"\n\n\n\n\n";
  fileOut<<"Numcache responses:  "<<numCacheRespCataloged<<"\n\n";
  fileOut<<"\tEND*****";
}

bool OSegCacheResponseAnalysis::compareEvts(OSegCacheResponseEvent A, OSegCacheResponseEvent B)
{


  return A.time < B.time;
}


//oseg cache error analysis


OSegCacheErrorAnalysis::OSegCacheErrorAnalysis(const char* opt_name, const uint32 nservers)
{
  for(uint32 server_id = 1; server_id <= nservers; server_id++)
  {
    String loc_file = GetPerServerFile(opt_name, server_id);
    std::ifstream is(loc_file.c_str(), std::ios::in);

    while(is)
    {
      Event* evt = Event::read(is, server_id);
      if (evt == NULL)
        break;

      ObjectMigrationRoundTripEvent* oseg_rd_trip_evt = dynamic_cast<ObjectMigrationRoundTripEvent*> (evt);
      if (oseg_rd_trip_evt != NULL)
      {
        mMigrationVector.push_back(*oseg_rd_trip_evt);
        delete evt;
        continue;
      }

      ObjectLookupProcessedEvent* oseg_lookup_proc_evt = dynamic_cast<ObjectLookupProcessedEvent*> (evt);
      if (oseg_lookup_proc_evt != NULL)
      {
        mLookupVector.push_back(*oseg_lookup_proc_evt);
        delete evt;
        continue;
      }

      OSegCacheResponseEvent* oseg_cache_evt = dynamic_cast<OSegCacheResponseEvent*> (evt);
      if (oseg_cache_evt != NULL)
      {
        mCacheResponseVector.push_back(*oseg_cache_evt);
        delete evt;
        continue;
      }

      ObjectLookupNotOnServerEvent* oseg_lookup_not_on_server_evt = dynamic_cast<ObjectLookupNotOnServerEvent*> (evt);
      if (oseg_lookup_not_on_server_evt != NULL)
      {
        mObjectLookupNotOnServerVector.push_back(*oseg_lookup_not_on_server_evt);
        delete evt;
        continue;
      }

      delete evt;
    }
  }
}


bool OSegCacheErrorAnalysis::compareRoundTripEvents(ObjectMigrationRoundTripEvent A, ObjectMigrationRoundTripEvent B)
{
  return A.time < B.time;
}

bool OSegCacheErrorAnalysis::compareLookupProcessedEvents(ObjectLookupProcessedEvent A, ObjectLookupProcessedEvent B)
{
  return A.time < B.time;
}

bool OSegCacheErrorAnalysis::compareCacheResponseEvents(OSegCacheResponseEvent A, OSegCacheResponseEvent B)
{
  return A.time < B.time;
}

void OSegCacheErrorAnalysis::buildObjectMap()
{
  //sort all vectors first
  std::sort(mMigrationVector.begin(), mMigrationVector.end(), compareRoundTripEvents);
  std::sort(mLookupVector.begin(), mLookupVector.end(), compareLookupProcessedEvents);
  std::sort(mCacheResponseVector.begin(), mCacheResponseVector.end(), compareCacheResponseEvents);


  //First read through all object migrations.
  std::vector< ObjectMigrationRoundTripEvent >::const_iterator migrationIterator;

  for(migrationIterator =  mMigrationVector.begin(); migrationIterator != mMigrationVector.end(); ++migrationIterator)
  {
    ServerIDTimePair sidtime;
    sidtime.sID = migrationIterator->sID_migratingTo;
    sidtime.t   = migrationIterator->time;

    mObjLoc[migrationIterator->obj_id].push_back(sidtime);
  }

  //now, read through the lookup vector.  If an object does not exist in map, then put it in.

  std::vector<ObjectLookupProcessedEvent >::const_iterator lookupIterator;
  for (lookupIterator = mLookupVector.begin(); lookupIterator != mLookupVector.end(); ++lookupIterator)
  {
    ServerIDTimePair sidtime;
    sidtime.sID = lookupIterator->mID_objectOn;
    sidtime.t   = lookupIterator->time;

    //    if (mObjLoc[lookupIterator->mObjID] == mObjLoc.end())
    if (mObjLoc.find(lookupIterator->mObjID) == mObjLoc.end())
    {
      //means that we don't have any record of the object yet.
      mObjLoc[lookupIterator->mObjID].push_back(sidtime);
    }
  }
}

void OSegCacheErrorAnalysis::analyzeMisses(Results& res, double processedAfterInMicro)
{

  buildObjectMap(); //populates mObjLoc

  //initialize results struct
  res.missesCache  = 0;
  res.hitsCache    = 0;
  res.totalCache   = 0;
  res.totalLookups = 0;

  //need to read through all the cached responses and see how many hits and misses there are.
  //
  std::vector< OSegCacheResponseEvent >::const_iterator  cacheResponseIterator;

  for (cacheResponseIterator = mCacheResponseVector.begin(); cacheResponseIterator != mCacheResponseVector.end(); ++cacheResponseIterator)
  {
    OSegCacheResponseEvent os_cache_resp = (*cacheResponseIterator);
    UUID obj_ider   = os_cache_resp.obj_id;
    Time ter        = os_cache_resp.time;
    ServerID sIDer  = os_cache_resp.cacheResponseID;

    if (os_cache_resp.time.raw() >= processedAfterInMicro)
    {
      if (checkHit(obj_ider, ter, sIDer))
      {
        ++res.hitsCache;
      }
      else
      {
        ++res.missesCache;
      }
      ++res.totalCache;
    }
  }


  //this counts the number of lookups made
  std::vector< ObjectLookupNotOnServerEvent>::const_iterator lookupNotOnServerIter;
  for (lookupNotOnServerIter =  mObjectLookupNotOnServerVector.begin();  lookupNotOnServerIter != mObjectLookupNotOnServerVector.end(); ++lookupNotOnServerIter)
  {
    if (lookupNotOnServerIter->time.raw() >= processedAfterInMicro )
    {
      ++res.totalLookups;
    }
  }
}



bool OSegCacheErrorAnalysis::checkHit(const UUID& obj_ider, const Time& ter, const ServerID& sID)
{
  bool previousCorrect = true;

  LocationList locList = mObjLoc[obj_ider];
  LocationList::const_iterator locListIterator;
  for (locListIterator = locList.begin(); locListIterator != locList.end(); ++locListIterator)
  {
    locListIterator->sID; //serverID
    locListIterator->t;   //time

    if (locListIterator->t.raw() > ter.raw())  //means that we returned the value sID before the migration occured.
    {

      return previousCorrect;
    }

    previousCorrect = (locListIterator->sID == sID);
  }
  return previousCorrect;
}


//processedAfter is in seconds.
void OSegCacheErrorAnalysis::printData(std::ostream& fileOut , int processedAfter)
{
  double processedAfterInMicro  =  processedAfter*OSEG_SECOND_TO_RAW_CONVERSION_FACTOR;  //convert seconds to microseconds

  Results res;

  analyzeMisses(res,processedAfterInMicro);

  fileOut << "\n\nOSegCacheErrorAnalysis\n" ;
  fileOut << "\t Basic statistics: \n ";
  fileOut << "\t After "<< processedAfter << " second mark\n";
  fileOut << "\t\t Total Lookups:  "<< res.totalLookups<<"\n";
  fileOut << "\t\t TotalCache:     "<< res.totalCache<<"\n";
  fileOut << "\t\t Hits:           "<< res.hitsCache<<"\n";
  fileOut << "\t\t Misses:         "<< res.missesCache<<"\n\n\n";
  fileOut << "Statistics:   \n";
  fileOut << "\tFrac of Lookups from Cache:              " << ((double)res.totalCache)/((double)res.totalLookups) <<"\n" ;
  fileOut << "\tFrac of Lookups Correct from Cache:      " << ((double)res.hitsCache)/((double)res.totalLookups) <<"\n" ;
  fileOut << "\tFrac of Lookups Incorrect from Cache:    " << ((double)res.missesCache)/((double)res.totalLookups) <<"\n" ;
  fileOut << "\tFrac of Lookups Cache Lookups Correct:   " << ((double)res.hitsCache)/((double)res.totalCache) <<"\n" ;
  fileOut << "\tFrac of Lookups Cache Lookups Incorrect: " << ((double)res.missesCache)/((double)res.totalCache) <<"\n" ;

  fileOut <<"\n\n\nEND****";

}

//simple destructor
OSegCacheErrorAnalysis::~OSegCacheErrorAnalysis()
{

}


//end oseg cache error analysis




//end oseg analysis




/** A motion path reconstructed from an event stream. */
class TraceEventMotionPath {
public:
    TraceEventMotionPath()
     : dirty(false)
    {}

    void add(GeneratedLocationEvent* evt) {
        dirty = true;
        events.push_back(evt);
    }

    MotionVector3f at(const Time& t) {
        if (dirty) {
            if (!events.empty())
                std::sort(events.begin(), events.end(), EventTimeComparator());
            dirty = false;
        }

        if (events.empty() || t < events[0]->loc.time())
            return MotionVector3f(Vector3f(0,0,0), Vector3f(0,0,0));

        uint32 i = 0;
        while(i+1 < events.size() && t >= events[i+1]->loc.time() )
            i++;

        if (i >= events.size())
            return MotionVector3f(Vector3f(0,0,0), Vector3f(0,0,0));

        return events[i]->loc.extrapolate(t);
    }
private:
    bool dirty;
    std::vector<GeneratedLocationEvent*> events;
};

void LocationLatencyAnalysis(const char* opt_name, const uint32 nservers) {
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        typedef std::vector<Event*> EventList;
        typedef std::map<UUID, EventList*> EventListMap;
        // Source Object -> List of Location Events
        EventListMap locEvents;
        // Object -> Motion path
        typedef std::map<UUID, TraceEventMotionPath*> MotionPathMap;
        MotionPathMap paths;

        // Extract all loc and gen loc events
        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;

            GeneratedLocationEvent* gen_loc_evt = dynamic_cast<GeneratedLocationEvent*>(evt);
            LocationEvent* loc_evt = dynamic_cast<LocationEvent*>(evt);

            UUID source = UUID::null();
            if (gen_loc_evt != NULL) source = gen_loc_evt->source;
            if (loc_evt != NULL) source = loc_evt->source;

            if (gen_loc_evt || loc_evt) {
                if (locEvents.find(source) == locEvents.end())
                    locEvents[source] = new EventList();
                locEvents[source]->push_back(evt);

                if (gen_loc_evt) {
                    if (paths.find(source) == paths.end())
                        paths[source] = new TraceEventMotionPath();
                    paths[source]->add(gen_loc_evt);
                }

                continue;
            }
            else {
                delete evt;
            }
        }


        String llfile = GetPerServerString("loc_latency.txt", server_id);
        std::ofstream os(llfile.c_str(), std::ios::out);

        for(EventListMap::iterator it = locEvents.begin(); it != locEvents.end(); it++) {
            UUID objid = it->first;
            EventList* evt_list = it->second;

            // Sort all sets
            std::sort(evt_list->begin(), evt_list->end(), EventTimeComparator());

            for(EventList::iterator gen_loc_it = evt_list->begin(); gen_loc_it != evt_list->end(); gen_loc_it++) {
                GeneratedLocationEvent* gen_loc_evt = dynamic_cast<GeneratedLocationEvent*>(*gen_loc_it);
                if (gen_loc_evt == NULL) continue;

                //os << objid.toString() << " " << (gen_loc_evt->loc.updateTime()-Time::null()).toSeconds() << " " << gen_loc_evt->loc.position() << std::endl;
                for(EventList::iterator loc_it = gen_loc_it; loc_it != evt_list->end(); loc_it++) {
                    LocationEvent* loc_evt = dynamic_cast<LocationEvent*>(*loc_it);
                    if (loc_evt == NULL) continue;

                    // Make sure the updates match, because we loop over all loc updates after this and we might pass the
                    // next gen_loc_evt but we can't stop looking because the latencies might be very high
                    if ( (loc_evt->loc.updateTime() != gen_loc_evt->loc.updateTime()) ||
                        (loc_evt->loc.position() != gen_loc_evt->loc.position()) ||
                        (loc_evt->loc.velocity() != gen_loc_evt->loc.velocity()) )
                        continue;


                    MotionVector3f receiver_loc = paths[loc_evt->receiver]->at( gen_loc_evt->time );
                    MotionVector3f source_loc = loc_evt->loc.extrapolate( gen_loc_evt->time );

                    Duration latency = loc_evt->time - gen_loc_evt->time;

                    // Since we don't capture initial setup locations, we need to handle the case when the first update
                    // is after this
                    if ( (receiver_loc.position() == Vector3f(0,0,0) && receiver_loc.position() == Vector3f(0,0,0)) ||
                        (source_loc.position() == Vector3f(0,0,0) && source_loc.position() == Vector3f(0,0,0)) )
                        continue;

                    os << (source_loc.position() - receiver_loc.position()).length() << " " << latency.toSeconds() << std::endl;
                }
            }
        }

        os.close();

    }
}

void ProximityDumpAnalysis(const char* opt_name, const uint32 nservers, const String& outfilename) {
    typedef std::vector<ProximityEvent*> ProxEventList;
    ProxEventList prox_events;

    // Get all prox events for all servers
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String prox_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(prox_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;

            ProximityEvent* prox_evt = dynamic_cast<ProximityEvent*>(evt);
            if (prox_evt != NULL)
                prox_events.push_back(prox_evt);
            else
                delete evt;
        }
    }

    // Sort by event time
    std::sort(prox_events.begin(), prox_events.end(), EventTimeComparator());

    // And dump them
    std::ofstream os(outfilename.c_str(), std::ios::out);
    for(ProxEventList::iterator it = prox_events.begin(); it != prox_events.end(); it++) {
        ProximityEvent* evt = *it;
        const char* dir = evt->entered ? " in " : " out ";
        os << (evt->time-Time::null()).toMicroseconds()
           << evt->source.toString() << " "
           << evt->receiver.toString()
           << dir
           << evt->loc.position() << " "
           << evt->loc.velocity() << " "
           << (evt->loc.time()-Time::null()).toMicroseconds()
           << std::endl;
    }
}



OSegCumulativeTraceAnalysis::OSegCumulativeTraceAnalysis(const char* opt_name, const uint32 nservers, uint64 time_after)
  : mInitialTime(0)
{
  for(uint32 server_id = 1; server_id <= nservers; server_id++)
  {
    String loc_file = GetPerServerFile(opt_name, server_id);
    std::ifstream is(loc_file.c_str(), std::ios::in);

    while(is)
    {
      Event* evt = Event::read(is, server_id);
      if (evt == NULL)
        break;

      OSegCumulativeEvent* oseg_cum_evt = dynamic_cast<OSegCumulativeEvent*> (evt);
      if (oseg_cum_evt != NULL)
      {
        if (allTraces.size() == 0)
          mInitialTime = (oseg_cum_evt->time - Time::null()).toMicroseconds();
        else if (mInitialTime > (uint64) (oseg_cum_evt->time - Time::null()).toMicroseconds())
          mInitialTime = (oseg_cum_evt->time - Time::null()).toMicroseconds();

        allTraces.push_back(oseg_cum_evt);
        continue;
      }
      delete evt;
    }
  }

  filterShorterPath(time_after*OSEG_CUMULATIVE_ANALYSIS_SECONDS_TO_MICROSECONDS);
  generateAllData();
}


void OSegCumulativeTraceAnalysis::generateAllData()
{
  for (int s=0; s <(int) allTraces.size();  ++s)
  {
    CumulativeTraceData* toAdd = new CumulativeTraceData;
    mCumData.push_back(toAdd);
  }

  generateCacheTime();
    generateGetCraqLookupPostTime();
  generateCraqLookupTime();
  generateCraqLookupNotAlreadyLookingUpTime();
    generateManagerPostTime();
  generateManagerEnqueueTime();
  generateManagerDequeueTime();
    generateConnectionPostTime();
  generateConnectionNetworkQueryTime();
  generateConnectionNetworkTime();
    generateReturnPostTime();
  generateLookupReturnTime();
  generateCompleteLookupTime();

  sortByCompleteLookupTime();
}

void OSegCumulativeTraceAnalysis::sortByCompleteLookupTime()
{
  std::sort(mCumData.begin(), mCumData.end(), OSegCumulativeDurationComparator());
}

OSegCumulativeTraceAnalysis::~OSegCumulativeTraceAnalysis()
{
  for (int s=0;s < (int)allTraces.size(); ++s)
    delete allTraces[s];

  for (int s=0; s < (int) mCumData.size();  ++s)
    delete mCumData[s];


  mCumData.clear();
  allTraces.clear();
}

void OSegCumulativeTraceAnalysis::printDataHuman(std::ostream &fileOut)
{
  int untilVariable = mCumData.size();
  fileOut << "\n\nCumulative OSeg Analysis\n\n";

  fileOut << "\n\nComplete Lookup Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->completeLookupTime << ",";

  fileOut << "\n\nCache Check Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->cacheTime << ",";

  fileOut << "\n\nCraq Lookup Post Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->craqLookupPostTime << ",";

  fileOut << "\n\nCraq Lookup Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->craqLookupTime << ",";

  fileOut << "\n\nCraq Lookup Not Already Looking Up Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->craqLookupNotAlreadyLookingUpTime << ",";

  fileOut << "\n\nManager Post Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->managerPostTime << ",";

  fileOut << "\n\nManager Enqueue Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->managerEnqueueTime << ",";

  fileOut << "\n\nManager Dequeue Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->managerDequeueTime << ",";

  fileOut << "\n\nPost To ConnectionGet Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->connectionPostTime << ",";

  fileOut << "\n\nNetwork Query Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->connectionNetworkQueryTime << ",";

  fileOut << "\n\nNetwork Trip Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->connectionsNetworkTime << ",";

  fileOut << "\n\nReturn Post Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->returnPostTime << ",";

  fileOut << "\n\n Process Return Times\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->lookupReturnsTime << ",";

  fileOut <<"\n\n\n";
}


void OSegCumulativeTraceAnalysis::printData(std::ostream &fileOut)
{
  int untilVariable = mCumData.size();

  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->completeLookupTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->cacheTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->craqLookupPostTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->craqLookupTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->craqLookupNotAlreadyLookingUpTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->managerPostTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->managerEnqueueTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->managerDequeueTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->connectionPostTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->connectionNetworkQueryTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->connectionsNetworkTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->returnPostTime << ",";

  fileOut << "\n";
  for (int s=0; s < untilVariable; ++s)
    fileOut  << mCumData[s]->lookupReturnsTime << ",";

  fileOut <<"\n\n\n";
}





void OSegCumulativeTraceAnalysis::filterShorterPath(uint64 time_after_microseconds)
{
  std::vector<OSegCumulativeEvent*>::iterator traceIt = allTraces.begin();

  while(traceIt != allTraces.end())
  {
    if( ((*traceIt)->traceToken.notReady)                                 ||
        ((*traceIt)->traceToken.shuttingDown)                             ||
        ((*traceIt)->traceToken.deadlineExpired)                          ||
        ((*traceIt)->traceToken.notFound)                                 ||
        ((*traceIt)->traceToken.initialLookupTime                   == 0) ||
        ((*traceIt)->traceToken.checkCacheLocalBegin                == 0) ||
        ((*traceIt)->traceToken.checkCacheLocalEnd                  == 0) ||
        ((*traceIt)->traceToken.craqLookupBegin                     == 0) ||
        ((*traceIt)->traceToken.craqLookupEnd                       == 0) ||
        ((*traceIt)->traceToken.craqLookupNotAlreadyLookingUpBegin  == 0) ||
        ((*traceIt)->traceToken.craqLookupNotAlreadyLookingUpEnd    == 0) ||
        ((*traceIt)->traceToken.getManagerEnqueueBegin              == 0) ||
        ((*traceIt)->traceToken.getManagerEnqueueEnd                == 0) ||
        ((*traceIt)->traceToken.getManagerDequeued                  == 0) ||
        ((*traceIt)->traceToken.getConnectionNetworkGetBegin        == 0) ||
        ((*traceIt)->traceToken.getConnectionNetworkGetEnd          == 0) ||
        ((*traceIt)->traceToken.getConnectionNetworkReceived        == 0) ||
        ((*traceIt)->traceToken.lookupReturnBegin                   == 0) ||
        ((*traceIt)->traceToken.lookupReturnEnd                     == 0))
    {
      delete (*traceIt);
      traceIt = allTraces.erase(traceIt);
    }
    else
    {
      if (((*traceIt)->time -Time::null()).toMicroseconds()- mInitialTime < time_after_microseconds)
      {
        delete (*traceIt);
        traceIt = allTraces.erase(traceIt);
      }
      else
        ++traceIt;
    }
  }
}


void OSegCumulativeTraceAnalysis::generateCacheTime()
{
  uint64 toPush;
  for (int s= 0; s < (int)allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.checkCacheLocalEnd - allTraces[s]->traceToken.checkCacheLocalBegin;

    mCumData[s]->cacheTime = toPush;
  }
}

void OSegCumulativeTraceAnalysis::generateGetCraqLookupPostTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.craqLookupBegin -  allTraces[s]->traceToken.checkCacheLocalEnd;

    mCumData[s]->craqLookupPostTime = toPush;
  }

}
void OSegCumulativeTraceAnalysis::generateCraqLookupTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.craqLookupEnd - allTraces[s]->traceToken.craqLookupBegin;

    mCumData[s]->craqLookupTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateCraqLookupNotAlreadyLookingUpTime()
{
  uint64 toPush;
  for (int s= 0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.craqLookupNotAlreadyLookingUpEnd - allTraces[s]->traceToken.craqLookupNotAlreadyLookingUpBegin;

    mCumData[s]->craqLookupNotAlreadyLookingUpTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateManagerPostTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.getManagerEnqueueBegin - allTraces[s]->traceToken.craqLookupNotAlreadyLookingUpEnd;

    mCumData[s]->managerPostTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateManagerEnqueueTime()
{
  uint64 toPush;
  for (int s= 0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.getManagerEnqueueEnd - allTraces[s]->traceToken.getManagerEnqueueBegin;

    mCumData[s]->managerEnqueueTime = toPush;
  }

}
void OSegCumulativeTraceAnalysis::generateManagerDequeueTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.getManagerDequeued - allTraces[s]->traceToken.getManagerEnqueueEnd;

    mCumData[s]->managerDequeueTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateConnectionPostTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.getConnectionNetworkGetBegin - allTraces[s]->traceToken.getManagerDequeued;

    mCumData[s]->connectionPostTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateConnectionNetworkQueryTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.getConnectionNetworkGetEnd - allTraces[s]->traceToken.getConnectionNetworkGetBegin;

    mCumData[s]->connectionNetworkQueryTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateConnectionNetworkTime()
{
  uint64 toPush;
  for (int s= 0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.getConnectionNetworkReceived - allTraces[s]->traceToken.getConnectionNetworkGetEnd;

    mCumData[s]->connectionsNetworkTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateReturnPostTime()
{
  uint64 toPush;
  for (int s=0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.lookupReturnBegin - allTraces[s]->traceToken.getConnectionNetworkReceived;

    mCumData[s]->returnPostTime = toPush;
  }
}
void OSegCumulativeTraceAnalysis::generateLookupReturnTime()
{
  uint64 toPush;
  for(int s= 0; s < (int) allTraces.size();++s)
  {
    toPush = allTraces[s]->traceToken.lookupReturnEnd - allTraces[s]->traceToken.lookupReturnBegin;
    mCumData[s]->lookupReturnsTime = toPush;
  }
}

void OSegCumulativeTraceAnalysis::generateCompleteLookupTime()
{
  uint64 toPush;
  for(int s= 0; s < (int) allTraces.size(); ++s)
  {
    toPush = allTraces[s]->traceToken.lookupReturnEnd - allTraces[s]->traceToken.initialLookupTime;
    mCumData[s]->completeLookupTime = toPush;
  }
}



} // namespace CBR
