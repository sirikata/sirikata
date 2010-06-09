/*  Sirikata
 *  TracePacketElement.hpp
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

#ifndef _SIRIKATA_TRACE_PACKET_ELEMENT_HPP_
#define _SIRIKATA_TRACE_PACKET_ELEMENT_HPP_

#include "Utility.hpp"
#include "RouterElement.hpp"
#include "Statistics.hpp"

namespace Sirikata {

/** Records packet information as it passes through the element. The user specifies
 *  what tag to record it with on success and failure (for pull elements, only success
 *  makes sense).
 */
template<typename PacketType>
class TracePacketElement : public UpstreamElementFixed<PacketType, 1>, public DownstreamElementFixed<PacketType, 1> {
public:
    TracePacketElement(Context* ctx, Trace::MessagePath succ, Trace::MessagePath failure = Trace::NONE)
     : mContext(ctx),
       mSuccessTag(succ),
       mFailureTag(failure)
    {
    }

    virtual bool push(uint32 port, PacketType* pkt) {
        TIMESTAMP_START(tstamp, pkt);

        bool pushed = this->output(0).push(pkt);

        TIMESTAMP_END(tstamp, mSuccessTag);

        if (!pushed)
            TIMESTAMP_END(tstamp, mFailureTag);

        return pushed;
    }

    virtual PacketType* pull(uint32 port) {
        PacketType* pkt = this->input(0).pull();

        if (pkt != NULL) {
            TIMESTAMP(pkt, mSuccessTag);
        }

        return pkt;
    }

private:
    Context* mContext;
    Trace::MessagePath mSuccessTag;
    Trace::MessagePath mFailureTag;
}; // class TracePacketElement

} // namespace Sirikata

#endif //_SIRIKATA_TRACE_PACKET_ELEMENT_HPP_
