/*  Sirikata
 *  Trace.hpp
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

#ifndef _SIRIKATA_SIMOH_TRACE_HPP_
#define _SIRIKATA_SIMOH_TRACE_HPP_

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/space/OSegLookupTraceToken.hpp>

namespace Sirikata {

class SIRIKATA_SPACE_EXPORT SpaceTrace {
public:
    SpaceTrace(Trace::Trace* _trace)
     : mTrace(_trace)
    {
    };

    static void InitOptions();

    // OSeg
    CREATE_TRACE_DECL(objectSegmentationCraqLookupRequest, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo);
    CREATE_TRACE_DECL(objectSegmentationLookupNotOnServerRequest, const Time& t, const UUID& obj_id, const ServerID &sID_lookupTo);
    CREATE_TRACE_DECL(objectSegmentationProcessedRequest, const Time&t, const UUID& obj_id, const ServerID &sID, const ServerID & sID_processor, uint32 dTime, uint32 stillInQueue);
    CREATE_TRACE_DECL(processOSegTrackedSetResults, const Time &t, const UUID& obj_id, const ServerID& sID_migratingTo, const Duration& dur);
    CREATE_TRACE_DECL(processOSegShutdownEvents, const Time &t, const ServerID& sID, const int& num_lookups, const int& num_on_this_server, const int& num_cache_hits, const int& num_craq_lookups, const int& num_time_elapsed_cache_eviction, const int& num_migration_not_complete_yet);
    CREATE_TRACE_DECL(osegCacheResponse, const Time &t, const ServerID& sID, const UUID& obj);
    CREATE_TRACE_DECL(osegCumulativeResponse, const Time &t, OSegLookupTraceToken* traceToken);

    // Migration
    CREATE_TRACE_DECL(objectBeginMigrate, const Time& t, const UUID& ojb_id, const ServerID migrate_from, const ServerID migrate_to);
    CREATE_TRACE_DECL(objectAcknowledgeMigrate, const Time& t, const UUID& obj_id, const ServerID& acknowledge_from,const ServerID& acknowledge_to);
    CREATE_TRACE_DECL(objectMigrationRoundTrip, const Time& t, const UUID& obj_id, const ServerID &sID_migratingFrom, const ServerID& sID_migratingTo, const Duration& round_trip);

    // Datagram
    CREATE_TRACE_DECL(serverDatagramQueued, const Time& t, const ServerID& dest, uint64 id, uint32 size);
    CREATE_TRACE_DECL(serverDatagramSent, const Time& start_time, const Time& end_time, float weight, const ServerID& dest, uint64 id, uint32 size);
    CREATE_TRACE_DECL(serverDatagramReceived, const Time& start_time, const Time& end_time, const ServerID& src, uint64 id, uint32 size);

    // Loc/Prox
    CREATE_TRACE_DECL(serverLoc, const Time& t, const ServerID& sender, const ServerID& receiver, const UUID& obj, const TimedMotionVector3f& loc);
    CREATE_TRACE_DECL(serverObjectEvent, const Time& t, const ServerID& source, const ServerID& dest, const UUID& obj, bool added, const TimedMotionVector3f& loc);

    // CSeg
    CREATE_TRACE_DECL(segmentationChanged, const Time& t, const BoundingBox3f& bbox, const ServerID& serverID);


private:
    Trace::Trace* mTrace;
    static OptionValue* mLogOSeg;
    static OptionValue* mLogOSegCumulative;
    static OptionValue* mLogMigration;
    static OptionValue* mLogDatagram;
    static OptionValue* mLogLocProx;
    static OptionValue* mLogCSeg;
};

// This version of the SPACETRACE macro automatically uses mContext->trace() and
// passes mContext->simTime() as the first argument, which is the most common
// form.
#define CONTEXT_SPACETRACE(___name, ...)                 \
    TRACE( mContext->spacetrace(), ___name, mContext->simTime(), __VA_ARGS__)

// This version is like the above, but you can specify the time yourself.  Use
// this if you already called Context::simTime() recently. (It also works for
// cases where the first parameter is not the current time.)
#define CONTEXT_SPACETRACE_NO_TIME(___name, ...)             \
    TRACE( mContext->spacetrace(), ___name, __VA_ARGS__)


} // namespace Sirikata

#endif //_SIRIKATA_SIMOH_TRACE_HPP_
