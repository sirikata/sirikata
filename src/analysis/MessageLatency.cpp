/*  cbr
 *  MessageLatency.cpp
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

#include "AnalysisEvents.hpp"
#include "MessageLatency.hpp"
#include "Options.hpp"

namespace CBR {

namespace {

class PathPair {
  public:
    Trace::MessagePath first;
    Trace::MessagePath second;
    PathPair(const Trace::MessagePath &first,
             const Trace::MessagePath &second) {
        this->first=first;
        this->second=second;
    }
    bool operator < (const PathPair&other) const{
        if (first==other.first) return second<other.second;
        return first<other.first;
    }
    bool operator ==(const PathPair&other) const{
        return first==other.first&&second==other.second;
    }
};

class DTime:public Time {
  public:
    Trace::MessagePath mPath;
    bool isNull() const {
        const Time * t=this;
        return *t==Time::null();
    }
    uint32 mServerId;
    DTime():Time(Time::null()) {mServerId=0;mPath=Trace::NUM_PATHS;}
    DTime(const Time&t, Trace::MessagePath path):Time(t){mServerId=0;mPath=path;}
    bool operator < (const Time&other)const {
        return *static_cast<const Time*>(this)<other;
    }
    bool operator == (const Time&other)const {
        return *static_cast<const Time*>(this)==other;
    }
};
class Average {
    int64 sample_sum; // sum (t)
    int64 sample2_sum; // sum (t^2)
    uint32 numSamples;
  public:
    Average()
            : sample_sum(0),
              sample2_sum(0),
              numSamples(0)
    {}
    void sample(const Duration& t) {
        int64 st = t.toMicroseconds();
        sample_sum += st;
        sample2_sum += st*st;
        ++numSamples;
    }
    Duration average() {
        return Duration::microseconds((int64)(sample_sum / double(numSamples)));
    }
    Duration variance() {
        int64 avg = (int64)(sample_sum / double(numSamples));
        int64 sq_avg = (int64)(sample2_sum / double(numSamples));
        return Duration::microseconds(sq_avg - avg*avg);
    }
    Duration stddev() {
        int64 avg = (int64)(sample_sum / double(numSamples));
        int64 sq_avg = (int64)(sample2_sum / double(numSamples));
        return Duration::microseconds((int64)sqrt((double)(sq_avg - avg*avg)));
    }
    uint32 samples() const {
        return numSamples;
    }
};

// A group of tags which should be considered together since they are
// sensible combinations.  A group should either be a) a group of tags which
// are handled in the same thread of control so they should be guaranteed to
// be ordered properly or b) represent the barriers between threads of
// control, which ideally should be thin, hopefully having only one tag on
// either side.
class StageGroup {
  public:
    // Indicates how values in this group should be handled.  UNORDERED
    // allows any combination in any order to work.  ORDERED_ALLOW_NEGATIVE
    // constrains the order, but allows inverted pairs to be used with
    // negative durations, which can be useful for unsynchronized network or
    // cross-thread pairs.  ORDERED_STRICT only considers pairs that are
    // strictly in order.
    enum IsOrdered {
        UNORDERED,
        ORDERED_ALLOW_NEGATIVE,
        ORDERED_STRICT
    };

    StageGroup(const String& _name, IsOrdered ordr = UNORDERED)
            : mName(_name),
              mOrdered(ordr)
    {}

    String name() const {
        return mName;
    }
    // Returns *this for chaining
    StageGroup& add(Trace::MessagePath p) {
        assert( mTags.find(p) == mTags.end() );
        mTags.insert(p);
        mOrderedTags.push_back(p);
        return *this;
    }
    bool contains(Trace::MessagePath p) const {
        return (mTags.find(p) != mTags.end());
    }
    bool contains(const PathPair& p) const {
        return (contains(p.first) && contains(p.second));
    }

    std::vector<DTime> filter(const std::vector<DTime>& orig) {
        std::vector<DTime> result;
        for(std::vector<DTime>::const_iterator it = orig.begin(); it != orig.end(); it++)
            if (this->contains(it->mPath))
                result.push_back(*it);
        return result;
    }


    // Based on ordering constraints, get a PathPair for the two
    // records. For unordered, this will always return (start,end).  For
    // ordered, it will arrange them appropriately based on the order in
    // which the tags were added to the StageGroup.
    // If a pair cannot be constructed, the sentinal
    // PathPair(Trace::NONE, Trace::NONE) will be returned
    PathPair orderedPair(const DTime& start, const DTime& end) {
        assert( contains(start.mPath) );
        assert( contains(end.mPath) );

        if (mOrdered == UNORDERED) {
            return PathPair(start.mPath, end.mPath);
        }
        else if (mOrdered == ORDERED_ALLOW_NEGATIVE) {
            uint32 start_i = findTagOrder(start.mPath);
            uint32 end_i = findTagOrder(end.mPath);

            if (start_i <= end_i)
                return PathPair(start.mPath, end.mPath);
            else
                return PathPair(end.mPath, start.mPath);
        }
        else if (mOrdered == ORDERED_STRICT) {
            uint32 start_i = findTagOrder(start.mPath);
            uint32 end_i = findTagOrder(end.mPath);

            if (start_i <= end_i)
                return PathPair(start.mPath, end.mPath);
            else
                return PathPair(Trace::NONE, Trace::NONE);
        }
        else {
            assert(false);
        }
    }

    bool validPair(const DTime& start, const DTime& end) {
        // Loops to repeated tags aren't interesting without the
        // intermediate info
        if (start.mPath == end.mPath)
            return false;

        // Otherwise do normal sanity check
        PathPair ordered = orderedPair(start, end);
        return !(ordered.first == Trace::NONE || ordered.second == Trace::NONE);
    }

    // Computes the difference between start and end (end - start), taking
    // into account ordering constraints.  If unordered is allowed, then the
    // returned Duration will always be >= 0.  If ordering is required, then
    // the returned Duration may be negative.
    Duration difference(const DTime& start, const DTime& end) {
        // figure out the permissible ordering
        PathPair ordered = orderedPair(start, end);

        assert(validPair(start, end));

        // then just subtract in the matching direction
        if (ordered.first == start.mPath) {
            assert (ordered.second == end.mPath);
            return end - start;
        }
        else {
            assert (ordered.second == start.mPath);
            return start - end;
        }
    }

  private:
    uint32 findTagOrder(Trace::MessagePath tag) const {
        for(uint32 ii = 0; ii < mOrderedTags.size(); ii++) {
            if (mOrderedTags[ii] == tag)
                return ii;
        }
        assert(false);
        return 0;
    }

    String mName;
    IsOrdered mOrdered;
    typedef std::set<Trace::MessagePath> TagSet;
    typedef std::vector<Trace::MessagePath> OrderedTagSet;
    TagSet mTags;
    OrderedTagSet mOrderedTags;
};


class PacketData {
  public:
    uint64 id;
    uint16 source_port;
    uint16 dest_port;

    std::vector<DTime> mStamps;

    PacketData()
            : id(0),
              source_port(0),
              dest_port(0)
    {
    }
};

bool verify(const uint32*server, const PacketData &pd, Trace::MessagePath path) {
    if (server==NULL) return true;
    for (std::vector<DTime>::const_iterator i=pd.mStamps.begin(),ie=pd.mStamps.end();i!=ie;++i) {

        if (i->mPath==path)
            return i->mServerId==*server;
    }
    return false;

}

bool matches(const MessageLatencyFilters& filters, const PacketData& pd) {
    return (filters.mDestPort == NULL || pd.dest_port == *filters.mDestPort)&&
            verify(filters.mFilterByCreationServer,pd,Trace::CREATED)&&
            verify(filters.mFilterByDestructionServer,pd,Trace::DESTROYED);
}

} // namespace

MessageLatencyFilters::MessageLatencyFilters(uint16 *destPort, const uint32*filterByCreationServer,const uint32 *filterByDestructionServer, const uint32*filterByForwardingServer, const uint32 *filterByDeliveryServer) {
    mDestPort=destPort;
    mFilterByCreationServer=filterByCreationServer;
    mFilterByDestructionServer=filterByDestructionServer;
    mFilterByForwardingServer=filterByForwardingServer;
    mFilterByDeliveryServer=filterByDeliveryServer;
}


#define PACKETSTAGE(x) case Trace::x: return #x
const char* getPacketStageName (uint32 path) {
    switch (path) {
        // Object Host Checkpoints
        PACKETSTAGE(CREATED);
        PACKETSTAGE(DESTROYED);
        PACKETSTAGE(OH_ENQUEUED);
        PACKETSTAGE(OH_DEQUEUED);
        PACKETSTAGE(OH_HIT_NETWORK);
        PACKETSTAGE(OH_DROPPED_AT_OH_ENQUEUED);
        PACKETSTAGE(OH_NET_RECEIVED);
        PACKETSTAGE(OH_DROPPED_AT_RECEIVE_QUEUE);
        PACKETSTAGE(OH_RECEIVED);

        // Space Checkpoints
        PACKETSTAGE(HANDLE_OBJECT_HOST_MESSAGE);
        PACKETSTAGE(HANDLE_SPACE_MESSAGE);
        PACKETSTAGE(FORWARDED_LOCALLY);
        PACKETSTAGE(FORWARDING_STARTED);
        PACKETSTAGE(FORWARDED_LOCALLY_SLOW_PATH);
        PACKETSTAGE(OSEG_LOOKUP_STARTED);
        PACKETSTAGE(OSEG_CACHE_LOOKUP_FINISHED);
        PACKETSTAGE(OSEG_SERVER_LOOKUP_FINISHED);
        PACKETSTAGE(OSEG_LOOKUP_FINISHED);
        PACKETSTAGE(SPACE_TO_SPACE_ENQUEUED);
        PACKETSTAGE(DROPPED);
        PACKETSTAGE(SPACE_TO_OH_ENQUEUED);

      default:
        return "Unknown Stage, add to Analysis.cpp:getPacketStageName";
    }

}

std::ostream& operator<<(std::ostream& os, const PathPair& rhs) {
    const char* first_stage = getPacketStageName(rhs.first);
    const char* second_stage = getPacketStageName(rhs.second);

    os << first_stage << " - " << second_stage;

    return os;
}

/** Represents a graph of packet stages, forming a FSM which packets follow as
 *  they flow through the system.  This graph is a little more complicated than
 *  a regular FSM because as we cross network links we have the possibility that
 *  timestamps get out of sync.
 *
 *  In order to handle this we actually maintain the graph using 2 types of
 *  directed edges: normal edges are used when the packet must stay on the same
 *  server and have non-decreasing timestamps to be valid and network link edges
 *  are used when the packet may transfer between servers.  Network link edges
 *  allow transitions that violate timing to handle synchronization errors.
 *  When a packet's timestamp sequence is being matched to the graph, only
 *  normal links are considered for each sequence of timestamps on a server, and
 *  network links are considered to jump between servers.
 *
 *  Note that to simplify the code, either all or none of the inbound or
 *  outbound edges of a state must be network link edges, i.e. you can only
 *  arrive or leave a state via the network or a normal link exclusively.
 *
 *  In order to determine where to start analysis and whether a packet completed
 *  its trip successfully, valid start and end states are maintained as well.
 *  These must not have input and output edges, respectively.
 */
