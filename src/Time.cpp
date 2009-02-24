/*  cbr
 *  Time.cpp
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

#include "Time.hpp"
#include "Duration.hpp"

namespace CBR {

Time::Time(uint64 since_epoch)
 : mSinceEpoch(since_epoch)
{
}

Time::Time(const Time& cpy)
 : mSinceEpoch(cpy.mSinceEpoch)
{
}

Time::~Time() {
}

Time Time::operator+(const Duration& dt) const {
    return Time(mSinceEpoch + dt.mMicrosecs);
}

Time& Time::operator+=(const Duration& dt) {
    mSinceEpoch += dt.mMicrosecs;
    return *this;
}

Time Time::operator-(const Duration& dt) const {
    return Time(mSinceEpoch - dt.mMicrosecs);
}

Time& Time::operator-=(const Duration& dt) {
    mSinceEpoch -= dt.mMicrosecs;
    return *this;
}

Duration Time::operator-(const Time& rhs) const {
    return Duration(mSinceEpoch - rhs.mSinceEpoch);
}

bool Time::operator<(const Time& rhs) const {
    return mSinceEpoch < rhs.mSinceEpoch;
}

bool Time::operator>(const Time& rhs) const {
    return mSinceEpoch > rhs.mSinceEpoch;
}

bool Time::operator==(const Time& rhs) const {
    return mSinceEpoch < rhs.mSinceEpoch;
}

} // namespace CBR
