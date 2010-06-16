/*  Sirikata
 *  RateEstimator.hpp
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

#ifndef _SIRIKATA_RATE_ESTIMATOR_HPP_
#define _SIRIKATA_RATE_ESTIMATOR_HPP_

#include <sirikata/core/util/Platform.hpp>

namespace Sirikata {

/** Exponential weighted average rate estimator. */
class RateEstimator {
public:
    RateEstimator()
     : _value(0),
       _t(Time::null()),
       _backlog(0)
    {}

    RateEstimator(double good_guess, const Time& start)
     : _value(good_guess),
       _t(start),
       _backlog(0)
    {}

    double get()const {
        return _value;
    }

    double get(const Time& t, double K) const {
        Duration diff = t - _t;
        double dt = diff.toSeconds();
        if (dt<1.0e-9) {
            return _value;
        }
        double blend = exp(-dt/K);
        uint32 new_bytes = _backlog;
        return _value*blend+(1-blend)*new_bytes/dt;
    }

    double estimate_rate(const Time& t, uint32 len, double K) {
        Duration diff = t - _t;
        double dt = diff.toSeconds();
        if (dt<1.0e-9) {
            _backlog += len;
            return _value;
        }
        double blend = exp(-dt/K);
        uint32 new_bytes = len + _backlog;
        _value=_value*blend+(1-blend)*new_bytes/dt;
        _t = t;
        _backlog = 0;
        return _value;
    }
private:
    double _value;
    Time _t;
    uint32 _backlog;
};

/** RateEstimator that holds its falloff parameter with it.  This should only be
 * used when the cost of storing the falloff parameter per estimator is low,
 * i.e. if the estimator is unique.
 */
class SimpleRateEstimator : public RateEstimator {
public:
    SimpleRateEstimator(double K)
     : RateEstimator(),
       _K(K)
    {}

    SimpleRateEstimator(double good_guess, const Time& start, double K)
     : RateEstimator(good_guess, start),
       _K(K)
    {}

    double estimate_rate(const Time& t, uint32 len) {
        return RateEstimator::estimate_rate(t, len, _K);
    }
private:
    double _K;
};

} // namespace Sirikata

#endif //_SIRIKATA_RATE_ESTIMATOR_HPP_
