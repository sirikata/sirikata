/*  Sirikata
 *  Timer.hpp
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

#ifndef _SIRIKATA_TIMER_HPP_
#define _SIRIKATA_TIMER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/AtomicTypes.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

struct TimerImpl;

class SIRIKATA_EXPORT Timer {
    static Sirikata::AtomicValue<Duration> sOffset;

public:
    static void setSystemClockOffset(const Duration &skew);
    static Duration getSystemClockOffset();
    static Time getSpecifiedDate(const std::string&datestring);
    // Get offset to convert local time (the standard for most of this API) to UTC.
    static Duration getUTCOffset();

    Timer();
    ~Timer();

    void start();
    Time getTimerStarted()const;
    static Time now();
    Duration elapsed()const;

private:
    TimerImpl* mStart;
}; // class Timer

} // namespace Sirikata

#endif //_SIRIKATA_TIMER_HPP_
