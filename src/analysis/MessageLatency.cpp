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


#define PACKETSTAGE(x) case Trace::x: return #x
const char* getPacketStageName (uint32 path) {
    switch (path) {
        // Object Host Checkpoints
        PACKETSTAGE(CREATED);
        PACKETSTAGE(DESTROYED);
        PACKETSTAGE(OH_HIT_NETWORK);
        PACKETSTAGE(OH_DROPPED_AT_SEND);
        PACKETSTAGE(OH_NET_RECEIVED);
        PACKETSTAGE(OH_DROPPED_AT_RECEIVE_QUEUE);
        PACKETSTAGE(OH_RECEIVED);

        // Space Checkpoints
        PACKETSTAGE(HANDLE_OBJECT_HOST_MESSAGE);
        PACKETSTAGE(SPACE_DROPPED_AT_MAIN_STRAND_CROSSING);
        PACKETSTAGE(HANDLE_SPACE_MESSAGE);
        PACKETSTAGE(FORWARDED_LOCALLY);
        PACKETSTAGE(DROPPED_AT_FORWARDED_LOCALLY);
        PACKETSTAGE(FORWARDING_STARTED);
        PACKETSTAGE(FORWARDED_LOCALLY_SLOW_PATH);

        PACKETSTAGE(DROPPED_DURING_FORWARDING);

        PACKETSTAGE(OSEG_LOOKUP_STARTED);
        PACKETSTAGE(OSEG_CACHE_LOOKUP_FINISHED);
        PACKETSTAGE(OSEG_SERVER_LOOKUP_FINISHED);
        PACKETSTAGE(OSEG_LOOKUP_FINISHED);
        PACKETSTAGE(SPACE_TO_SPACE_ENQUEUED);
        PACKETSTAGE(SPACE_TO_SPACE_ACCEPTED);
        PACKETSTAGE(DROPPED_AT_SPACE_ENQUEUED);
        PACKETSTAGE(SPACE_TO_OH_ENQUEUED);

      default:
        return "Unknown Stage, add to Analysis.cpp:getPacketStageName";
    }
}

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

std::ostream& operator<<(std::ostream& os, const PathPair& rhs) {
    const char* first_stage = getPacketStageName(rhs.first);
    const char* second_stage = getPacketStageName(rhs.second);

    os << first_stage << " - " << second_stage;

    return os;
}

class PacketSample : public Time {
  public:
    uint32 server;
    Trace::MessagePath tag;


    bool isNull() const {
        const Time * t=this;
        return *t==Time::null();
    }

    PacketSample()
            : Time(Time::null()),
              server(-1),
              tag(Trace::NUM_PATHS)
    {
    }

    PacketSample(const Time&t, uint32 sid, Trace::MessagePath path)
            : Time(t),
              server(sid),
              tag(path)
    {
    }

    bool operator < (const Time&other)const {
        return *static_cast<const Time*>(this)<other;
    }
    bool operator == (const Time&other)const {
        return *static_cast<const Time*>(this)==other;
    }
};
typedef std::vector<PacketSample> PacketSampleList;

std::ostream& operator<<(std::ostream& os, const PacketSample& rhs) {
    const char* tag_name = getPacketStageName(rhs.tag);
    os << tag_name << "(" << (rhs-Time::null()) << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const PacketSampleList& rhs) {
    for(PacketSampleList::const_iterator it = rhs.begin(); it != rhs.end(); it++) {
        os << "[" << *it << "]";
    }
    return os;
}

class PacketData {
  public:
    uint64 id;
    uint16 source_port;
    uint16 dest_port;

    typedef std::map<uint32, PacketSampleList> ServerPacketMap;

    ServerPacketMap stamps;

    PacketData()
            : id(0),
              source_port(0),
              dest_port(0)
    {
    }
};

