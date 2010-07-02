/*  Sirikata
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

#ifndef _SIRIKATA_POLLING_SERVICE_HPP_
#define _SIRIKATA_POLLING_SERVICE_HPP_

#include "Poller.hpp"
#include "TimeProfiler.hpp"

namespace Sirikata {

class Context;

/** A service which needs to be polled periodically.  This class handles
 *  scheduling and polling the service.
 */
class SIRIKATA_EXPORT PollingService : public Poller {
public:
    PollingService(Network::IOStrand* str, const Duration& max_rate = Duration::microseconds(0), Context* ctx = NULL, const String& name = "");
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

} // namespace Sirikata

#endif //_SIRIKATA_POLLING_SERVICE_HPP_
