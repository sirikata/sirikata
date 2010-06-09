/*  cbr
 *  Context.cpp
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

#include "Context.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata {

Context::Context(const String& name, IOService* ios, IOStrand* strand, Trace* _trace, const Time& epoch, const Duration& simlen)
 : ioService(ios),
   mainStrand(strand),
   profiler( new TimeProfiler(name) ),
   mFinishedTimer( IOTimer::create(ios) ),
   mTrace(_trace),
   mEpoch(epoch),
   mLastSimTime(Time::null()),
   mSimDuration(simlen),
   mKillThread(),
   mKillService(NULL),
   mKillTimer()
{
}

Context::~Context() {
}

void Context::start() {
    Time t_now = simTime();
    Time t_end = simTime(mSimDuration);
    Duration wait_dur = t_end - t_now;
    mFinishedTimer->wait(
        wait_dur,
        mainStrand->wrap(
            std::tr1::bind(&Context::stopSimulation, this)
        )
    );
}

void Context::stopSimulation() {
    this->stop();
    for(std::vector<Service*>::iterator it = mServices.begin(); it != mServices.end(); it++)
        (*it)->stop();
}

void Context::stop() {
    mFinishedTimer.reset();
    startForceQuitTimer();
}



void Context::cleanup() {
    IOTimerPtr timer = mKillTimer;

    if (timer) {
        timer->cancel();
        mKillTimer.reset();
        timer.reset();

        mKillService->stop();

        mKillThread->join();

        IOServiceFactory::destroyIOService(mKillService);
        mKillService = NULL;
        mKillThread.reset();
    }
}

void Context::startForceQuitTimer() {
    // Note that we need to do this on another thread, with another IOService.
    // This is necessary to ensure that *this* doesn't keep things from
    // exiting.
    mKillService = IOServiceFactory::makeIOService();
    mKillTimer = IOTimer::create(mKillService);
    mKillTimer->wait(
        Duration::seconds(15),
        std::tr1::bind(&Context::forceQuit, this)
    );
    mKillThread = std::tr1::shared_ptr<Thread>(
        new Thread(
            std::tr1::bind(&IOService::run, mKillService)
        )
    );
}


} // namespace Sirikata