std::ostream& operator<<(std::ostream& os, const PacketData& rhs) {
    for(PacketData::ServerPacketMap::const_iterator it = rhs.stamps.begin(); it != rhs.stamps.end(); it++) {
        os << it->first << it->second << ", ";
    }
    return os;
}

bool verify(const uint32*server, const PacketData &pd, Trace::MessagePath path) {
    if (server==NULL)
        return true;

    PacketData::ServerPacketMap::const_iterator server_it = pd.stamps.find(*server);
    if (server_it == pd.stamps.end())
        return false;

    const PacketSampleList& samples = server_it->second;

    for(PacketSampleList::const_iterator sample_it = samples.begin(); sample_it != samples.end(); sample_it++) {
        if (sample_it->tag == path)
            return true;
    }

    return false;
}

bool matches(const MessageLatencyFilters& filters, const PacketData& pd) {
    return (filters.mDestPort == NULL || pd.dest_port == *filters.mDestPort)&&
            verify(filters.mFilterByCreationServer,pd,Trace::CREATED)&&
            verify(filters.mFilterByDestructionServer,pd,Trace::DESTROYED);
}

typedef std::tr1::function<void(PathPair, Duration)> ReportPairFunction;


class PermutationGenerator {
    /* Adapted from http://www.merriampark.com/perm.htm */
  private:
    std::vector<uint32> a;
    uint64 numLeft;
    uint64 total;

    static uint64 getFactorial (int n) {
        uint64 fact = 1;
        for (uint64 i = n; i > 1; i--)
            fact *= i;
        return fact;
    }

  public:

    PermutationGenerator (int n) {
        assert (n >= 1);
        assert (n <= 5);
        a.resize(n);
        total = getFactorial(n);
        reset();
    }

    void reset () {
        for (uint32 i = 0; i < a.size(); i++)
            a[i] = i;
        numLeft = total;
    }

    uint64 getNumLeft () {
        return numLeft;
    }

    uint64 getTotal () {
        return total;
    }

    bool hasMore () {
        return numLeft > 0;
    }

