/*  Sirikata
 *  Trace.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include "Trace.hpp"
#include "CBR_ObjectTrace.pbj.hpp"
#include "CBR_PingTrace.pbj.hpp"
#include <sirikata/core/options/Options.hpp>

namespace Sirikata {

#define TRACE_OBJECT_NAME                   "trace-object"
#define TRACE_PING_NAME                     "trace-ping"

OptionValue* OHTrace::mLogObject;
OptionValue* OHTrace::mLogPing;

void OHTrace::InitOptions() {
    mLogObject = new OptionValue(TRACE_OBJECT_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogPing = new OptionValue(TRACE_PING_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");

    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(mLogObject)
        .addOption(mLogPing)
        ;
}

static void fillTimedMotionVector(Sirikata::Trace::ITimedMotionVector tmv, const TimedMotionVector3f& val) {
    tmv.set_t(val.time());
    tmv.set_position(val.position());
    tmv.set_velocity(val.velocity());
}

// Object

CREATE_TRACE_DEF(OHTrace, prox, mLogObject, const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc) {
    Sirikata::Trace::Object::ProxUpdate pu;
    pu.set_t(t);
    pu.set_receiver(receiver);
    pu.set_source(source);
    pu.set_entered(entered);
    Sirikata::Trace::ITimedMotionVector pu_loc = pu.mutable_loc();
    fillTimedMotionVector(pu_loc, loc);

    mTrace->writeRecord(ProximityTag, pu);
}

CREATE_TRACE_DEF(OHTrace, objectConnected, mLogObject, const Time& t, const UUID& source, const ServerID& sid) {
    Sirikata::Trace::Object::Connected co;
    co.set_t(t);
    co.set_source(source);
    co.set_server(sid);

    mTrace->writeRecord(ObjectConnectedTag, co);
}

CREATE_TRACE_DEF(OHTrace, objectGenLoc, mLogObject, const Time& t, const UUID& source, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds) {
    Sirikata::Trace::Object::GeneratedLoc lu;
    lu.set_t(t);
    lu.set_source(source);
    Sirikata::Trace::ITimedMotionVector lu_loc = lu.mutable_loc();
    fillTimedMotionVector(lu_loc, loc);
    lu.set_bounds(bnds);

    mTrace->writeRecord(ObjectGeneratedLocationTag, lu);
}

CREATE_TRACE_DEF(OHTrace, objectLoc, mLogObject, const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc) {
    Sirikata::Trace::Object::LocUpdate lu;
    lu.set_t(t);
    lu.set_receiver(receiver);
    lu.set_source(source);
    Sirikata::Trace::ITimedMotionVector lu_loc = lu.mutable_loc();
    fillTimedMotionVector(lu_loc, loc);

    mTrace->writeRecord(ObjectLocationTag, lu);
}

// Ping

CREATE_TRACE_DEF(OHTrace, pingCreated, mLogPing, const Time& t, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint32 sz) {
    Sirikata::Trace::Ping::Created rec;
    rec.set_t(t);
    rec.set_sender(sender);
    rec.set_received(dst);
    rec.set_receiver(receiver);
    rec.set_ping_id(id);
    rec.set_distance(distance);
    rec.set_size(sz);

    mTrace->writeRecord(ObjectPingCreatedTag, rec);
}

CREATE_TRACE_DEF(OHTrace, ping, mLogPing, const Time& t, const UUID&sender, const Time&dst, const UUID& receiver, uint64 id, double distance, uint64 uid, uint32 sz) {
    Sirikata::Trace::Ping::Sent rec;
    rec.set_t(t);
    rec.set_sender(sender);
    rec.set_received(dst);
    rec.set_receiver(receiver);
    rec.set_ping_id(id);
    rec.set_distance(distance);
    rec.set_ping_uid(uid);
    rec.set_size(sz);

    mTrace->writeRecord(ObjectPingCreatedTag, rec);
}


} // namespace Sirikata
