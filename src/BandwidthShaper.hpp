/*  cbr
 *  BandwidthShaper.hpp
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

#ifndef _CBR_BANDWIDTH_SHAPER_
#define _CBR_BANDWIDTH_SHAPER_

#include "Utility.hpp"
#include "RouterElement.hpp"

namespace CBR {

/** A bandwidth shaper router element which limits packet flow to a specified
 *  maximum bandwidth.
 */
template<typename PacketType>
class BandwidthShaper : public UpstreamElementFixed<PacketType, 1>, public DownstreamElementFixed<PacketType, 1> {
public:
    BandwidthShaper(uint32 rate)
     : mRate(rate),
       mRemainder(0),
       mLastEvalTime(Timer::now()),
       mLastEndTime(mLastEvalTime)
    {
    }

    virtual bool push(uint32 port, PacketType* pkt) {
        assert(false);
        return false;
    }

    PacketType* pull() {
        return pull(0);
    }
    virtual PacketType* pull(uint32 port) {
        assert(port == 0);

        // Update current budget
        Time curtime = Timer::now();
        Duration since_last = curtime - mLastEvalTime;
        mLastEvalTime = curtime;
        mRemainder = since_last.toSeconds() * mRate + mRemainder;

        // If we're still running a deficit, bail out
        if (mRemainder <= 0)
            return NULL;

        // Otherwise, try to pull
        PacketType* pkt = this->input(0).pull();

        // If upstream has nothing for us, bail out, dropping our budget
        if (pkt == NULL) {
            mRemainder = 0;
            mLastEndTime = curtime;
            return NULL;
        }

        // Otherwise, we're golden and we just need to do accounting
        uint32 pkt_size = pkt->size();
        Duration duration = Duration::seconds((float)pkt_size / (float)mRate);
        Time start_time = mLastEndTime;
        Time end_time = mLastEndTime + duration;
        mRemainder -= pkt_size;
        mLastEndTime = end_time;
        return pkt;
    }

private:
    uint32 mRate; // rate in bytes / second
    int32 mRemainder; // remainder bytes left from last round
    Time mLastEvalTime; // last time we evaluated the real time
    Time mLastEndTime; // packet send end time
}; // class BandwidthShaper

} // namespace CBR

#endif //_CBR_BANDWIDTH_SHAPER_
