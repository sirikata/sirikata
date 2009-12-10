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
#include "Context.hpp"

namespace CBR {

class MessageRouter;
class MessageDispatcher;
class Forwarder;
class MockForwarder;

/** SpaceContext holds a number of useful items that are effectively global
 *  for each space node and used throughout the system -- ServerID, time information,
 *  MessageRouter (sending messages), MessageDispatcher (subscribe/unsubscribe
 *  from messages), and a Trace object.
 */
class SpaceContext : public Context, public PollingService {
public:
    SpaceContext(ServerID _id, IOService* ios, IOStrand* strand, const Time& epoch, const Time& curtime, Trace* _trace, const Duration& duration)
     : Context("Space", ios, strand, _trace, epoch, duration),
       PollingService(strand),
       lastTime(curtime),
       time(curtime),
       mID(_id),
       mRouter(NULL),
       mDispatcher(NULL)
    {
        mIterationProfiler = profiler->addStage("Context Iteration");
        mIterationProfiler->started();
        mWorkProfiler = profiler->addStage("Context Work");
    }

    ~SpaceContext() {
        delete mIterationProfiler;
        delete mWorkProfiler;
    }

    ServerID id() const {
        return mID.read();
    }

    MessageRouter* router() const {
        return mRouter.read();
    }
    MessageDispatcher* dispatcher() const {
        return mDispatcher.read();
    }

    // FIXME only used by vis code because it is out of date and horrible
    void tick(const Time& t) {
        lastTime = time;
        time = t;
    }

    Time lastTime;
    Time time;
private:
    virtual void poll() {
        mIterationProfiler->finished();
        mWorkProfiler->started();

        Duration elapsed = sinceEpoch( Timer::now());

        if (elapsed > mSimDuration)
        {
            this->stop();
            for(std::vector<Service*>::iterator it = mServices.begin(); it != mServices.end(); it++)
                (*it)->stop();
        }
        
        lastTime = time;
        time = Time::null() + elapsed;

        mWorkProfiler->finished();
        mIterationProfiler->started();
    }

    friend class Forwarder; // Allow forwarder to set mRouter and mDispatcher
    friend class MockForwarder; // Same for mock forwarder

    Sirikata::AtomicValue<ServerID> mID;

    Sirikata::AtomicValue<MessageRouter*> mRouter;
    Sirikata::AtomicValue<MessageDispatcher*> mDispatcher;

    TimeProfiler::Stage* mIterationProfiler;
    TimeProfiler::Stage* mWorkProfiler;
}; // class SpaceContext

} // namespace CBR

#endif //_CBR_SPACE_CONTEXT_HPP_
