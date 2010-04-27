/*  cbr
 *  PollingService.hpp
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

#ifndef _CBR_POLLING_SERVICE_HPP_
#define _CBR_POLLING_SERVICE_HPP_

#include "Utility.hpp"
#include "TimeProfiler.hpp"

namespace CBR {

class Context;

/** A Service is simply something that runs during the main loop.
 *  It must implement methods which allow it to start and indicate
 *  when it must start shutting down.
 */
class Service {
public:
    virtual ~Service() {}

    virtual void start() = 0;
    virtual void stop() = 0;
};

/** Poller allows you to generate a callback periodically without having
 *  to inherit from the PollingService class.  It serves the same function
 *  but requires a new object for every callback instead of using an existing
 *  service.
 */
class Poller : public Service {
public:
    Poller(IOStrand* str, const IOCallback& cb, const Duration& max_rate = Duration::microseconds(0));

    /** Start polling this service on this strand at the given maximum rate. */
    virtual void start();

    /** Stop scheduling this service. Note that this does not immediately
     *  stop the service, it simply guarantees the service will not
     *  be scheduled again.  This allows outstanding events to be handled
     *  properly.
     */
    virtual void stop();
private:
    void handleExec();

    IOStrand* mStrand;
    IOTimerPtr mTimer;
    Duration mMaxRate;
    bool mUnschedule;
    IOCallback mCB; // Our callback, just saves us from reconstructing it all the time
    IOCallback mUserCB; // The user's callback
}; // class Poller

/** A service which needs to be polled periodically.  This class handles
 *  scheduling and polling the service.
 */
class PollingService : public Poller {
public:
    PollingService(IOStrand* str, const Duration& max_rate = Duration::microseconds(0), Context* ctx = NULL, const String& name = "");
    ~PollingService();

    virtual void stop();
protected:
    /** Override this method to specify the work to be done when polling. */
    virtual void poll() = 0;
    /** Override this method to clean up when a shutdown is requested. */
    virtual void shutdown() {}

private:
    void indirectPoll();
    TimeProfiler::Stage* mProfiler;
};

} // namespace CBR

#endif //_CBR_POLLING_SERVICE_HPP_
