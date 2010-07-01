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
#include "Protocol_OSegTrace.pbj.hpp"
#include "Protocol_MigrationTrace.pbj.hpp"
#include "Protocol_DatagramTrace.pbj.hpp"
#include "Protocol_LocProxTrace.pbj.hpp"
#include "Protocol_CSegTrace.pbj.hpp"
#include <sirikata/core/options/Options.hpp>

namespace Sirikata {

#define TRACE_OSEG_NAME                     "trace-oseg"
#define TRACE_OSEG_CUMULATIVE_NAME          "trace-oseg-cumulative"
#define TRACE_MIGRATION_NAME                "trace-migration"
#define TRACE_DATAGRAM_NAME                 "trace-datagram"
#define TRACE_LOCPROX_NAME                  "trace-locprox"
#define TRACE_CSEG_NAME                     "trace-cseg"

OptionValue* SpaceTrace::mLogOSeg;
OptionValue* SpaceTrace::mLogOSegCumulative;
OptionValue* SpaceTrace::mLogMigration;
OptionValue* SpaceTrace::mLogDatagram;
OptionValue* SpaceTrace::mLogLocProx;
OptionValue* SpaceTrace::mLogCSeg;

void SpaceTrace::InitOptions() {
    mLogOSeg = new OptionValue(TRACE_OSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogOSegCumulative = new OptionValue(TRACE_OSEG_CUMULATIVE_NAME,"true",Sirikata::OptionValueType<bool>(),"Log oseg cumulative trace data");
    mLogMigration = new OptionValue(TRACE_MIGRATION_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogDatagram = new OptionValue(TRACE_DATAGRAM_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogLocProx = new OptionValue(TRACE_LOCPROX_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");
    mLogCSeg = new OptionValue(TRACE_CSEG_NAME,"false",Sirikata::OptionValueType<bool>(),"Log object trace data");

    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(mLogOSeg)
        .addOption(mLogOSegCumulative)
        .addOption(mLogMigration)
        .addOption(mLogDatagram)
        .addOption(mLogLocProx)
        .addOption(mLogCSeg)
        ;
}


static void fillTimedMotionVector(Sirikata::Trace::ITimedMotionVector tmv, const TimedMotionVector3f& val) {
    tmv.set_t(val.time());
    tmv.set_position(val.position());
    tmv.set_velocity(val.velocity());
}


// OSeg

CREATE_TRACE_DEF(SpaceTrace, objectSegmentationCraqLookupRequest, mLogOSeg, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo){
    Sirikata::Trace::OSeg::CraqRequest rec;
    rec.set_t(t);
    rec.set_object(obj_id);
    rec.set_server(sID_lookupTo);

    mTrace->writeRecord(ObjectSegmentationCraqLookupRequestAnalysisTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, objectSegmentationProcessedRequest, mLogOSeg, const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 objectsInQueue)
{
    Sirikata::Trace::OSeg::ProcessedRequest rec;
    rec.set_t(t);
    rec.set_object(obj_id);
    rec.set_processor(sID_processor);
    rec.set_server(sID);
    rec.set_dtime(sID);
    rec.set_queued(objectsInQueue);

    mTrace->writeRecord(ObjectSegmentationProcessedRequestAnalysisTag, rec);
}


CREATE_TRACE_DEF(SpaceTrace, processOSegTrackedSetResults, mLogOSeg, const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, const Duration& dur)
{
    Sirikata::Trace::OSeg::TrackedSetResults rec;
    rec.set_t(t);
    rec.set_object(obj_id);
    rec.set_server(sID_migratingTo);
    rec.set_roundtrip(dur);

    mTrace->writeRecord(OSegTrackedSetResultAnalysisTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, processOSegShutdownEvents, mLogOSeg, const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet)
{
  std::cout<<"\n\n**********oseg shutdown:  \n";
  std::cout<<"\tsid:                              "<<sID<<"\n";
  std::cout<<"\tnum lookups:                      "<<num_lookups<<"\n";
  std::cout<<"\tnum_on_this_server:               "<<num_on_this_server<<"\n";
  std::cout<<"\tnum_cache_hits:                   "<<num_cache_hits<<"\n";
  std::cout<<"\tnum_craq_lookups:                 "<<num_craq_lookups<<"\n";
  std::cout<<"\tnum_migration_not_complete_yet:   "<< num_migration_not_complete_yet<<"\n\n";
  std::cout<<"***************************\n\n";

  Sirikata::Trace::OSeg::Shutdown rec;
  rec.set_t(t);
  rec.set_lookups(num_lookups);
  rec.set_local_lookups(num_on_this_server);
  rec.set_cache_hits(num_cache_hits);
  rec.set_craq_lookups(num_craq_lookups);
  rec.set_cache_eviction_elapsed(num_time_elapsed_cache_eviction);
  rec.set_outstanding_migrations(num_migration_not_complete_yet);

  mTrace->writeRecord(OSegShutdownEventTag, rec);
}




CREATE_TRACE_DEF(SpaceTrace, osegCacheResponse, mLogOSeg, const Time &t, const ServerID& sID, const UUID& obj_id)
{
    Sirikata::Trace::OSeg::CacheResponse rec;
    rec.set_t(t);
    rec.set_server(sID);
    rec.set_object(obj_id);

    mTrace->writeRecord(OSegCacheResponseTag, rec);
}


CREATE_TRACE_DEF(SpaceTrace, objectSegmentationLookupNotOnServerRequest, mLogOSeg, const Time& t, const UUID& obj_id, const ServerID &sID_lookerupper)
{
    Sirikata::Trace::OSeg::InvalidLookup rec;
    rec.set_t(t);
    rec.set_server(sID_lookerupper);
    rec.set_object(obj_id);

    mTrace->writeRecord(OSegLookupNotOnServerAnalysisTag, rec);
}


CREATE_TRACE_DEF(SpaceTrace, osegCumulativeResponse, mLogOSegCumulative, const Time &t, OSegLookupTraceToken* traceToken)
{
    if (traceToken == NULL)
        return;

    Sirikata::Trace::OSeg::CumulativeResponse rec;
    rec.set_t(t);
    rec.set_object(traceToken->mID);
    rec.set_lookup_server(traceToken->lookerUpper);
    rec.set_location_server(traceToken->locatedOn);

    rec.set_not_ready(traceToken->notReady);
    rec.set_shutting_down(traceToken->shuttingDown);
    rec.set_deadline_expired(traceToken->deadlineExpired);
    rec.set_not_found(traceToken->notFound);

    rec.set_initial_lookup_time(traceToken->initialLookupTime);
    rec.set_check_cache_local_begin(traceToken->checkCacheLocalBegin);
    rec.set_check_cache_local_end(traceToken->checkCacheLocalEnd);
    rec.set_qlen_post_query(traceToken->osegQLenPostQuery);
    rec.set_craq_lookup_begin(traceToken->craqLookupBegin);
    rec.set_craq_lookup_end(traceToken->craqLookupEnd);
    rec.set_craq_lookup_not_already_lookup_begin(traceToken->craqLookupNotAlreadyLookingUpBegin);
    rec.set_craq_lookup_not_already_lookup_end(traceToken->craqLookupNotAlreadyLookingUpEnd);

    rec.set_get_manager_enqueue_begin(traceToken->getManagerEnqueueBegin);
    rec.set_get_manager_enqueue_end(traceToken->getManagerEnqueueEnd);
    rec.set_get_manager_dequeued(traceToken->getManagerDequeued);
    rec.set_get_connection_network_begin(traceToken->getConnectionNetworkGetBegin);
    rec.set_get_connection_network_end(traceToken->getConnectionNetworkGetEnd);

    rec.set_get_connection_network_received(traceToken->getConnectionNetworkReceived);
    rec.set_qlen_post_return(traceToken->osegQLenPostReturn);
    rec.set_lookup_return_begin(traceToken->lookupReturnBegin);
    rec.set_lookup_return_end(traceToken->lookupReturnEnd);

    mTrace->writeRecord(OSegCumulativeTraceAnalysisTag, rec);
}

// Migration

CREATE_TRACE_DEF(SpaceTrace, objectBeginMigrate, mLogMigration, const Time& t, const UUID& obj_id, const ServerID migrate_from, const ServerID migrate_to) {
    Sirikata::Trace::Migration::Begin begin;
    begin.set_t(t);
    begin.set_object(obj_id);
    begin.set_from(migrate_from);
    begin.set_toward(migrate_to);

    mTrace->writeRecord(MigrationBeginTag, begin);
}

CREATE_TRACE_DEF(SpaceTrace, objectAcknowledgeMigrate, mLogMigration, const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to) {
    Sirikata::Trace::Migration::Ack ack;
    ack.set_t(t);
    ack.set_object(obj_id);
    ack.set_from(acknowledge_to);
    ack.set_toward(acknowledge_from);

    mTrace->writeRecord(MigrationAckTag, ack);
}

CREATE_TRACE_DEF(SpaceTrace, objectMigrationRoundTrip, mLogMigration, const Time& t, const UUID& obj_id, const ServerID &migrate_to, const ServerID& migrate_from, const Duration& round_trip)
{
    Sirikata::Trace::Migration::RoundTrip rt;
    rt.set_t(t);
    rt.set_object(obj_id);
    rt.set_from(migrate_from);
    rt.set_toward(migrate_to);
    rt.set_roundtrip(round_trip);

    mTrace->writeRecord(MigrationRoundTripTag, rt);
}

// Datagram

CREATE_TRACE_DEF(SpaceTrace, serverDatagramQueued, mLogDatagram, const Time& t, const ServerID& dest, uint64 id, uint32 size) {
    Sirikata::Trace::Datagram::Queued rec;
    rec.set_t(t);
    rec.set_dest_server(dest);
    rec.set_uid(id);
    rec.set_size(size);

    mTrace->writeRecord(ServerDatagramQueuedTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, serverDatagramSent, mLogDatagram, const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size) {
    Sirikata::Trace::Datagram::Sent rec;
    rec.set_t(start_time);
    rec.set_dest_server(dest);
    rec.set_uid(id);
    rec.set_size(size);
    rec.set_weight(weight);
    rec.set_start_time(start_time);
    rec.set_end_time(end_time);

    mTrace->writeRecord(ServerDatagramSentTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, serverDatagramReceived, mLogDatagram, const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size) {
    Sirikata::Trace::Datagram::Received rec;
    rec.set_t(start_time);
    rec.set_source_server(src);
    rec.set_uid(id);
    rec.set_size(size);
    rec.set_start_time(start_time);
    rec.set_end_time(end_time);

    mTrace->writeRecord(ServerDatagramReceivedTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, serverLoc, mLogLocProx, const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc) {
    Sirikata::Trace::LocProx::LocUpdate rec;
    rec.set_t(t);
    rec.set_sender(sender);
    rec.set_receiver(receiver);
    rec.set_object(obj);
    Sirikata::Trace::ITimedMotionVector rec_loc = rec.mutable_loc();
    fillTimedMotionVector(rec_loc, loc);

    mTrace->writeRecord(ServerLocationTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, serverObjectEvent, mLogLocProx, const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc) {
    Sirikata::Trace::LocProx::ObjectEvent rec;
    rec.set_t(t);
    rec.set_sender(source);
    rec.set_receiver(dest);
    rec.set_object(obj);
    rec.set_added(added);
    Sirikata::Trace::ITimedMotionVector rec_loc = rec.mutable_loc();
    fillTimedMotionVector(rec_loc, loc);

    mTrace->writeRecord(ServerObjectEventTag, rec);
}

CREATE_TRACE_DEF(SpaceTrace, segmentationChanged, mLogCSeg, const Time& t, const BoundingBox3f& bbox, const ServerID& serverID){
    Sirikata::Trace::CSeg::SegmentationChanged rec;
    rec.set_t(t);
    rec.set_bbox(bbox);
    rec.set_server(serverID);
    mTrace->writeRecord(SegmentationChangeTag, rec);
}


} // namespace Sirikata
