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

template<typename Value, typename TimeType>
class ExtrapolatorBase {
public:
    virtual ~ExtrapolatorBase(){}
    virtual bool needsUpdate(const TimeType&now, const Value&actualValue)const=0;
    virtual Value extrapolate(const TimeType&now)const=0;
    virtual TimeType lastUpdateTime()const=0;
    virtual ExtrapolatorBase<Value, TimeType>& updateValue(const TimeType&now, const Value&actualValue)=0;
};

template<typename Value>
class Extrapolator : public virtual ExtrapolatorBase<Value, Time> {
};


template <typename Value, typename UpdatePredicate, typename TimeType, typename DurationType>
class TimedWeightedExtrapolatorBase : public virtual ExtrapolatorBase<Value, TimeType>, protected UpdatePredicate { // Use protected inheritence for low cost inclusing, zero cost
                                                                                                                    // if the class is empty
protected:
    enum ValueTimes{PAST=0,PRESENT=1, MAXSAMPLES};
    typedef TemporalValueBase<Value, TimeType> TemporalValueType;
    TemporalValueType mValuePast;
    TemporalValueType mValuePresent;
    DurationType mFadeTime;
public:
    TimedWeightedExtrapolatorBase(const DurationType&fadeTime, const TimeType&t, const Value&actualValue, const UpdatePredicate&needsUpdate)
     : ExtrapolatorBase<Value, TimeType>(),
       UpdatePredicate(needsUpdate),
       mValuePast(t,actualValue),
       mValuePresent(t,actualValue),
       mFadeTime(fadeTime)
    {}
    virtual ~TimedWeightedExtrapolatorBase(){}
    virtual bool needsUpdate(const TimeType&now,const Value&actualValue) const{
        const UpdatePredicate* mNeedsUpdate=this;
        return (*mNeedsUpdate)(actualValue, extrapolate(now));
    }
    Value extrapolate(const TimeType&t) const {
        DurationType timeSinceUpdate=t-mValuePresent.lastUpdateTime();
        if (mFadeTime<timeSinceUpdate) {
            return mValuePresent.extrapolate(t);
        }else{
            return mValuePast.extrapolate(t)
                .blend(mValuePresent.extrapolate(t),
                       (float64)timeSinceUpdate/(float64)mFadeTime);
        }
    }
    TimeType lastUpdateTime()const{
        return mValuePresent.lastUpdateTime();
    }
    ExtrapolatorBase<Value, TimeType>& updateValue(const TimeType&t, const Value&l) {
        mValuePast=mValuePresent;
        mValuePresent.updateValue(t,l);
        return *this;
    }
};

template <typename Value, typename UpdatePredicate>
class TimedWeightedExtrapolator : public TimedWeightedExtrapolatorBase<Value, UpdatePredicate, Time, Duration>, public Extrapolator<Value> {
public:
    TimedWeightedExtrapolator(const Duration&fadeTime, const Time&t, const Value&actualValue, const UpdatePredicate&needsUpdate)
     : TimedWeightedExtrapolatorBase<Value, UpdatePredicate, Time, Duration>(fadeTime, t, actualValue, needsUpdate)
    {}
};

}
#endif
