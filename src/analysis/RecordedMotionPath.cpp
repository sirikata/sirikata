/*  cbr
 *  RecordedMotionPath.cpp
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

#include "RecordedMotionPath.hpp"

namespace CBR {

TimedMotionVector3f RecordedMotionPath::DummyUpdate;

RecordedMotionPath::RecordedMotionPath() {
}

RecordedMotionPath::~RecordedMotionPath() {
}

const TimedMotionVector3f RecordedMotionPath::initial() const {
    if (mUpdates.empty())
        return DummyUpdate;

    return mUpdates[0];
}

const TimedMotionVector3f* RecordedMotionPath::nextUpdate(const Time& curtime) const {
    if (mUpdates.empty())
        return &DummyUpdate;

    for(uint32 i = 0; i < mUpdates.size(); i++)
        if (mUpdates[i].time() > curtime) return &mUpdates[i];
    return NULL;
}

const TimedMotionVector3f RecordedMotionPath::at(const Time& t) const {
    if (mUpdates.empty())
        return DummyUpdate;

    for(uint32 i = 1; i < mUpdates.size(); i++)
        if (mUpdates[i].time() > t) return mUpdates[i-1];

    return mUpdates[ mUpdates.size()-1 ];
}

void RecordedMotionPath::addUpdate(const TimedMotionVector3f& up) {
    mUpdates.push_back(up);
    // FIXME do we need to sort here just to ensure ordering?
}

} // namespace CBR
