/*  Sirikata
 *  TimeSeries.hpp
 *
 *  Copyright (c) 2011, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_TIME_SERIES_HPP_
#define _SIRIKATA_TIME_SERIES_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/Factory.hpp>

namespace Sirikata {

class Context;

namespace Trace {

/** TimeSeries tracks numeric, time series data. It's very generic, just
 *  reporting a value for a given key. Keys are just strings, but are encouraged
 *  to be hierarchical, split with '.', e.g. space.server0.objects, allowing the
 *  receiver to better organize data for exploration and display. Time values
 *  are read from the current context, so you only need to specify the key and
 *  value.
 */
class SIRIKATA_EXPORT TimeSeries {
  public:
    TimeSeries(Context* ctx);
    virtual ~TimeSeries();

    virtual void report(const String& name, float64 val);

  private:
    Context* mContext;
}; // class TimeSeries

class SIRIKATA_EXPORT TimeSeriesFactory : public Factory2<TimeSeries*,Context*,const String&> {
  public:
    static TimeSeriesFactory& getSingleton();
    static void destroy();

    TimeSeriesFactory();
    ~TimeSeriesFactory();
};

} // namespace Trace
} // namespace Sirikata

#endif //_SIRIKATA_TIME_SERIES_HPP_
