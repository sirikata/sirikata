/*  cbr
 *  Context.hpp
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

#ifndef _SIRIKATA_CONTEXT_HPP_
#define _SIRIKATA_CONTEXT_HPP_

#include "Utility.hpp"
#include "Timer.hpp"
#include "TimeProfiler.hpp"
#include "PollingService.hpp"

#define FORCE_MONOTONIC_CLOCK 1

namespace Sirikata {

class Trace;

/** Base class for Contexts, provides basic infrastructure such as IOServices,
 *  IOStrands, Trace, and timing information.
 */
class Context : public Service {
public:

    Context(const String& name, IOService* ios, IOStrand* strand, Trace* _trace, const Time& epoch, const Duration& simlen);
    ~Context();

    Time epoch() const {
        return mEpoch.read();
    }
    Duration sinceEpoch(const Time& rawtime) const {
        return rawtime - mEpoch.read();
    }
    Time simTime(const Duration& sinceStart) const {
        if (sinceStart.toMicroseconds() < 0)
            return Time::null();
        return Time::null() + sinceStart;
    }
    Time simTime(const Time& rawTime) const {
        return simTime( sinceEpoch(rawTime) );
    }
    // WARNING: The evaluates Timer::now, which shouldn't be done too often
    Time simTime() const {
        Time curt = simTime( Timer::now() );

#if FORCE_MONOTONIC_CLOCK
        Time last = mLastSimTime.read();
        if (curt < last)
            curt = last;

        // FIXME this is to avoid const fallout since this didn't used to
        // need to modify data
        const_cast< Sirikata::AtomicValue<Time>& >(this->mLastSimTime) = curt;
#endif
        return curt;
    }

    // A low cost alternative to simTime(). Gets that last value simTime()
    // returned. This should be used when a Time is needed, but the cost of
    // calling simTime() is too high. Obviously not everybody can use this or
    // simTime() will never progress.
    Time recentSimTime() const {
        return this->mLastSimTime.read();
    }

    void add(Service* ps) {
        mServices.push_back(ps);
        ps->start();
    }

    void run(uint32 nthreads = 1) {
        std::vector<Thread*> workerThreads;

        // Start workers
        for(uint32 i = 1; i < nthreads; i++) {
            workerThreads.push_back(
                new Thread( std::tr1::bind(&IOService::run, ioService) )
            );
        }

        // Run
        ioService->run();

        // Wait for workers to finish
        for(uint32 i = 0; i < workerThreads.size(); i++) {
            workerThreads[i]->join();
            delete workerThreads[i];
        }
        workerThreads.clear();
    }

    // Call after run returns to ensure all resources get cleaned up.
    void cleanup();

    Trace* trace() const {
        return mTrace;
    }

    IOService* ioService;
    IOStrand* mainStrand;
    TimeProfiler* profiler;
protected:

    // Main Lifetime Management
    virtual void start();
    void stopSimulation();
    virtual void stop();
    IOTimerPtr mFinishedTimer;


    // Forced Quit Management
    void startForceQuitTimer();

    // Forces quit by stopping event processing.  Should only be
    // invoked after waiting a sufficient period after an initial stop
    // request.
    void forceQuit() {
        SILOG(forcequit,fatal,"[FORCEQUIT] Fatal error: Quit forced by timeout.");
        ioService->stop();
    }

    Trace* mTrace;

    Sirikata::AtomicValue<Time> mEpoch;
    Sirikata::AtomicValue<Time> mLastSimTime;
    Duration mSimDuration;
    std::vector<Service*> mServices;

    std::tr1::shared_ptr<Thread> mKillThread;
    IOService* mKillService;
    IOTimerPtr mKillTimer;
}; // class ObjectHostContext

} // namespace Sirikata


#endif //_SIRIKATA_CONTEXT_HPP_
