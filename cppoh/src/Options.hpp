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

#ifndef _SIRIKATA_CPPOH_OPTIONS_HPP_
#define _SIRIKATA_CPPOH_OPTIONS_HPP_

#define OPT_CONFIG_FILE          "cfg"

#define OPT_OH_PLUGINS           "oh.plugins"

#define STATS_OH_TRACE_FILE     "stats.oh-trace-filename"
#define STATS_SAMPLE_RATE    "stats.sample-rate"

#define OPT_OH_OPTIONS           "objecthost"
#define OPT_MAIN_SPACE           "mainspace"
#define OPT_SPACEID_MAP          "spaceidmap"

#define OPT_SIGFPE               "sigfpe"

#define OPT_OBJECT_FACTORY       "object-factory"
#define OPT_OBJECT_FACTORY_OPTS  "object-factory-opts"


#define OPT_CAMERASCRIPT         "camerascript"
#define OPT_CAMERASCRIPTTYPE         "camerascripttype"


namespace Sirikata {

void InitCPPOHOptions();

} // namespace Sirikata


#endif //_SIRIKATA_CPPOH_OPTIONS_HPP_
