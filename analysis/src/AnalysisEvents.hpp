/*  Sirikata
 *  AnalysisEvents.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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


#ifndef __SIRIKATA_ANALYSIS_EVENTS_HPP__
#define __SIRIKATA_ANALYSIS_EVENTS_HPP__

#include <sirikata/core/trace/Trace.hpp>
#include "Protocol_ObjectTrace.pbj.hpp"
#include "Protocol_OSegTrace.pbj.hpp"
#include "Protocol_MigrationTrace.pbj.hpp"
#include "Protocol_PingTrace.pbj.hpp"
#include "Protocol_DatagramTrace.pbj.hpp"
#include "Protocol_CSegTrace.pbj.hpp"
#include "Protocol_LocProxTrace.pbj.hpp"

namespace Sirikata {

/** Read a single trace record, storing the type hint in type_hint_out and the result in payload_out.*/
bool read_record(std::istream& is, uint16* type_hint_out, std::string* payload_out);

struct Event {
    static Event* parse(uint16 type_hint, const std::string& record, const ServerID& trace_server_id);

    Event()
     : time(Time::null())
    {}
    virtual ~Event() {}

    Time time;
};

struct EventTimeComparator {
    bool operator()(const Event* lhs, const Event* rhs) const {
        return (lhs->time < rhs->time);
    }
};

template<typename T>
struct PBJEvent : public Event {
    T data;
};




struct ObjectEvent : public Event {
    UUID receiver;
    UUID source;
};

struct MessageTimestampEvent : public ObjectEvent {
    uint64 uid;
    Trace::MessagePath path;
};

struct MessageCreationTimestampEvent : public MessageTimestampEvent {
    ObjectMessagePort srcport;
    ObjectMessagePort dstport;
};


// Object
typedef PBJEvent<Trace::Object::Connected> ObjectConnectedEvent;
typedef PBJEvent<Trace::Object::LocUpdate> LocationEvent;
typedef PBJEvent<Trace::Object::GeneratedLoc> GeneratedLocationEvent;
typedef PBJEvent<Trace::Object::ProxUpdate> ProximityEvent;
//OSeg
typedef PBJEvent<Trace::OSeg::CraqRequest> OSegCraqRequestEvent;
typedef PBJEvent<Trace::OSeg::ProcessedRequest> OSegProcessedRequestEvent;
typedef PBJEvent<Trace::OSeg::TrackedSetResults> OSegTrackedSetResultsEvent;
typedef PBJEvent<Trace::OSeg::Shutdown> OSegShutdownEvent;
typedef PBJEvent<Trace::OSeg::CacheResponse> OSegCacheResponseEvent;
typedef PBJEvent<Trace::OSeg::InvalidLookup> OSegInvalidLookupEvent;
typedef PBJEvent<Trace::OSeg::CumulativeResponse> OSegCumulativeResponseEvent;
//Migration
typedef PBJEvent<Trace::Migration::Begin> MigrationBeginEvent;
typedef PBJEvent<Trace::Migration::Ack> MigrationAckEvent;
typedef PBJEvent<Trace::Migration::RoundTrip> MigrationRoundTripEvent;
//Ping
typedef PBJEvent<Trace::Ping::Created> PingCreatedEvent;
typedef PBJEvent<Trace::Ping::Sent> PingEvent;
//Datagram
typedef PBJEvent<Trace::Datagram::Queued> DatagramQueuedEvent;
typedef PBJEvent<Trace::Datagram::Sent> DatagramSentEvent;
typedef PBJEvent<Trace::Datagram::Received> DatagramReceivedEvent;
//CSeg
typedef PBJEvent<Trace::CSeg::SegmentationChanged> SegmentationChangedEvent;
//LocProx
typedef PBJEvent<Trace::LocProx::LocUpdate> ServerLocUpdateEvent;
typedef PBJEvent<Trace::LocProx::ObjectEvent> ServerObjectLocUpdateEvent;
}

#endif
