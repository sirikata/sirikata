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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/Simulation.hpp>
#include <sirikata/core/service/Poller.hpp>
#include <sirikata/core/service/Context.hpp>
#include <sirikata/core/service/TimeProfiler.hpp>

namespace Sirikata {

// Simulation which wants a periodic callback, e.g. for rendering.
//
// To avoid double-inheritance from Service, this uses Poller instead of
// PollingService. This unfortunately means it essentially duplicates that
// implementation.
class TimeSteppedSimulation : public Simulation, public Poller {
public:
    TimeSteppedSimulation(
        Context* ctx, const Duration& rate, const String& name,
        Network::IOStrand* strand, const char* cb_tag, bool accurate=false)
     : Poller(strand, std::tr1::bind(&TimeSteppedSimulation::indirectPoll, this), cb_tag, rate, accurate),
       mProfiler(NULL)
    {
        if (ctx != NULL && !name.empty())
            mProfiler = ctx->profiler->addStage(name);
    }
    ~TimeSteppedSimulation() {
        delete mProfiler;
    }

    virtual void start() {
        Poller::start();
    }
    virtual void stop() {
        shutdown();
        Poller::stop();
    }

protected:
    /** Override this method to specify the work to be done when polling. */
    virtual void poll() = 0;

    /** Override this method to clean up when a shutdown is requested. */
    virtual void shutdown() {}

private:
    void indirectPoll() {
        if (mProfiler != NULL)
            mProfiler->started();

        poll();

        if (mProfiler != NULL)
            mProfiler->finished();
    }

    TimeProfiler::Stage* mProfiler;
};

}

#endif
