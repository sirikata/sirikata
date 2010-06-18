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

#ifndef _SIRIKATA_SIMOH_OPTIONS_HPP_
#define _SIRIKATA_SIMOH_OPTIONS_HPP_

#define MAX_EXTRAPOLATOR_DIST "max-extrapolator-dist"

#define STATS_OH_TRACE_FILE     "stats.oh-trace-filename"
#define STATS_SAMPLE_RATE    "stats.sample-rate"

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

#define OBJECT_SL_FILE       "object.sl-file"
#define OBJECT_SL_NUM        "object.sl-num"
#define OBJECT_SL_CENTER     "object.sl-center"

#define OBJECT_DRIFT_X             "object_drift_x"
#define OBJECT_DRIFT_Y             "object_drift_y"
#define OBJECT_DRIFT_Z             "object_drift_z"
#define OSEG_LOOKUP_QUEUE_SIZE     "oseg_lookup_queue_size"

namespace Sirikata {

void InitSimOHOptions();

} // namespace Sirikata


#endif //_SIRIKATA_CBR_OPTIONS_HPP_
