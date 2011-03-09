/*  Sirikata
 *  TimeSeries.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/trace/TimeSeries.hpp>
#include <sirikata/core/service/Context.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Trace::TimeSeriesFactory);

namespace Sirikata {
namespace Trace {

TimeSeries::TimeSeries(Context* ctx)
 : mContext(ctx)
{
    mContext->timeSeries = this;
}

TimeSeries::~TimeSeries() {
}

void TimeSeries::report(const String& name, float64 val) {
}


namespace {
TimeSeries* createNullTimeSeries(Context* ctx, const String& opts) {
    return new TimeSeries(ctx);
}
}

TimeSeriesFactory::TimeSeriesFactory() {
    // We always add a 'null' TimeSeries factory using the default
    // implementation which just drops the data
    registerConstructor(
        "null",
        createNullTimeSeries,
        true
    );
}

TimeSeriesFactory::~TimeSeriesFactory() {
}

TimeSeriesFactory& TimeSeriesFactory::getSingleton() {
    return AutoSingleton<TimeSeriesFactory>::getSingleton();
}

void TimeSeriesFactory::destroy() {
    AutoSingleton<TimeSeriesFactory>::destroy();
}

} // namespace Trace
} // namespace Sirikata
