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
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/util/RegionWeightCalculator.hpp>
#include "Protocol_ObjectTrace.pbj.hpp"
#include "Protocol_PingTrace.pbj.hpp"

namespace Sirikata {

typedef PBJEvent<Trace::Object::Connected> ObjectConnectedEvent;
typedef PBJEvent<Trace::Object::GeneratedLoc> GeneratedLocationEvent;
typedef PBJEvent<Trace::Ping::HitPoint> HitPointEvent;


FlowStatsAnalysis::FlowStatsAnalysis(const char* opt_name, const uint32 nservers) {
    RegionWeightCalculator* swc =
        RegionWeightCalculatorFactory::getSingleton().getConstructor(GetOptionValue<String>(OPT_REGION_WEIGHT))(GetOptionValue<String>(OPT_REGION_WEIGHT_ARGS))
;
    Time smallestHitPointTime(Time::epoch());
    bool firstHitPointSample=true;
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            uint16 type_hint;
            std::string raw_evt;
            if (!read_record(is, &type_hint, &raw_evt)) break;
            if (!is) break;
            Event* evt = Event::parse(type_hint, raw_evt, server_id);
            if (evt == NULL)
                break;

            {
                ObjectConnectedEvent* conn_evt = dynamic_cast<ObjectConnectedEvent*>(evt);
                if (conn_evt != NULL) {
                    mObjectMap[conn_evt->data.source()].server = conn_evt->data.server();
                }
            }
            {
                GeneratedLocationEvent* gen_loc_evt = dynamic_cast<GeneratedLocationEvent*>(evt);
                if (gen_loc_evt != NULL) {
                    mObjectMap[gen_loc_evt->data.source()].path.add(gen_loc_evt);
                    mObjectMap[gen_loc_evt->data.source()].bounds = gen_loc_evt->data.bounds();
                }
            }
            {
                PingCreatedEvent* ping_evt = dynamic_cast<PingCreatedEvent*>(evt);
                if (ping_evt != NULL) {
                    mFlowMap[ ObjectPair(ping_evt->data.sender(),ping_evt->data.receiver()) ].sent_count++;
                    mFlowMap[ ObjectPair(ping_evt->data.sender(),ping_evt->data.receiver()) ].sent_bytes += ping_evt->data.size();
                }
            }
            {
                PingEvent* ping_evt = dynamic_cast<PingEvent*>(evt);
                if (ping_evt != NULL) {
                    mFlowMap[ ObjectPair(ping_evt->data.sender(),ping_evt->data.receiver()) ].recv_count++;
                    mFlowMap[ ObjectPair(ping_evt->data.sender(),ping_evt->data.receiver()) ].recv_bytes += ping_evt->data.size();
                }
            }

            {
                HitPointEvent* ping_evt = dynamic_cast<HitPointEvent*>(evt);
                if (ping_evt != NULL) {
                    HitPointInfo * hpi=NULL;
                    ObjectPair pair(ping_evt->data.sender(),ping_evt->data.receiver());
                    if (mHitPointMap.find(pair)==mHitPointMap.end()) {
                        
                        ObjectMap::iterator source_it = mObjectMap.find(ping_evt->data.sender());
                        ObjectMap::iterator dest_it = mObjectMap.find(ping_evt->data.receiver());
                        if (source_it!=mObjectMap.end()&&dest_it!=mObjectMap.end()) {
                            hpi=&mHitPointMap[pair];
                            hpi->distance=ping_evt->data.distance();
                            TimedMotionVector3f start1 = source_it->second.path.initial();
                            TimedMotionVector3f start2 = dest_it->second.path.initial();
                            
                            BoundingBox3f world_bounds1 = BoundingBox3f(source_it->second.bounds.center() + start1.position(), source_it->second.bounds.radius());
                            BoundingBox3f world_bounds2 = BoundingBox3f(dest_it->second.bounds.center() + start2.position(), dest_it->second.bounds.radius());
                            double priority = swc->weight(world_bounds1, world_bounds2);


                            hpi->weight=priority;
                        }else {
                            SILOG(analysis,error, "Unable to find "<<ping_evt->data.sender().toString()<<" and/or "<<ping_evt->data.receiver().toString());
                        }
                    }else {
                        hpi=&mHitPointMap[pair];
                    }
                    hpi->samples.push_back(HitPointInfo::Sample(ping_evt->data.t(),ping_evt->data.received()));
                    
                    hpi->samples.back().starthp=ping_evt->data.sent_hp();
                    hpi->samples.back().endhp=ping_evt->data.actual_hp();
                    if(firstHitPointSample) 
                        smallestHitPointTime=ping_evt->data.t();
                    else if (smallestHitPointTime>ping_evt->data.t()) {
                        smallestHitPointTime=ping_evt->data.t();
                    }
                    firstHitPointSample=false;
                    
                }
            }
            delete evt;
        }
    }
    if (mHitPointMap.size()) {
        FILE * fp = fopen("hitpointstats.txt","w");
        if (fp )  {
            fprintf(fp,"Distance, Weight, sent, received, senthp, receivedhp\n");
            for (HitPointMap::const_iterator i=mHitPointMap.begin(),ie=mHitPointMap.end();i!=ie;++i) {
                fprintf(fp,"%f",i->second.distance);
                fprintf(fp,",  %f",i->second.weight);
                
                for (size_t j=0;j<i->second.samples.size();++j) {
                    HitPointInfo::Sample  s= i->second.samples[j];
                    fprintf(fp,", %f, %f, %f, %f",
                            (s.start-smallestHitPointTime).toSeconds(),
                            (s.end-smallestHitPointTime).toSeconds(),
                            s.starthp,
                            s.endhp);
                            
                }
                fprintf(fp,"\n");
            }
            fclose(fp);
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