    const std::vector<uint32>& getNext() {
        if (numLeft == total) {
            numLeft -= 1;
            return a;
        }

        int temp;

        // Find largest index j with a[j] < a[j+1]

        int j = a.size() - 2;
        while (a[j] > a[j+1]) {
            j--;
        }

        // Find index k such that a[k] is smallest integer
        // greater than a[j] to the right of a[j]

        int k = a.size() - 1;
        while (a[j] > a[k]) {
            k--;
        }

        // Interchange a[j] and a[k]

        temp = a[k];
        a[k] = a[j];
        a[j] = temp;

        // Put tail end of permutation after jth position in increasing order

        int r = a.size() - 1;
        int s = j + 1;

        while (r > s) {
            temp = a[s];
            a[s] = a[r];
            a[r] = temp;
            r--;
            s++;
        }

        numLeft -= 1;
        return a;
    }
}; // PermutationGenerator

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

    enum EdgeType {
        SYNC,
        ASYNC
    };

    void addEdge(Trace::MessagePath from, Trace::MessagePath to, EdgeType type = SYNC) {
        Edge edge;

        edge.index = edges.size();
        edge.from = from;
        edge.to = to;
        edge.type = type;

        edges.push_back(edge);

        stages[from].out_edges.push_back(edge.index);
        verifyEdges(from);

        stages[to].in_edges.push_back(edge.index);
        verifyEdges(to);

        recomputeTerminals();
    }

    typedef std::tr1::function<void(PathPair)> EdgeTraversalCallback;

    void depth_first_edge_traversal(EdgeTraversalCallback cb) const {
        StageSet visited;

        typedef std::stack<Trace::MessagePath> StageStack;
        StageStack next;

        for(StageSet::iterator it = starting_stages.begin(); it != starting_stages.end(); it++)
            next.push(*it);

        while(!next.empty()) {
            Trace::MessagePath tag = next.top();
            next.pop();

            if (visited.find(tag) != visited.end())
                continue;

            visited.insert(tag);

            const Stage& st = stages.find(tag)->second;
            for(EdgeIndexList::const_iterator it = st.out_edges.begin(); it != st.out_edges.end(); it++) {
                const Edge& edge = edges[*it];

                PathPair pp(edge.from, edge.to);

                cb(pp);

                if (visited.find(edge.to) == visited.end())
                    next.push(edge.to);
            }
        }
    }

    void match_path(PacketData pd, ReportPairFunction cb) const {
        // First try to get everything broken down into sets of sample lists
        // which start and end with async operations, ordered by the time of the
        // first sample in each list
        OrderedPacketSampleListList ordered_list_list;
        bool broken = breakAtAsync(pd, ordered_list_list);

        if (!broken) {
            std::cout << "Couldn't find sane breakdown for: " << pd << std::endl;
            return;
        }

        bool matched = tryPermutations(
            ordered_list_list,
            cb
                                       );

        if (!matched) {
            std::cout << "Couldn't match packet path: " << pd
                      << std::endl;

            for(OrderedPacketSampleListList::iterator dump_it = ordered_list_list.begin(); dump_it != ordered_list_list.end(); dump_it++) {
                PacketSampleList dump_subseq = *dump_it;
                std::cout << dump_subseq << std::endl;
            }
            return;
        }
    }
  private:
    bool hasStage(Trace::MessagePath mp) const {
        if (stages.find(mp) == stages.end()) {
            std::cerr << "Don't have stage " << getPacketStageName(mp) << std::endl;
            return false;
        }
        return true;
    }

    // Verifies that the edges associated with a tag actually satisfy the
    // constraints -- that all inputs and all outputs are the same type
    void verifyEdges(Trace::MessagePath mp) const {
        assert(hasStage(mp));
        const Stage& stage = stages.find(mp)->second;

        EdgeType in_type = SYNC;
        for(EdgeIndexList::const_iterator it = stage.in_edges.begin(); it != stage.in_edges.end(); it++) {
            if (it == stage.in_edges.begin()) {
                in_type = edges[*it].type;
                continue;
            }

            assert(edges[*it].type == in_type);
        }

        EdgeType out_type = SYNC;
        for(EdgeIndexList::const_iterator it = stage.out_edges.begin(); it != stage.out_edges.end(); it++) {
            if (it == stage.out_edges.begin()) {
                out_type = edges[*it].type;
                continue;
            }

            assert(edges[*it].type == out_type);
        }
    }

    // The following check whether a stage is a pure entry or exit, meaning they
    // are packet sources or sinks.
    bool isPureEntry(Trace::MessagePath mp) const {
        assert(hasStage(mp));
        const Stage& stage = stages.find(mp)->second;

        return (stage.in_edges.size() == 0);
    }
    bool isPureExit(Trace::MessagePath mp) const {
        assert(hasStage(mp));
        const Stage& stage = stages.find(mp)->second;

        return (stage.out_edges.size() == 0);
    }

    // The following check whether a stage is an async entry or async exit
    // state, i.e. all exits or entries to that state are async.  Given the
    // check performed by verifyEdges, we only have to check one edge in the
    // correct direction to check this.
    bool isAsyncEntry(Trace::MessagePath mp) const {
        assert(hasStage(mp));
        const Stage& stage = stages.find(mp)->second;

        if (stage.in_edges.size() == 0)
            return true;

        return (edges[stage.in_edges[0]].type == ASYNC);
    }
    bool isAsyncExit(Trace::MessagePath mp) const {
        assert(hasStage(mp));
        const Stage& stage = stages.find(mp)->second;

        if (stage.out_edges.size() == 0)
            return true;

        return (edges[stage.out_edges[0]].type == ASYNC);
    }

    // Returns true if the edge exists in the graph
    bool validEdge(Trace::MessagePath from, Trace::MessagePath to) const {
        assert(hasStage(from) && hasStage(to));

        const Stage& from_stage = stages.find(from)->second;

        for(uint32 ii = 0; ii < from_stage.out_edges.size(); ii++) {
            assert(edges[from_stage.out_edges[ii]].from == from);
            if (edges[from_stage.out_edges[ii]].to == to)
                return true;
        }
        return false;
    }

    // Computes starting and ending states. We don't expect this graph to be
    // huge so we just do this brute force each time instead of maintaining the
    // info and performing updates.
    void recomputeTerminals() {
        starting_stages.clear();
        terminal_stages.clear();

        for(StageMap::iterator it = stages.begin(); it != stages.end(); it++) {
            Trace::MessagePath st_tag = it->first;
            Stage& st = it->second;

            if (st.in_edges.empty())
                starting_stages.insert(st_tag);

            if (st.out_edges.empty())
                terminal_stages.insert(st_tag);
        }
    }

    // Breaks up PacketData into an ordered set of PacketSampleLists based on
    // the UNSYNC edges.  This assumes that within sequences of tags on the same
    // server there are no breaks by other tags.  This *should* be safe since
    // the packet would have to make it back to the server over the network and
    // the clock would have to tag it with an earlier time, which, aside from
    // multicore systems and uber fast network links, shouldn't be possible.
    // Note that another possible source of this problem is cross-thread posting
    // where tags are not recorded until it is completed.  Generally this should
    // not be done unless absolutely necessary.  Instead, prefer using a tag
    // before the operation and one for each of success and failure to post.
    typedef std::vector<PacketSampleList> OrderedPacketSampleListList;
    // Records a sample list and the starting offset of unused samples in
    // that list
    struct UnusedSamples {
        uint32 offset;
        PacketSampleList list;
    };
    bool breakAtAsync(const PacketData& pd, OrderedPacketSampleListList& output) const {
        typedef std::multimap<Time, UnusedSamples> UnusedSampleMap;
        UnusedSampleMap sorted_unused_samples;

        OrderedPacketSampleListList unconstrained_output;

        for(PacketData::ServerPacketMap::const_iterator it = pd.stamps.begin(); it != pd.stamps.end(); it++) {
            UnusedSamples us;
            us.offset = 0;
            us.list = it->second;
            sorted_unused_samples.insert(UnusedSampleMap::value_type(it->second[0], us));
        }
        while(!sorted_unused_samples.empty()) {
            // Take the next earliest
            Time us_time = sorted_unused_samples.begin()->first;
            UnusedSamples us = sorted_unused_samples.begin()->second;
            sorted_unused_samples.erase( sorted_unused_samples.begin() );

            // First *must* be async entry
            PacketSampleList next_set;
            uint32 cur_offset = us.offset;
            if (!isAsyncEntry(us.list[cur_offset].tag)) {
                printf("First entry in list isn't async\n");
                return false;
            }

            // Then continue until we hit the end or async exit
            while(cur_offset < us.list.size()) {
                next_set.push_back( us.list[cur_offset] );
                uint32 was_offset = cur_offset;
                cur_offset++;
                if ( isAsyncExit(us.list[was_offset].tag) )
                    break;
            }

            unconstrained_output.push_back(next_set);

            if (cur_offset < us.list.size()) {
                // Reinsert
                us.offset = cur_offset;
                sorted_unused_samples.insert(UnusedSampleMap::value_type(us.list[cur_offset], us));
            }
        }

        // With things sorted, we just need to make sure that one with a
        // starting state and one with an ending state are first and last
        // respectively
        int32 starting_samples_idx = -1, ending_samples_idx = -1;
        for(uint32 ii = 0; ii < unconstrained_output.size(); ii++) {
            if ( isPureEntry(unconstrained_output[ii][0].tag) ) {
                if (starting_samples_idx != -1) {
                    printf("Found 2 entry points\n");
                    return false; // Found 2 entry points
                }
                else
                    starting_samples_idx = ii;
            }

            if ( isPureExit(unconstrained_output[ii][ unconstrained_output[ii].size()-1 ].tag) ) {
                if (ending_samples_idx != -1) {
                    printf("Found 2 exit points\n");
                    return false; // Found 2 exit points
                }
                else
                    ending_samples_idx = ii;
            }
        }

        if (starting_samples_idx == -1 || ending_samples_idx == -1) {
            printf("Didn't find both entry and exit point.\n");
            return false;
        }

        if (unconstrained_output.size() == 1) {
            if (starting_samples_idx == ending_samples_idx &&
                starting_samples_idx != -1 &&
                ending_samples_idx != -1) {
                output.push_back( unconstrained_output[0] );
                return true;
            }
            else {
                printf("One subsequence, but source and sink don't match.\n");
                return false;
            }
        }

        output.push_back( unconstrained_output[starting_samples_idx] );
        for(uint32 ii = 0; ii < unconstrained_output.size(); ii++) {
            if (ii != (uint32)starting_samples_idx && ii != (uint32)ending_samples_idx)
                output.push_back( unconstrained_output[ii] );
        }
        output.push_back( unconstrained_output[ending_samples_idx] );

        return true;
    }


    bool testPermutation(OrderedPacketSampleListList ordered_list_list, ReportPairFunction cb) const {
        // Now just run through them in order, sanity checking the order and
        // connections, and generating samples.  Note that we do 2 passes, where
        // the second one records data if the first pass was successful.
        // We could try to match subsequences more intelligently, but currently
        // we're just hoping we're actually slow enough + synchronized enough
        // for it not to matter.
        for(uint8 report = 0; report < 2; report++) {
            PacketSample prev_sample;

            for(OrderedPacketSampleListList::iterator ordered_it = ordered_list_list.begin(); ordered_it != ordered_list_list.end(); ordered_it++) {
                PacketSampleList subseq = *ordered_it;
                for(PacketSampleList::iterator packet_it = subseq.begin(); packet_it != subseq.end(); packet_it++) {
                    PathPair pp(prev_sample.tag, packet_it->tag);

                    if (!prev_sample.isNull() &&
                        !validEdge(prev_sample.tag, packet_it->tag) ) {

                        return false;
                    }

                    if (report) {
                        Duration diff = (*packet_it - prev_sample);
                        cb(pp, diff);
                    }
                    prev_sample = *packet_it;
                }
            }
        }

        return true;
    }

    // Generates permutations of the subsequences until it finds one that
    // generates a valid path
    // FIXME for long paths we should probably do additional sanity checks on
    // timestamps between stages on each permutation
    bool tryPermutations(OrderedPacketSampleListList samples, ReportPairFunction cb) const {
        if (samples.empty())
            return true;

        if (samples.size() <= 2) {
            bool valid = testPermutation(samples, cb);
            return valid;
        }

        // The start and end subsequences will always be the same
        PacketSampleList samples_front = samples.front();
        PacketSampleList samples_back = samples.back();

        samples.erase(samples.begin());
        samples.pop_back();

        // Now generate all permutations of interior sample subsequences
        PermutationGenerator perms(samples.size());
        while(perms.hasMore()) {
            // Construct the actual permutation
            std::vector<uint32> perm = perms.getNext();
            OrderedPacketSampleListList perm_list;
            perm_list.push_back(samples_front);
            for(uint32 ii = 0; ii < perm.size(); ii++)
                perm_list.push_back( samples[perm[ii]] );
            perm_list.push_back(samples_back);

            // And test it
            bool valid = testPermutation(perm_list, cb);
            if (valid)
                return true;
        }

        return false;
    }

    typedef uint32 EdgeIndex; // Index of an edge in our list
    typedef std::vector<EdgeIndex> EdgeIndexList;

    struct Stage {
        EdgeIndexList in_edges;
        EdgeIndexList out_edges;
    };

    typedef std::set<Trace::MessagePath> StageSet;
    typedef std::map<Trace::MessagePath, Stage> StageMap;

    struct Edge {
        uint32 index;
        Trace::MessagePath from;
        Trace::MessagePath to;
        EdgeType type;
    };
    typedef std::vector<Edge> EdgeList;

    StageMap stages;
    EdgeList edges;

    StageSet starting_stages;
    StageSet terminal_stages;
}; // class PacketStageGraph






