/*  cbr
 *  Timer.cpp
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
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Timer.hpp"

namespace Sirikata {

Timer::Timer() {
}

Timer::~Timer() {
}
static boost::posix_time::ptime gEpoch(boost::posix_time::time_from_string(std::string("2009-03-12 23:59:59.000")));
Time Timer::getSpecifiedDate(const std::string&dat) {
    boost::posix_time::time_duration since_epoch=boost::posix_time::time_from_string(dat)-gEpoch;
    return Time::null() + Duration::microseconds(since_epoch.total_microseconds());
}
void Timer::start() {
    mStart = boost::posix_time::microsec_clock::local_time();
}
Time Timer::getTimerStarted() const{
    boost::posix_time::time_duration since_start =mStart-gEpoch;
    return Time::null() + Duration::microseconds(since_start.total_microseconds());
}

Sirikata::AtomicValue<Duration> Timer::sOffset(Duration::seconds(0.0));

void Timer::setSystemClockOffset(const Duration&skew) {
    sOffset=skew;
}
Duration Timer::getSystemClockOffset(){
    return sOffset;
}

Time Timer::now() {
    boost::posix_time::time_duration since_start = boost::posix_time::microsec_clock::local_time()-gEpoch;
    return Time::null() + Duration::microseconds( since_start.total_microseconds() ) + sOffset.read();
}

Duration Timer::elapsed() const{
    boost::posix_time::time_duration since_start = boost::posix_time::microsec_clock::local_time() - mStart;
    return Duration::microseconds( since_start.total_microseconds() );
}


} // namespace Sirikata
