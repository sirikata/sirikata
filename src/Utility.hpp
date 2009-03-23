/*  cbr
 *  main.cpp
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

#ifndef _CBR_UTILITY_HPP_
#define _CBR_UTILITY_HPP_

#include <sirikata/util/Platform.hpp>
#include <sirikata/util/Vector3.hpp>
#include <sirikata/util/Vector4.hpp>
#include <sirikata/util/UUID.hpp>
#include <sirikata/options/Options.hpp>
#include <sirikata/util/TemporalValue.hpp>
#include <sirikata/util/Extrapolation.hpp>

namespace CBR {

typedef Sirikata::int8 int8;
typedef Sirikata::uint8 uint8;
typedef Sirikata::int16 int16;
typedef Sirikata::uint16 uint16;
typedef Sirikata::int32 int32;
typedef Sirikata::uint32 uint32;
typedef Sirikata::int64 int64;
typedef Sirikata::uint64 uint64;

typedef Sirikata::float32 float32;
typedef Sirikata::float64 float64;

typedef Sirikata::Vector3f Vector3f;
typedef Sirikata::Vector3d Vector3d;

typedef Sirikata::Vector3<uint32> Vector3ui32;
typedef Sirikata::Vector3<int32> Vector3i32;

typedef Sirikata::Vector4f Vector4f;
typedef Sirikata::Vector4d Vector4d;

typedef Sirikata::Vector4<uint32> Vector4ui32;
typedef Sirikata::Vector4<int32> Vector4i32;

typedef Sirikata::Quaternion Quaternion;

typedef Sirikata::UUID UUID;

typedef std::string String;

typedef Sirikata::OptionSet OptionSet;
typedef Sirikata::OptionValue OptionValue;
typedef Sirikata::InitializeClassOptions InitializeOptions;
typedef uint32 ServerID;


class Time;
class Duration;

/* CBR Derivations of TemporalValue and Extrapolator classes, using our Time and Duration classes. */

template <typename Value>
class TemporalValue : public Sirikata::TemporalValueBase<Value, Time> {
public:
    TemporalValue()
     : Sirikata::TemporalValueBase<Value, Time>( Time(0), Value() )
    {}
    TemporalValue(const Time& when, const Value& l)
     : Sirikata::TemporalValueBase<Value, Time>(when, l)
    {}
}; // class TemporalValue


template<typename Value>
class Extrapolator : public virtual Sirikata::ExtrapolatorBase<Value, Time> {
}; // class Extrapolator

template <typename Value, typename UpdatePredicate>
class TimedWeightedExtrapolator : public Sirikata::TimedWeightedExtrapolatorBase<Value, UpdatePredicate, Time, Duration>, public Extrapolator<Value> {
public:
    TimedWeightedExtrapolator(const Duration&fadeTime, const Time&t, const Value&actualValue, const UpdatePredicate&needsUpdate)
     : Sirikata::TimedWeightedExtrapolatorBase<Value, UpdatePredicate, Time, Duration>(fadeTime, t, actualValue, needsUpdate)
    {}
}; // class TimedWeightedExtrapolator


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

} // namespace CBR


#endif //_CBR_UTILITY_HPP_
