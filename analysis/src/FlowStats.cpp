/*  Sirikata
 *  FlowStats.cpp
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

#include "AnalysisEvents.hpp"
#include "FlowStats.hpp"
#include <sirikata/cbrcore/Options.hpp>
#include <sirikata/core/util/RegionWeightCalculator.hpp>

namespace Sirikata {

FlowStatsAnalysis::FlowStatsAnalysis(const char* opt_name, const uint32 nservers) {
    RegionWeightCalculator* swc =
        RegionWeightCalculatorFactory::getSingleton().getConstructor(GetOption(OPT_REGION_WEIGHT)->as<String>())(GetOption(OPT_REGION_WEIGHT_ARGS)->as<String>())
;

    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            uint16 type_hint;
            std::string raw_evt;
            read_record(is, &type_hint, &raw_evt);
            if (!is) break;
            Event* evt = Event::parse(type_hint, raw_evt, server_id);
            if (evt == NULL)
                break;

            {
                ObjectConnectedEvent* conn_evt = dynamic_cast<ObjectConnectedEvent*>(evt);
                if (conn_evt != NULL) {
                    mObjectMap[conn_evt->source].server = conn_evt->server;
                }
            }
            {
                GeneratedLocationEvent* gen_loc_evt = dynamic_cast<GeneratedLocationEvent*>(evt);
                if (gen_loc_evt != NULL) {
                    mObjectMap[gen_loc_evt->source].path.add(gen_loc_evt);
                    mObjectMap[gen_loc_evt->source].bounds = gen_loc_evt->bounds;
                }
            }
            {
                PingCreatedEvent* ping_evt = dynamic_cast<PingCreatedEvent*>(evt);
                if (ping_evt != NULL) {
                    mFlowMap[ ObjectPair(ping_evt->source,ping_evt->receiver) ].sent_count++;
                    mFlowMap[ ObjectPair(ping_evt->source,ping_evt->receiver) ].sent_bytes += ping_evt->size;
                }
            }
            {
                PingEvent* ping_evt = dynamic_cast<PingEvent*>(evt);
                if (ping_evt != NULL) {
                    mFlowMap[ ObjectPair(ping_evt->source,ping_evt->receiver) ].recv_count++;
                    mFlowMap[ ObjectPair(ping_evt->source,ping_evt->receiver) ].recv_bytes += ping_evt->size;
                }
            }
            delete evt;
        }
    }

    // Header
    SILOG(flowstats,fatal,"Distance, Rad1, Server1, Rad2, Server2, ServerOnlyPriority, DistanceOnlyPriority, Priority, ReceivedCount, SentCount");
    for(FlowMap::iterator it = mFlowMap.begin(); it != mFlowMap.end(); it++) {
        ObjectMap::iterator source_it = mObjectMap.find(it->first.source);
        ObjectMap::iterator dest_it = mObjectMap.find(it->first.dest);

        TimedMotionVector3f start1 = source_it->second.path.initial();
        TimedMotionVector3f start2 = dest_it->second.path.initial();

        BoundingBox3f world_bounds1 = BoundingBox3f(source_it->second.bounds.center() + start1.position(), source_it->second.bounds.radius());
        BoundingBox3f world_bounds2 = BoundingBox3f(dest_it->second.bounds.center() + start2.position(), dest_it->second.bounds.radius());

        double distance = (start1.position()-start2.position()).length();
        double rad1 = source_it->second.bounds.radius();
        ServerID server1 = source_it->second.server;
        double rad2 = dest_it->second.bounds.radius();
        ServerID server2 = dest_it->second.server;

        double server_priority = 0.0; // FIXME
        double distance_priority = 0.0; // FIXME
        double priority = swc->weight(world_bounds1, world_bounds2);
        uint64 recv_bytes = it->second.recv_bytes;
        uint64 sent_bytes = it->second.sent_bytes;

        SILOG(flowstats,fatal,
            distance << ", " <<
            rad1 << ", " <<
            server1 << ", " <<
            rad2 << ", " <<
            server2 << ", " <<
            server_priority << ", " <<
            distance_priority << ", " <<
            priority << ", " <<
            recv_bytes << ", " <<
            sent_bytes
        );
    }

    delete swc;
}

} // namespace Sirikata
