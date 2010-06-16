/*  Sirikata
 *  SimpleExtrapolator.hpp
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

#ifndef _SIRIKATA_SIMPLE_EXTRAPOLATOR_HPP_
#define _SIRIKATA_SIMPLE_EXTRAPOLATOR_HPP_

#include "Extrapolation.hpp"

namespace Sirikata {

/** SimpleExtrapolator classes, which always give the most accurate information available, at the cost of possibly being discontinuous. */
template <typename Value, typename UpdatePredicate, typename TimeType, typename DurationType>
class SimpleExtrapolatorBase : public virtual Sirikata::ExtrapolatorBase<Value, TimeType>, protected UpdatePredicate {
protected:
    typedef Sirikata::TemporalValueBase<Value, TimeType> TemporalValueType;
    TemporalValueType mValue;
public:
    SimpleExtrapolatorBase(const TimeType& t, const Value& actualValue, const UpdatePredicate& needsUpdate)
     : Sirikata::ExtrapolatorBase<Value, TimeType>(),
       UpdatePredicate(needsUpdate),
       mValue(t,actualValue)
    {}
    SimpleExtrapolatorBase(const TimeType& t, const Value& actualValue)
     : Sirikata::ExtrapolatorBase<Value, TimeType>(),
       UpdatePredicate(),
       mValue(t,actualValue)
    {}

    SimpleExtrapolatorBase(const TemporalValueType& tv, const UpdatePredicate& needsUpdate)
     : Sirikata::ExtrapolatorBase<Value, TimeType>(),
       UpdatePredicate(needsUpdate),
       mValue(tv)
    {}
    SimpleExtrapolatorBase(const TemporalValueType& tv)
     : Sirikata::ExtrapolatorBase<Value, TimeType>(),
       UpdatePredicate(),
       mValue(tv)
    {}

    virtual ~SimpleExtrapolatorBase(){}
    virtual bool needsUpdate(const TimeType&now,const Value&actualValue) const{
        const UpdatePredicate* mNeedsUpdate=this;
        return (*mNeedsUpdate)(actualValue, extrapolate(now));
    }
    Value extrapolate(const TimeType&t) const {
        return mValue.extrapolate(t);
    }
    const Value&lastValue() const{
        return mValue.value();
    }
    TimeType lastUpdateTime()const{
        return mValue.time();
    }
    SimpleExtrapolatorBase& updateValue(const TimeType&t, const Value&l) {
        mValue = TemporalValueType(t,l);
        return *this;
    }
    SimpleExtrapolatorBase& resetValue(const TimeType&t, const Value&l) {
        mValue = TemporalValueType(t,l);
        return *this;
    }
    bool propertyHolds(const TimeType&when, const std::tr1::function<bool(const Value&)>&f)const{
        return f(mValue.extrapolate(when));
    }
};

template <typename Value, typename UpdatePredicate>
class SimpleExtrapolator : public SimpleExtrapolatorBase<Value, UpdatePredicate, Time, Duration>, public Extrapolator<Value> {
private:
    typedef typename SimpleExtrapolatorBase<Value, UpdatePredicate, Time, Duration>::TemporalValueType TemporalValueType;
public:
    SimpleExtrapolator(const Time& t, const Value& actualValue, const UpdatePredicate& needsUpdate)
     : SimpleExtrapolatorBase<Value, UpdatePredicate, Time, Duration>(t, actualValue, needsUpdate)
    {}
    SimpleExtrapolator(const Time& t, const Value& actualValue)
     : SimpleExtrapolatorBase<Value, UpdatePredicate, Time, Duration>(t, actualValue)
    {}
    SimpleExtrapolator(const TemporalValueType& tv, const UpdatePredicate& needsUpdate)
     : SimpleExtrapolatorBase<Value, UpdatePredicate, Time, Duration>(tv, needsUpdate)
    {}
    SimpleExtrapolator(const TemporalValueType& tv)
     : SimpleExtrapolatorBase<Value, UpdatePredicate, Time, Duration>(tv)
    {}
};

}

#endif //_SIRIKATA_SIMPLE_EXTRAPOLATOR_HPP_
