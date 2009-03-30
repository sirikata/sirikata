/*  cbr
 *  Analysis.hpp
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

#ifndef _CBR_ANALYSIS_HPP_
#define _CBR_ANALYSIS_HPP_

#include "Utility.hpp"
#include "Time.hpp"
#include "MotionVector.hpp"
#include "ServerNetwork.hpp"

namespace CBR {

struct Event;
struct ObjectEvent;
struct PacketEvent;
class ObjectFactory;

/** Error of observed vs. true object locations over simulation period. */
class LocationErrorAnalysis {
public:
    LocationErrorAnalysis(const char* opt_name, const uint32 nservers);
    ~LocationErrorAnalysis();

    // Return true if observer was watching seen at any point during the simulation
    bool observed(const UUID& observer, const UUID& seen) const;

    // Return the average error in the approximation of an object over its observed period, sampled at the given rate.
    double averageError(const UUID& observer, const UUID& seen, const Duration& sampling_rate, ObjectFactory* obj_factory) const;
    // Return the average error in object position approximation over all observers and observed objects, sampled at the given rate.
    double globalAverageError(const Duration& sampling_rate, ObjectFactory* obj_factory) const;
private:
    typedef std::vector<ObjectEvent*> EventList;
    typedef std::map<UUID, EventList*> ObjectEventListMap;

    EventList* getEventList(const UUID& observer) const;

    ObjectEventListMap mEventLists;
}; // class LocationErrorAnalysis

/** Does analysis of bandwidth, e.g. checking total bandwidth in and out of a server,
 *  checking relative bandwidths when under load, etc.
 */
class BandwidthAnalysis {
public:
    BandwidthAnalysis(const char* opt_name, const uint32 nservers);
    ~BandwidthAnalysis();

    void computeSendRate(const ServerID& sender, const ServerID& receiver) const;
    void computeReceiveRate(const ServerID& sender, const ServerID& receiver) const;

    void computeWindowedSendRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time) const;
    void computeWindowedReceiveRate(const ServerID& sender, const ServerID& receiver, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time) const;
private:
    typedef std::vector<PacketEvent*> EventList;
    typedef std::map<ServerID, EventList*> ServerEventListMap;

    template<typename PacketEventType>
    class PacketIterator {
    public:
        PacketIterator(const ServerID& sender, const ServerID& receiver, EventList::iterator begin, EventList::iterator end)
         : mSender(sender), mReceiver(receiver), mRangeCurrent(begin), mRangeEnd(end)
        {
            if (!currentMatches())
                next();
        }

        PacketEventType* current() {
            if (mRangeCurrent == mRangeEnd)
                return NULL;

            PacketEventType* event = dynamic_cast<PacketEventType*>(*mRangeCurrent);
            assert(event != NULL);
            return event;
        }

        PacketEventType* next() {
            if (mRangeCurrent == mRangeEnd) return NULL;
            do {
                mRangeCurrent++;
            } while(mRangeCurrent != mRangeEnd && !currentMatches());
            return current();
        }

    private:
        bool currentMatches() {
            PacketEventType* event = dynamic_cast<PacketEventType*>(*mRangeCurrent);
            if (event == NULL) return false;
            if (event->source != mSender || event->dest != mReceiver) return false;
            return true;
        }

        ServerID mSender;
        ServerID mReceiver;
        EventList::iterator mRangeCurrent;
        EventList::iterator mRangeEnd;
    };

    EventList::iterator begin(const ServerID& server) const {
        return getEventList(server)->begin();
    }
    EventList::iterator end(const ServerID& server) const {
        return getEventList(server)->end();
    }

    EventList* getEventList(const ServerID& server) const;


    template<typename PacketEventType>
    void computeRate(const ServerID& sender, const ServerID& receiver, const ServerID& filter) const {
        uint64 total_bytes = 0;
        uint32 last_bytes = 0;
        Duration last_duration;
        Time last_time(0);
        double max_bandwidth = 0;
        for(PacketIterator<PacketEventType> event_it(sender, receiver, begin(filter), end(filter)); event_it.current() != NULL; event_it.next() ) {
            PacketEventType* p_evt = event_it.current();

            total_bytes += p_evt->size;

            if (p_evt->time != last_time) {
                double bandwidth = (double)last_bytes / last_duration.seconds();
                if (bandwidth > max_bandwidth)
                    max_bandwidth = bandwidth;

                last_bytes = 0;
                last_duration = p_evt->time - last_time;
                last_time = p_evt->time;
            }

            last_bytes += p_evt->size;
        }

        printf("%d to %d: %ld total, %f max\n", sender, receiver, total_bytes, max_bandwidth);
    }

    template<typename PacketEventType>
    void computeWindowedRate(const ServerID& sender, const ServerID& receiver, const ServerID& filter, const Duration& window, const Duration& sample_rate, const Time& start_time, const Time& end_time) const {
        PacketIterator<PacketEventType> event_it(sender, receiver, begin(filter), end(filter));
        std::queue<PacketEventType*> window_events;

        uint64 bytes = 0;
        double max_bandwidth = 0;

        for(Time window_center = start_time; window_center < end_time; window_center += sample_rate) {
            Time window_end = window_center + window / 2.f;

            // add in any new packets that now fit in the window
            while(true) {
                PacketEventType* evt = event_it.current();
                if (evt == NULL || evt->time > window_end) break;
                bytes += evt->size;
                window_events.push(evt);
                event_it.next();
            }

            // subtract out any packets that have fallen out of the window
            // note we use event_time + window < window_end because subtracting could underflow the time
            while(!window_events.empty() && window_events.front()->time + window < window_end) {
                bytes -= window_events.front()->size;
                window_events.pop();
            }

            // finally compute the current bandwidth
            double bandwidth = bytes / window.seconds();
            if (bandwidth > max_bandwidth)
                max_bandwidth = bandwidth;
        }

        printf("%d to %d: %f max\n", sender, receiver, max_bandwidth);
    }


    ServerEventListMap mEventLists;
}; // class BandwidthAnalysis


} // namespace CBR

#endif //_CBR_ANALYSIS_HPP_
