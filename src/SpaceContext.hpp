/*  cbr
 *  SpaceContext.hpp
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

#ifndef _CBR_SPACE_CONTEXT_HPP_
#define _CBR_SPACE_CONTEXT_HPP_

#include "Utility.hpp"
#include "ServerNetwork.hpp"
#include "Timer.hpp"
#include "TimeProfiler.hpp"

namespace CBR {

class MessageRouter;
class MessageDispatcher;
class Trace;
class Forwarder;

/** SpaceContext holds a number of useful items that are effectively global
 *  for each space node and used throughout the system -- ServerID, time information,
 *  MessageRouter (sending messages), MessageDispatcher (subscribe/unsubscribe
 *  from messages), and a Trace object.
 */
class SpaceContext {
public:
    SpaceContext(ServerID _id, const Time& epoch, const Time& curtime, Trace* _trace)
     : lastTime(curtime),
       time(curtime),
       sinceLast(Duration::seconds(0)),
       profiler( new TimeProfiler("Space") ),
       mID(_id),
       mEpoch(epoch),
       mRouter(NULL),
       mDispatcher(NULL),
       mTrace(_trace)
    {
    }

    // NOTE: not atomic, should only be used from the main thread currently
    void tick(const Time& t) {
        lastTime = time;
        time = t;
        sinceLast = time - lastTime;
    }


    ServerID id() const {
        return mID.read();
    }

    Time epoch() const {
        return mEpoch.read();
    }
    Duration sinceEpoch(const Time& rawtime) const {
        return rawtime - mEpoch.read();
    }
    Time simTime(const Duration& sinceStart) const {
        return Time::null() + sinceStart;
    }
    Time simTime(const Time& rawTime) const {
        return simTime( sinceEpoch(rawTime) );
    }
    // WARNING: The evaluates Timer::now, which shouldn't be done too often
    Time simTime() const {
        return simTime( Timer::now() );
    }

    MessageRouter* router() const {
        return mRouter.read();
    }
    MessageDispatcher* dispatcher() const {
        return mDispatcher.read();
    }

    Trace* trace() const {
        return mTrace.read();
    }


    // NOTE: these are not thread-safe, should only be used from the main thread
    Time lastTime;
    Time time;
    Duration sinceLast;

    TimeProfiler* profiler;
private:
    friend class Forwarder; // Allow forwarder to set mRouter and mDispatcher

    Sirikata::AtomicValue<ServerID> mID;

    Sirikata::AtomicValue<Time> mEpoch;

    Sirikata::AtomicValue<MessageRouter*> mRouter;
    Sirikata::AtomicValue<MessageDispatcher*> mDispatcher;

    Sirikata::AtomicValue<Trace*> mTrace;
}; // class SpaceContext

} // namespace CBR

#endif //_CBR_SPACE_CONTEXT_HPP_
