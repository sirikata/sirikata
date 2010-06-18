/*  Sirikata
 *  Options.hpp
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

#ifndef _SIRIKATA_ANALYSIS_OPTIONS_HPP_
#define _SIRIKATA_ANALYSIS_OPTIONS_HPP_

#define ANALYSIS_LOC         "analysis.loc"
#define ANALYSIS_LOCVIS         "analysis.locvis"
#define ANALYSIS_LOCVIS_SEED         "analysis.locvis.seed"
#define ANALYSIS_BANDWIDTH   "analysis.bandwidth"
#define ANALYSIS_LATENCY   "analysis.latency"
#define ANALYSIS_OBJECT_LATENCY   "analysis.object.latency"
#define ANALYSIS_MESSAGE_LATENCY   "analysis.message.latency"
#define ANALYSIS_WINDOWED_BANDWIDTH          "analysis.windowed-bandwidth"
#define ANALYSIS_WINDOWED_BANDWIDTH_WINDOW   "analysis.windowed-bandwidth.window"
#define ANALYSIS_WINDOWED_BANDWIDTH_RATE     "analysis.windowed-bandwidth.rate"
#define ANALYSIS_OSEG        "analysis.oseg"
#define ANALYSIS_LOC_LATENCY "analysis.loc.latency"
#define ANALYSIS_PROX_DUMP "analysis.prox.dump"
#define ANALYSIS_FLOW_STATS "analysis.flow.stats"

#define OSEG_ANALYZE_AFTER         "oseg_analyze_after"


namespace Sirikata {

void InitAnalysisOptions();

} // namespace Sirikata


#endif //_SIRIKATA_ANALYSIS_OPTIONS_HPP_
