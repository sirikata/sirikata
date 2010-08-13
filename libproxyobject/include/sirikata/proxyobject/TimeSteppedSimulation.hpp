/*  Sirikata Utilities -- Sirikata Listener Pattern
 *  TimeSteppedSimulation.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#ifndef _SIRIKATA_TIME_STEPPED_SIMULATION_HPP_
#define _SIRIKATA_TIME_STEPPED_SIMULATION_HPP_

#include <sirikata/proxyobject/ProxyCreationListener.hpp>
#include <sirikata/proxyobject/ConnectionEventListener.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/service/Context.hpp>

namespace Sirikata {

class TimeSteppedSimulation: public ProxyCreationListener, public ConnectionEventListener, public PollingService {
public:
    TimeSteppedSimulation(Context* ctx, const Duration& rate, const String& name)
     : PollingService(ctx->mainStrand, rate, ctx, name)
    {
    }
    virtual Duration desiredTickRate() const = 0;
    ///returns true if simulation should continue (false quits app)
    virtual void poll()=0;
};

}

#endif
