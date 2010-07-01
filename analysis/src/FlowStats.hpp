/*  Sirikata
 *  FlowStats.hpp
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

#ifndef _SIRIKATA_FLOW_STATS_ANALYSIS_HPP_
#define _SIRIKATA_FLOW_STATS_ANALYSIS_HPP_

#include <sirikata/core/trace/Trace.hpp>
#include "RecordedMotionPath.hpp"

namespace Sirikata {

/** Generates summary statistics on a per flow basis.  A flow is data between an
 *  ordered pair of objects (source, dest).  Summary information includes
 *  weights, sent bytes, received bytes.
 */
class FlowStatsAnalysis {
public:
    FlowStatsAnalysis(const char* opt_name, const uint32 nservers);

private:
    struct ObjectInfo {
        ServerID server;
        RecordedMotionPath path;
        BoundingSphere3f bounds;
    };
    typedef std::tr1::unordered_map<UUID, ObjectInfo, UUID::Hasher> ObjectMap;
    ObjectMap mObjectMap;

    struct ObjectPair {
        ObjectPair(const UUID& s, const UUID& d)
         : source(s), dest(d)
        {}

        bool operator<(const ObjectPair& rhs) const {
            return (source < rhs.source || (source == rhs.source && dest < rhs.dest));
        }

        bool operator==(const ObjectPair& rhs) const {
            return (source == rhs.source && dest == rhs.dest);
        }

        class Hasher {
        public:
            size_t operator() (const ObjectPair& op) const {
                return *(uint32*)op.source.getArray().data() ^ *(uint32*)op.dest.getArray().data();
            }
        };

        UUID source;
        UUID dest;
    };
    struct FlowInfo {
        FlowInfo()
         : sent_count(0),
           sent_bytes(0),
           recv_count(0),
           recv_bytes(0)
        {}

        int64 sent_count;
        int64 sent_bytes;
        int64 recv_count;
        int64 recv_bytes;
    };
    typedef std::tr1::unordered_map<ObjectPair, FlowInfo, ObjectPair::Hasher> FlowMap;
    FlowMap mFlowMap;
}; // class FlowStatsAnalysis

} // namespace Sirikata

#endif //_SIRIKATA_FLOW_STATS_ANALYSIS_HPP_