class PacketStageGraph {
  public:

  private:
}; // class PacketStageGraph

void MessageLatencyAnalysis(const char* opt_name, const uint32 nservers, MessageLatencyFilters filter, const String& stage_dump_filename)
{
    StageGroup oh_create_group("Object Host Creation");
    oh_create_group.add(Trace::CREATED)
            .add(Trace::OH_ENQUEUED)
            .add(Trace::OH_DROPPED_AT_OH_ENQUEUED)
            ;

    StageGroup oh_net_exchange_group("Object Host Networking Exchange");
    oh_net_exchange_group.add(Trace::OH_ENQUEUED)
            .add(Trace::OH_DEQUEUED)
            ;

    StageGroup oh_send_group("Object Host Send");
    oh_send_group.add(Trace::OH_DEQUEUED)
            .add(Trace::OH_HIT_NETWORK)
            ;

    StageGroup oh_to_space_group("Object Host -> Space", StageGroup::ORDERED_ALLOW_NEGATIVE);
    oh_to_space_group
            .add(Trace::OH_HIT_NETWORK)
            .add(Trace::HANDLE_OBJECT_HOST_MESSAGE)
            ;

    StageGroup space_from_oh_route_decision_group("OH Message Routing Decision", StageGroup::ORDERED_STRICT);
    space_from_oh_route_decision_group
            .add(Trace::HANDLE_OBJECT_HOST_MESSAGE)
            .add(Trace::FORWARDED_LOCALLY)
            .add(Trace::FORWARDING_STARTED)
            ;

    StageGroup space_from_space_route_decision_group("Space Message Routing Decision", StageGroup::ORDERED_STRICT);
    space_from_space_route_decision_group
            .add(Trace::HANDLE_SPACE_MESSAGE)
            .add(Trace::FORWARDED_LOCALLY)
            .add(Trace::FORWARDING_STARTED)
            ;

    StageGroup space_direct_route_group("Space Direct Routing");
    space_direct_route_group
            .add(Trace::FORWARDED_LOCALLY)
            .add(Trace::DROPPED)
            .add(Trace::SPACE_TO_OH_ENQUEUED)
            ;

    StageGroup space_slow_direct_route_group("Space Slow Direct Routing");
    space_slow_direct_route_group
            .add(Trace::FORWARDING_STARTED)
            .add(Trace::DROPPED)
            .add(Trace::FORWARDED_LOCALLY_SLOW_PATH)
            ;

    StageGroup space_slow_direct_delivery_route_group("Space Slow Direct Delivery Routing");
    space_slow_direct_delivery_route_group
            .add(Trace::FORWARDED_LOCALLY_SLOW_PATH)
            .add(Trace::SPACE_TO_OH_ENQUEUED)
            ;

    StageGroup oseg_lookup_group("OSeg Lookups");
    oseg_lookup_group
            .add(Trace::FORWARDING_STARTED)
            .add(Trace::OSEG_LOOKUP_STARTED)
            .add(Trace::OSEG_CACHE_LOOKUP_FINISHED)
            .add(Trace::OSEG_SERVER_LOOKUP_FINISHED)
            .add(Trace::OSEG_LOOKUP_FINISHED)
            ;

    StageGroup space_forwarder_queue_group("Space Forwarding Queues");
    space_forwarder_queue_group
            .add(Trace::OSEG_LOOKUP_FINISHED)
            .add(Trace::DROPPED)
            .add(Trace::SPACE_TO_SPACE_ENQUEUED)
            ;

    StageGroup space_to_space_group("Space -> Space", StageGroup::ORDERED_ALLOW_NEGATIVE);
    space_to_space_group
            .add(Trace::SPACE_TO_SPACE_ENQUEUED)
            .add(Trace::HANDLE_SPACE_MESSAGE)
            ;

    StageGroup space_to_oh_group("Space -> Object Host", StageGroup::ORDERED_ALLOW_NEGATIVE);
    space_to_oh_group
            .add(Trace::SPACE_TO_OH_ENQUEUED)
            .add(Trace::OH_NET_RECEIVED)
            ;

    StageGroup oh_receive_group("Object Host Receive");
    oh_receive_group.add(Trace::DESTROYED)
            .add(Trace::OH_DROPPED_AT_RECEIVE_QUEUE)
            .add(Trace::OH_NET_RECEIVED)
            .add(Trace::OH_RECEIVED)
            ;


    typedef std::vector<StageGroup> StageGroupList;
    StageGroupList groups;
    groups.push_back(oh_create_group);
    groups.push_back(oh_net_exchange_group);
    groups.push_back(oh_send_group);
    groups.push_back(oh_to_space_group);
    groups.push_back(space_from_oh_route_decision_group);
    groups.push_back(space_from_space_route_decision_group);
    groups.push_back(space_direct_route_group);
    groups.push_back(space_slow_direct_route_group);
    groups.push_back(space_slow_direct_delivery_route_group);
    groups.push_back(oseg_lookup_group);
    groups.push_back(space_forwarder_queue_group);
    groups.push_back(space_to_space_group);
    groups.push_back(space_to_oh_group);
    groups.push_back(oh_receive_group);

    // read in all our data
    typedef std::tr1::unordered_map<uint64,PacketData> PacketMap;
    PacketMap packetFlow;

    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            Event* evt = Event::read(is, server_id);
            if (evt == NULL)
                break;

            {
                MessageTimestampEvent* tevt = dynamic_cast<MessageTimestampEvent*>(evt);
                if (tevt != NULL) {
                    PacketData* pd = &packetFlow[tevt->uid];
                    pd->mStamps.push_back(DTime(tevt->begin_time(),tevt->path));
                    MessageCreationTimestampEvent* cevt = dynamic_cast<MessageCreationTimestampEvent*>(evt);
                    if (cevt != NULL) {
                        if (cevt->srcport!=0) pd->source_port = cevt->srcport;
                        if (cevt->dstport!=0) pd->dest_port = cevt->dstport;
                    }
                }
            }
            delete evt;
        }
    }

    std::ofstream* stage_dump_file = NULL;
    if (!stage_dump_filename.empty()) {
        stage_dump_file = new std::ofstream(stage_dump_filename.c_str());
    }

    // Compute time diffs for each group x packet
    typedef std::map<PathPair,Average> PathAverageMap;
    PathAverageMap results;
    for (PacketMap::iterator iter=packetFlow.begin(),ie=packetFlow.end();
         iter!=ie;
         ++iter) {
        PacketData& pd = iter->second;
        if ( !matches(filter, pd) || (pd.mStamps.size() == 0) ) continue;
        std::stable_sort(pd.mStamps.begin(),pd.mStamps.end());
        for(StageGroupList::iterator group_it = groups.begin(); group_it != groups.end(); group_it++) {
            StageGroup& group = *group_it;
            // Given our packet list and a set of valid stages, we need to step
            // through finding "adjacent" pairs of stages, where adjacent means
            // that tag A and tag B are both in the StageGroup and there is no
            // tag C that is also in the StageGroup and exists between A and B.
            std::vector<DTime> filtered_stamps = group.filter(pd.mStamps);
            for (uint32 idx = 1; idx < filtered_stamps.size(); ++idx) {
                if (!group.validPair( filtered_stamps[idx-1], filtered_stamps[idx] ))
                    continue;
                PathPair pp = group.orderedPair( filtered_stamps[idx-1], filtered_stamps[idx] );
                Duration diff = group.difference( filtered_stamps[idx-1], filtered_stamps[idx] );
                results[pp].sample(diff);

                if (stage_dump_file) {
                    (*stage_dump_file) << pp << " " << diff << std::endl;
                }
            }
        }
        if (stage_dump_file) {
            (*stage_dump_file) << std::endl;
        }

    }

    if (stage_dump_file) {
        stage_dump_file->close();
        delete stage_dump_file;
        stage_dump_file = NULL;
    }

    // Report results for each (sensible) pair of stages
    // Reports are done by groups
    for (StageGroupList::iterator group_it = groups.begin(); group_it != groups.end(); group_it++) {
        StageGroup& group = *group_it;
        SILOG(analysis,info,"Group: " << group.name());
        // This approach is inefficient but only bad if # of groups sky rockets
        for (PathAverageMap::iterator resiter=results.begin(),resiterend=results.end();
             resiter!=resiterend;
             ++resiter) {

            PathPair path_pair = resiter->first;
            if (!group.contains(path_pair))
                continue;

            Average& avg = resiter->second;
            if (avg.samples() == 0)
                continue;

            SILOG(analysis,info,
                  "Stage " << path_pair << ":"
                  << avg.average() << " stddev " << avg.stddev()
                  << " #" << avg.samples()
                  );
        }
    }
}

} // namespace CBR