typedef std::map<PathPair,Average> PathAverageMap;

// When a valid mapping is found, this will take care of outputting a single
// pair of stages.  Used as callback in method that maps packet timestamp
// sequence to stage pairs
void reportPair(PathPair pp, Duration tdiff, PathAverageMap* pam, std::ostream* stage_dump_file) {
    if (pam && pp.first != Trace::NUM_PATHS) {
        (*pam)[pp].sample(tdiff);
    }

    if (stage_dump_file) {
        if (pp.first == Trace::NUM_PATHS)
            (*stage_dump_file) << std::endl;
        else
            (*stage_dump_file) << pp << " " << tdiff << std::endl;
    }
}

// Reports aggregate statistics for a stage pair to SILOG. Callback passed to
// graph traversal method to get "ordered" output.
void reportStats(PathPair pp, PathAverageMap* pam) {
    if (pam == NULL)
        return;

    Average& avg = (*pam)[pp];
    if (avg.samples() == 0)
        return;

    SILOG(analysis,info,
          "Stage " << pp << ":"
          << avg.average() << " stddev " << avg.stddev()
          << " #" << avg.samples()
          );
}

} // namespace

MessageLatencyFilters::MessageLatencyFilters(uint16 *destPort, const uint32*filterByCreationServer,const uint32 *filterByDestructionServer, const uint32*filterByForwardingServer, const uint32 *filterByDeliveryServer) {
    mDestPort=destPort;
    mFilterByCreationServer=filterByCreationServer;
    mFilterByDestructionServer=filterByDestructionServer;
    mFilterByForwardingServer=filterByForwardingServer;
    mFilterByDeliveryServer=filterByDeliveryServer;
}



