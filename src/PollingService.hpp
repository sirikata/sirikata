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

namespace CBR {

/** A service which needs to be polled periodically.  This class handles
 *  scheduling and polling the service.
 */
class PollingService {
public:
    PollingService(IOStrand* str, const Duration& max_rate = Duration::microseconds(0))
     : mStrand(str),
       mMaxRate(max_rate),
       mUnschedule(false)
    {
    }

    /** Start polling this service on this strand at the given maximum rate. */
    void start() {
        mStrand->post( std::tr1::bind(&PollingService::handleExec, this) );
    }

    /** Stop scheduling this service. Note that this does not immediately
     *  stop the service, it simply guarantees the service will not
     *  be scheduled again.  This allows outstanding events to be handled
     *  properly.
     */
    void stop() {
        mUnschedule = true;
    }
protected:
    /** Override this method to specify the work to be done when polling. */
    virtual void poll() = 0;

private:

    void handleExec() {
        this->poll();

        if (!mUnschedule)
            mStrand->post( std::tr1::bind(&PollingService::handleExec, this) );
    }

    IOStrand* mStrand;
    Duration mMaxRate;
    bool mUnschedule;
};

} // namespace CBR

#endif //_CBR_POLLING_SERVICE_HPP_
