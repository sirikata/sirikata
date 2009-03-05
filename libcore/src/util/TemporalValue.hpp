/*  Sirikata Utilities -- Math Library
 *  TemporalLocation.hpp
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
#ifndef _TEMPORAL_LOCATION_HPP_
#define _TEMPORLA_LOCATION_HPP_

namespace Sirikata {

template <typename Value, typename UpdatePredicate> class TemporalLocation {
    Value mCurrentValue;
    Time mWhen;
    UpdatePredicate mNeedsUpdate;
public:
    TemporalLocation(){}
    TemporalLocation(const Value&l,
                     Time when,
                     const UpdatePredicate &needsUpdate):
        mCurrentValue(l),
       mWhen(when),
       mNeedsUpdate(needsUpdate){}
    bool needsUpdate(const Time&now, const Value&actualValue) {
        return mNeedsUpdate(actualValue,getLocation(now));
    }
    Value getLocation(const Time &t) const {
        return mCurrentValue.predict(t-mWhen);
    }
    Time getLastLocationUpdateTime() {
        return mWhen;
    }
    void updateLocation(const Value&l, const Time&t) {
        mCurrentValue=l;
        mWhen=t;
    }
};
}
#endif