void MessageLatencyAnalysis(const char* opt_name, const uint32 nservers, MessageLatencyFilters filter, const String& stage_dump_filename)
{
    // Setup the graph of valid stage transitions
    PacketStageGraph stage_graph;

    stage_graph.addEdge(Trace::CREATED, Trace::OH_HIT_NETWORK);
    stage_graph.addEdge(Trace::CREATED, Trace::OH_DROPPED_AT_SEND); // drop

    stage_graph.addEdge(Trace::OH_HIT_NETWORK, Trace::HANDLE_OBJECT_HOST_MESSAGE, PacketStageGraph::ASYNC);
    stage_graph.addEdge(Trace::HANDLE_OBJECT_HOST_MESSAGE, Trace::FORWARDED_LOCALLY);
    stage_graph.addEdge(Trace::HANDLE_OBJECT_HOST_MESSAGE, Trace::FORWARDING_STARTED);
    stage_graph.addEdge(Trace::HANDLE_OBJECT_HOST_MESSAGE, Trace::SPACE_DROPPED_AT_MAIN_STRAND_CROSSING); // drop

    stage_graph.addEdge(Trace::HANDLE_SPACE_MESSAGE, Trace::FORWARDING_STARTED);

    stage_graph.addEdge(Trace::FORWARDED_LOCALLY, Trace::DROPPED_AT_FORWARDED_LOCALLY); // drop
    stage_graph.addEdge(Trace::FORWARDED_LOCALLY, Trace::SPACE_TO_OH_ENQUEUED);

    stage_graph.addEdge(Trace::FORWARDING_STARTED, Trace::FORWARDED_LOCALLY_SLOW_PATH);
    stage_graph.addEdge(Trace::FORWARDING_STARTED, Trace::OSEG_LOOKUP_STARTED);

    stage_graph.addEdge(Trace::FORWARDED_LOCALLY_SLOW_PATH, Trace::SPACE_TO_OH_ENQUEUED);
    stage_graph.addEdge(Trace::FORWARDED_LOCALLY_SLOW_PATH, Trace::DROPPED_DURING_FORWARDING); // drop

    stage_graph.addEdge(Trace::OSEG_LOOKUP_STARTED, Trace::DROPPED_DURING_FORWARDING); // drop
    stage_graph.addEdge(Trace::OSEG_LOOKUP_STARTED, Trace::OSEG_CACHE_LOOKUP_FINISHED);
    stage_graph.addEdge(Trace::OSEG_LOOKUP_STARTED, Trace::OSEG_SERVER_LOOKUP_FINISHED);
    stage_graph.addEdge(Trace::OSEG_CACHE_LOOKUP_FINISHED, Trace::OSEG_LOOKUP_FINISHED);
    stage_graph.addEdge(Trace::OSEG_SERVER_LOOKUP_FINISHED, Trace::OSEG_LOOKUP_FINISHED);

    stage_graph.addEdge(Trace::OSEG_LOOKUP_FINISHED, Trace::SPACE_TO_SPACE_ENQUEUED);
    stage_graph.addEdge(Trace::SPACE_TO_SPACE_ENQUEUED, Trace::DROPPED_AT_SPACE_ENQUEUED); // drop
    stage_graph.addEdge(Trace::SPACE_TO_SPACE_ENQUEUED, Trace::SPACE_TO_SPACE_ACCEPTED);
    stage_graph.addEdge(Trace::SPACE_TO_SPACE_ACCEPTED, Trace::HANDLE_SPACE_MESSAGE, PacketStageGraph::ASYNC);

    stage_graph.addEdge(Trace::SPACE_TO_OH_ENQUEUED, Trace::OH_NET_RECEIVED, PacketStageGraph::ASYNC);
    stage_graph.addEdge(Trace::OH_NET_RECEIVED, Trace::OH_RECEIVED);
    stage_graph.addEdge(Trace::OH_NET_RECEIVED, Trace::OH_DROPPED_AT_RECEIVE_QUEUE); // drop
    stage_graph.addEdge(Trace::OH_RECEIVED, Trace::DESTROYED);
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
                    pd->stamps[server_id].push_back(PacketSample(tevt->begin_time(), server_id, tevt->path));
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

    // Prepare output data structures
    std::ofstream* stage_dump_file = NULL;
    if (!stage_dump_filename.empty()) {
        stage_dump_file = new std::ofstream(stage_dump_filename.c_str());
    }
    PathAverageMap results;

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    ReportPairFunction report_func = std::tr1::bind(&reportPair, _1, _2, &results, stage_dump_file);


    // Perform a stable sort for each packet's server timestamp lists, then try
    // to match it to the graph.
    // Note that the stable sort is only necessary because the logging is
    // multithreaded and may not get everything perfectly in order.
    for (PacketMap::iterator iter = packetFlow.begin(),ie=packetFlow.end();
         iter!=ie;
         ++iter) {
        PacketData& pd = iter->second;
        if ( !matches(filter, pd) || (pd.stamps.size() == 0) ) continue;

        for(PacketData::ServerPacketMap::iterator server_it = pd.stamps.begin();
            server_it != pd.stamps.end();
            server_it++) {
            std::stable_sort(server_it->second.begin(), server_it->second.end());
        }

        stage_graph.match_path(pd, report_func);
    }

    if (stage_dump_file) {
        stage_dump_file->close();
        delete stage_dump_file;
        stage_dump_file = NULL;
    }

    // Report results for all stages which we've found pairs for
    stage_graph.depth_first_edge_traversal(
        std::tr1::bind(&reportStats, _1, &results)
                                           );
}

} // namespace CBR
