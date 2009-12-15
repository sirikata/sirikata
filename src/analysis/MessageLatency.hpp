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

#ifndef _ANALYSIS_MESSAGE_LATENCY_HPP_
#define _ANALYSIS_MESSAGE_LATENCY_HPP_

#include "Statistics.hpp"

namespace CBR {

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


class MessageLatencyAnalysis {
  public:
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
        uint64 mId;
        unsigned char mSrcPort;
        unsigned short mDstPort;

        std::vector<DTime> mStamps;

        PacketData();
    };
    class Filters {
    public:
        const uint32*mFilterByCreationServer;
        const uint32*mFilterByDestructionServer;
        const uint32*mFilterByForwardingServer;
        const uint32 *mFilterByDeliveryServer;
        const uint16 *mDestPort;
        Filters(uint16 *destPort=NULL, const uint32*filterByCreationServer=NULL,const uint32 *filterByDestructionServer=NULL, const uint32*filterByForwardingServer=NULL, const uint32 *filterByDeliveryServer=NULL) {
            mDestPort=destPort;
            mFilterByCreationServer=filterByCreationServer;
            mFilterByDestructionServer=filterByDestructionServer;
            mFilterByForwardingServer=filterByForwardingServer;
            mFilterByDeliveryServer=filterByDeliveryServer;
        }
        bool verify(const uint32*server,const PacketData &pd, Trace::MessagePath path) const{
            if (server==NULL) return true;
            for (std::vector<DTime>::const_iterator i=pd.mStamps.begin(),ie=pd.mStamps.end();i!=ie;++i) {

                if (i->mPath==path)
                    return i->mServerId==*server;
            }
            return false;

        }
        bool operator() (const PacketData&pd)const{
            return (mDestPort==NULL||pd.mDstPort==*mDestPort)&&
                verify(mFilterByCreationServer,pd,Trace::CREATED)&&
                    verify(mFilterByDestructionServer,pd,Trace::DESTROYED);
        }
    };
    MessageLatencyAnalysis(const char* opt_name, const uint32 nservers, Filters f, const String& stage_dump_file = "");
    ~MessageLatencyAnalysis();
    Filters mFilter;
    uint32_t mNumberOfServers;
};

} // namespace CBR


#endif //_ANALYSIS_MESSAGE_LATENCY_HPP_
