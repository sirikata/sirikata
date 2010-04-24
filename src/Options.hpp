/*  cbr
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

#ifndef _CBR_OPTIONS_HPP_
#define _CBR_OPTIONS_HPP_

#include "Utility.hpp"

#define CBR_MODULE "cbr"

#define MAX_EXTRAPOLATOR_DIST "max-extrapolator-dist"

#define STATS_TRACE_FILE     "stats.trace-filename"
#define STATS_OH_TRACE_FILE     "stats.oh-trace-filename"
#define STATS_SAMPLE_RATE    "stats.sample-rate"

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

#define OBJECT_NUM_RANDOM    "object.num.random"
#define OBJECT_CONNECT_PHASE "object.connect"
#define OBJECT_STATIC        "object.static"
#define OBJECT_SIMPLE        "object.simple"
#define OBJECT_2D            "object.2d"
#define OBJECT_QUERY_FRAC    "object.query-frac"
#define OBJECT_PACK_DIR      "object.pack-dir"
#define OBJECT_PACK          "object.pack"
#define OBJECT_PACK_OFFSET   "object.pack-offset"
#define OBJECT_PACK_NUM      "object.pack-num"
#define OBJECT_PACK_DUMP     "object.pack-dump"

#define SERVER_QUEUE         "server.queue"
#define SERVER_QUEUE_LENGTH  "server.queue.length"
#define SERVER_RECEIVER      "server.receiver"
#define SERVER_ODP_FLOW_SCHEDULER   "server.odp.flowsched"

#define NETWORK_TYPE         "net"

#define SEND_BANDWIDTH       "send-bandwidth"
#define RECEIVE_BANDWIDTH       "receive-bandwidth"

#define CSEG                "cseg"

#define LOC                        "loc"
#define LOC_MAX_PER_RESULT         "loc.max-per-result"

#define PROFILE                    "profile"

#define OSEG                       "oseg"
#define OSEG_OPTION_CRAQ           "oseg_craq"
#define OSEG_UNIQUE_CRAQ_PREFIX    "oseg_unique_craq_prefix"
#define OSEG_CACHE_SIZE              "oseg-cache-size"
#define OSEG_CACHE_CLEAN_GROUP_SIZE  "oseg-cache-clean-group-size"
#define OSEG_CACHE_ENTRY_LIFETIME    "oseg-cache-entry-lifetime"



#define OBJECT_DRIFT_X             "object_drift_x"
#define OBJECT_DRIFT_Y             "object_drift_y"
#define OBJECT_DRIFT_Z             "object_drift_z"
#define OSEG_ANALYZE_AFTER         "oseg_analyze_after"
#define OSEG_LOOKUP_QUEUE_SIZE     "oseg_lookup_queue_size"


#define PROX_MAX_PER_RESULT        "prox.max-per-result"


namespace CBR {

void InitOptions();
void ParseOptions(int argc, char** argv);
OptionValue* GetOption(const char* name);


String GetPerServerString(const String& orig, const ServerID& sid);
/** Get an option which is a filename and modify it to be server specific. */
String GetPerServerFile(const char* opt_name, const ServerID& sid);
String GetPerServerFile(const char* opt_name, const ObjectHostID& ohid);

} // namespace CBR


#endif //_CBR_OPTIONS_HPP_
