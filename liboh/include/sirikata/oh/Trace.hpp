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

#ifndef _SIRIKATA_LIBOH_TRACE_HPP_
#define _SIRIKATA_LIBOH_TRACE_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/core/trace/Trace.hpp>
#include <sirikata/core/util/MotionVector.hpp>

namespace Sirikata {

class SIRIKATA_OH_EXPORT OHTrace {
public:
    OHTrace(Trace::Trace* _trace)
     : mTrace(_trace)
    {
    };

    static void InitOptions();

    CREATE_TRACE_DECL(prox, const Time& t, const UUID& receiver, const UUID& source, bool entered, const TimedMotionVector3f& loc);

    CREATE_TRACE_DECL(objectConnected, const Time& t, const UUID& receiver, const ServerID& sid);
    CREATE_TRACE_DECL(objectLoc, const Time& t, const UUID& receiver, const UUID& source, const TimedMotionVector3f& loc);
    CREATE_TRACE_DECL(objectGenLoc, const Time& t, const UUID& source, const TimedMotionVector3f& loc, const BoundingSphere3f& bnds);

    CREATE_TRACE_DECL(pingCreated, const Time&sent, const UUID&src, const Time&recv, const UUID& dst, uint64 id, double distance, uint32 sz);
    CREATE_TRACE_DECL(ping, const Time&sent, const UUID&src, const Time&recv, const UUID& dst, uint64 id, double distance, uint64 uniquePacketId, uint32 sz);

    CREATE_TRACE_DECL(hitpoint, const Time&sent, const UUID&src, const Time&recv, const UUID& dst, double sentHP, double recvHP, double distance, double srcRadius, double dstRadius, uint32 sz);
private:
    Trace::Trace* mTrace;
    static OptionValue* mLogObject;
    static OptionValue* mLogPing;
};

// This version of the OHTRACE macro automatically uses mContext->trace() and
// passes mContext->simTime() as the first argument, which is the most common
// form.
#define CONTEXT_OHTRACE(___name, ...)                 \
    TRACE( mContext->ohtrace(), ___name, mContext->simTime(), __VA_ARGS__)

// This version is like the above, but you can specify the time yourself.  Use
// this if you already called Context::simTime() recently. (It also works for
// cases where the first parameter is not the current time.)
#define CONTEXT_OHTRACE_NO_TIME(___name, ...)             \
    TRACE( mContext->ohtrace(), ___name, __VA_ARGS__)


} // namespace Sirikata

#endif //_SIRIKATA_LIBOH_TRACE_HPP_
