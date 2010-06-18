/*  Sirikata
 *  Options.cpp
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

#include "Options.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/util/Time.hpp>

namespace Sirikata {

void InitAnalysisOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue("num-oh", "1", Sirikata::OptionValueType<uint32>(), "Number of object hosts."))

        .addOption(new OptionValue(ANALYSIS_LOC, "false", Sirikata::OptionValueType<bool>(), "Do a loc analysis instead of a normal run"))
        .addOption(new OptionValue(ANALYSIS_LOCVIS, "none", Sirikata::OptionValueType<String>(), "The type of visualization to run, none to disable."))
        .addOption(new OptionValue(ANALYSIS_LOCVIS_SEED, "5", Sirikata::OptionValueType<int>(), "Do a loc analysis on this object"))

        .addOption(new OptionValue(ANALYSIS_BANDWIDTH, "false", Sirikata::OptionValueType<bool>(), "Do a bandwidth analysis instead of a normal run"))
        .addOption(new OptionValue(ANALYSIS_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Do a latency analysis instead of a normal run"))

        .addOption(new OptionValue(ANALYSIS_OBJECT_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Do a object distance latency analysis instead of a normal run"))
        .addOption(new OptionValue(ANALYSIS_MESSAGE_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Do a message stage latency analysis instead of a normal run"))

        .addOption(new OptionValue(ANALYSIS_WINDOWED_BANDWIDTH, "", Sirikata::OptionValueType<String>(), "Do a windowed bandwidth analysis of the specified type: datagram, packet"))
        .addOption(new OptionValue(ANALYSIS_WINDOWED_BANDWIDTH_WINDOW, "2000ms", Sirikata::OptionValueType<Duration>(), "Size of the window in windowed bandwidth analysis"))
        .addOption(new OptionValue(ANALYSIS_WINDOWED_BANDWIDTH_RATE, "10ms", Sirikata::OptionValueType<Duration>(), "Frequency of samples in windowed bandwidth analysis, i.e. how much to slide the window by"))

        .addOption(new OptionValue(ANALYSIS_OSEG, "false", Sirikata::OptionValueType<bool>(), "Run OSEG analyses - migrates, lookups, processed lookups"))
      .addOption(new OptionValue(OSEG_ANALYZE_AFTER,"0", Sirikata::OptionValueType<int>(),"Only run the oseg analysis after this many seconds of the run have elapsed."))


        .addOption(new OptionValue(ANALYSIS_LOC_LATENCY, "false", Sirikata::OptionValueType<bool>(), "Run location latency analysis - latency of location updates"))

        .addOption(new OptionValue(ANALYSIS_PROX_DUMP, "", Sirikata::OptionValueType<String>(), "Run proximity dump analysis -- just dumps a textual form of all proximity events to the specified file"))

        .addOption(new OptionValue(ANALYSIS_FLOW_STATS, "false", Sirikata::OptionValueType<bool>(), "Get summary object pair flow statistics"))
      ;
}

} // namespace Sirikata
