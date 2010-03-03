/*  cbr
 *  ObjectLatency.cpp
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

#include "AnalysisEvents.hpp"
#include "ObjectLatency.hpp"
#include "Options.hpp"

namespace CBR {

ObjectLatencyAnalysis::ObjectLatencyAnalysis(const char*opt_name, const uint32 nservers) {
    mNumberOfServers = nservers;
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            uint16 type_hint;
            std::string raw_evt;
            read_record(is, &type_hint, &raw_evt);
            Event* evt = Event::parse(type_hint, raw_evt, server_id);
            if (evt == NULL)
                break;

            {
                PingEvent* ping_evt = dynamic_cast<PingEvent*>(evt);
                if (ping_evt != NULL) {
                    mLatency.insert(
                        std::multimap<double,Duration>::value_type(ping_evt->distance,ping_evt->end_time()-ping_evt->begin_time()));
                }
            }
            delete evt;
        }
    }

}

void ObjectLatencyAnalysis::histogramDistanceData(double bucketWidth, std::map<int, Average > &retval){
    int bucket=-1;
    int bucketSamples=0;
    for (std::multimap<double,Duration>::iterator i=mLatency.begin(),ie=mLatency.end();
         i!=ie;++i) {
        int curBucket=(int)floor(i->first/bucketWidth);
        if (curBucket!=bucket) {
            if (bucketSamples) {
                retval.find(bucket)->second.time/=(double)bucketSamples;
                retval.find(bucket)->second.numSamples=(double)bucketSamples;
            }
            bucket=curBucket;
            bucketSamples=1;
            retval.insert(std::map<int,Average>::value_type(bucket,Average(i->second)));
        }else {
            bucketSamples++;
            retval.find(bucket)->second.time+=i->second;
        }
    }
    if (bucketSamples) {
        retval.find(bucket)->second.time/=(double)bucketSamples;
        retval.find(bucket)->second.numSamples=bucketSamples;
    }
}
void ObjectLatencyAnalysis::printHistogramDistanceData(std::ostream&out, double bucketWidth){
    std::map<int,Average> histogram;
    histogramDistanceData(bucketWidth,histogram);
    for (std::map<int,Average>::iterator i=histogram.begin(),ie=histogram.end();i!=ie;++i) {
        out<<i->first*bucketWidth<<", ";
        if(i->second.time.toMicroseconds()>9999) {
            out<<i->second.time;
        }else {
            out<<i->second.time.toMicroseconds()<<"us";
        }
        out <<", ";
        out << i->second.time.toMicroseconds();
        out << ", "<<i->second.numSamples<<'\n';
    }
}

} // namespace CBR
