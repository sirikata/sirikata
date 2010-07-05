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

void InitSimOHOptions() {
    InitializeClassOptions::module(SIRIKATA_OPTIONS_MODULE)
        .addOption(new OptionValue(OPT_OH_PLUGINS,"weight-exp,weight-sqr",Sirikata::OptionValueType<String>(),"Plugin list to load."))

        .addOption(new OptionValue("ohid", "1", Sirikata::OptionValueType<ObjectHostID>(), "Object host ID for this server"))

        .addOption(new OptionValue(STATS_OH_TRACE_FILE, "trace.txt", Sirikata::OptionValueType<String>(), "The filename to save the trace to"))
        .addOption(new OptionValue(STATS_SAMPLE_RATE, "250ms", Sirikata::OptionValueType<Duration>(), "Frequency to sample non-event statistics such as queue information."))

        .addOption(new OptionValue(MAX_EXTRAPOLATOR_DIST, "1.0", Sirikata::OptionValueType<float64>(), "The maximum distance an object is permitted to deviate from the predictions by other objects before an update is sent out."))

        .addOption(new OptionValue(OBJECT_NUM_RANDOM, "100", Sirikata::OptionValueType<uint32>(), "Number of random objects to generate."))

        .addOption(new OptionValue(OBJECT_CONNECT_PHASE, "0s", Sirikata::OptionValueType<Duration>(), "Length of time to initiate connections over. Connection requests will be uniformly distributed."))
        .addOption(new OptionValue(OBJECT_STATIC, "random", Sirikata::OptionValueType<String>(), "Whether objects should be static (static) or move randomly (randome) or drift in one direction (drift)."))
      .addOption(new OptionValue(OBJECT_DRIFT_X, "0",Sirikata::OptionValueType<float>(), "If select drift for motion path (under OBJECT_STATIC), then this is the x component of all objects' drifts"))
      .addOption(new OptionValue(OBJECT_DRIFT_Y, "0",Sirikata::OptionValueType<float>(), "If select drift for motion path (under OBJECT_STATIC), then this is the y component of all objects' drifts"))
      .addOption(new OptionValue(OBJECT_DRIFT_Z, "0",Sirikata::OptionValueType<float>(), "If select drift for motion path (under OBJECT_STATIC), then this is the z component of all objects' drifts"))

        .addOption(new OptionValue(OBJECT_SIMPLE, "false", Sirikata::OptionValueType<bool>(), "Simple object distribution - all the same size, useful for sanity checking queries"))
        .addOption(new OptionValue(OBJECT_2D, "false", Sirikata::OptionValueType<bool>(), "Constrain location and motion to just 2 dimensions."))
        .addOption(new OptionValue(OBJECT_QUERY_FRAC, "0.1", Sirikata::OptionValueType<float>(), "Percent of objects which should issue prox queries."))

        .addOption(new OptionValue(OBJECT_PACK_DIR, "", Sirikata::OptionValueType<String>(), "Directory to store and load pack files from."))
        .addOption(new OptionValue(OBJECT_PACK, "", Sirikata::OptionValueType<String>(), "Filename of the object pack to use to generate objects."))
        .addOption(new OptionValue(OBJECT_PACK_OFFSET, "0", Sirikata::OptionValueType<uint32>(), "Offset into the object pack to start generating objects at."))
        .addOption(new OptionValue(OBJECT_PACK_NUM, "0", Sirikata::OptionValueType<uint32>(), "Number of objects to load from a pack file."))
        .addOption(new OptionValue(OBJECT_PACK_DUMP, "", Sirikata::OptionValueType<String>(), "If non-empty, dumps any generated objects to the specified file."))

        .addOption(new OptionValue(OBJECT_SL_FILE, "", Sirikata::OptionValueType<String>(), "Filename of the object pack to use to generate objects."))
        .addOption(new OptionValue(OBJECT_SL_NUM, "0", Sirikata::OptionValueType<uint32>(), "Number of objects to load from a pack file."))
        .addOption(new OptionValue(OBJECT_SL_CENTER, "", Sirikata::OptionValueType<Vector3f>(), "The center point to start adding SL objects from. They will be added in order of increasing distance from this point."))

      .addOption(new OptionValue("scenario", "ping", Sirikata::OptionValueType<String>(), "ObjectHost-wide script dictating mass wide object behaviors"))
      .addOption(new OptionValue("scenario-options", "", Sirikata::OptionValueType<String>(), "Options for ObjectHost-wide script dictating mass wide object behaviors"))
      .addOption(new OptionValue("object-host-receive-buffer", "32768", Sirikata::OptionValueType<size_t>(), "size of the object host space node connection receive queue"))
      .addOption(new OptionValue("object-host-send-buffer", "32768", Sirikata::OptionValueType<size_t>(), "size of the object host space node cnonection send queue"))

      ;
}

} // namespace Sirikata
