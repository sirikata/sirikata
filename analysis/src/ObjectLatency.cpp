/*  Sirikata
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
#include "ObjectLatency.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

namespace Sirikata {

ObjectLatencyAnalysis::ObjectLatencyAnalysis(const char*opt_name, const uint32 nservers) {
    mNumberOfServers = nservers;
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            uint16 type_hint;
            std::string raw_evt;
            if (!read_record(is, &type_hint, &raw_evt)) break;
            Event* evt = Event::parse(type_hint, raw_evt, server_id);
            if (evt == NULL)
                break;

            {
                PingEvent* ping_evt = dynamic_cast<PingEvent*>(evt);
                if (ping_evt != NULL) {
                    mLatency.insert(
                        PingMap::value_type(ping_evt->data.distance(),ping_evt->data.received()-ping_evt->data.t()));
                }
            }
            delete evt;
        }
    }

}

void ObjectLatencyAnalysis::histogramDistanceData(uint32 numBuckets, AverageHistogram &retval) {
    if (mLatency.empty()) return;

    // Find min and max
    double min_dist = mLatency.begin()->first, max_dist = mLatency.begin()->first;
    for (PingMap::iterator it = mLatency.begin(); it != mLatency.end(); ++it) {
        double dist = it->first;
        min_dist = std::min(min_dist, dist);
        max_dist = std::max(max_dist, dist);
    }

    if (min_dist == max_dist) {
        // As a special case, if we're only getting one distance value
        // (which can happen since they are discretized by server and
        // we sometimes only send messages to objects on the same
        // server), for this case we just respond with a single
        // bucket with all items and ignor numBuckets.
        numBuckets = 1;
    }

    // Compute buckets, initialize linear buckets (by bucket index
    // rather than distance)
    double bucketWidth = (max_dist - min_dist) / numBuckets;
    std::vector<Average> buckets(numBuckets);

    // Now add samples to the buckets
    for (PingMap::iterator it = mLatency.begin(); it != mLatency.end(); ++it) {
        double sample_dist = it->first;
        Duration sample_lat = it->second;
        int curBucket =
            numBuckets == 1 ? 0 :
            std::min((int)floor((sample_dist - min_dist)/bucketWidth), (int)numBuckets-1);
        buckets[curBucket].sample(sample_lat);
    }

    // And finally map to the final output
    for(uint32 bucket_idx = 0; bucket_idx < buckets.size(); bucket_idx++) {
        retval[ min_dist + bucketWidth*bucket_idx ] = buckets[bucket_idx];
    }
}

void ObjectLatencyAnalysis::printHistogramDistanceData(std::ostream&out, uint32 numBuckets){
    AverageHistogram histogram;
    histogramDistanceData(numBuckets, histogram);
    for (AverageHistogram::iterator it = histogram.begin(); it != histogram.end(); ++it) {
        int bucket_start_dist = it->first;
        Average& bucket_avg = it->second;

        out << bucket_start_dist << ", ";
        out << bucket_avg.average();
        out << ", ";
        out << bucket_avg.average().toMicroseconds();
        out << ", " << bucket_avg.numSamples << '\n';
    }
}

void ObjectLatencyAnalysis::printTotalAverage(std::ostream&out) {
    Duration total_lat = Duration::zero();
    for (PingMap::iterator it = mLatency.begin(); it != mLatency.end(); ++it) {
        double sample_dist = it->first;
        Duration sample_lat = it->second;
        total_lat += sample_lat;
    }

    out << (total_lat / (float)mLatency.size()) << "  (" <<  mLatency.size() << " samples)\n";
}

} // namespace Sirikata
