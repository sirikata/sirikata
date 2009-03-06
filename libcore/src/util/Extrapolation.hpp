/*  Sirikata Utilities -- Math Library
 *  Extrapolation.hpp
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
#ifndef _EXTRAPOLATION_HPP_
#define _EXTRAPOLATION_HPP_
#include "TemporalValue.hpp"
namespace Sirikata {
template<typename Value> class Extrapolator {
public:
    virtual ~Extrapolator(){}
    virtual bool needsUpdate(const Time&now, const Value&actualValue)const=0;
    virtual Value extrapolate(const Time&now)const=0;
    virtual Time getLastValueUpdateTime()const=0;
    virtual Extrapolator<Value>& updateValue(const Time&now, const Value&actualValue)=0;
};

template <typename Value, typename UpdatePredicate>
class TimedWeightedExtrapolator:public Extrapolator<Value> {
    enum ValueTimes{PAST=0,PRESENT=1, MAXSAMPLES};
    TemporalValue<Value,UpdatePredicate> mValuePast;
    TemporalValue<Value,UpdatePredicate> mValuePresent;
    Duration mFadeTime;
public:
    TimedWeightedExtrapolator(const Duration&fadeTime, const Time&t, const Value&actualValue, const UpdatePredicate&needsUpdate)
    :mValuePast(t,actualValue,needsUpdate)
    ,mValuePresent(t,actualValue,needsUpdate)
    ,mFadeTime(fadeTime) {}
    virtual ~TimedWeightedExtrapolator(){}
    virtual bool needsUpdate(const Time&now,const Value&actualValue) const{
        return TemporalValue<Value,UpdatePredicate>(mValuePresent)
                 .updateValue(now,extrapolate(now))
                    .needsUpdate(now,actualValue);
    }
    Value extrapolate(const Time&t) const {
        Duration timeSinceUpdate=t-mValuePresent.getLastValueUpdateTime();
        if (mFadeTime<timeSinceUpdate) {
            return mValuePresent.extrapolate(t);
        }else{
            return mValuePast.extrapolate(t)
                .blend(mValuePresent.extrapolate(t),
                       (float64)timeSinceUpdate/(float64)mFadeTime);
        }
    }
    Time getLastValueUpdateTime()const{ 
        return mValuePresent.getLastValueUpdateTime();
    }
    Extrapolator<Value>& updateValue(const Time&t, const Value&l) {
        mValuePast=mValuePresent;
        mValuePresent.updateValue(t,l);
        return *this;
    }
};
}
#endif
