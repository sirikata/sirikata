/*  cbr
 *  QueueRouterElement.hpp
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

#ifndef _CBR_QUEUE_ROUTER_ELEMENT_
#define _CBR_QUEUE_ROUTER_ELEMENT_

#include "Utility.hpp"
#include "RouterElement.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>

namespace CBR {

/** A queue router element which buffers packets. The size of the queue and
 *  a method for determining the size of a packet is provided.  If the maximum
 *  size would be violated, drops the last item in the queue.  Only has a single
 *  input and output.
 */
template<typename PacketType>
class QueueRouterElement : public UpstreamElementFixed<PacketType, 1>, public DownstreamElementFixed<PacketType, 1> {
    typedef boost::mutex mutex;
    typedef boost::unique_lock<mutex> unique_lock;
public:
    typedef std::tr1::function<uint32(PacketType*)> SizeFunctor;

    QueueRouterElement(uint32 max_size, SizeFunctor sizefunc)
     : mSizeFunctor(sizefunc),
       mMaxSize(max_size),
       mSize(0)
    {
    }

    ~QueueRouterElement() {
        PacketType* pkt = NULL;
        while( (pkt = pull()) != NULL)
            delete pkt;
    }

    bool push(PacketType* pkt) {
        return push(0, pkt);
    }
    virtual bool push(uint32 port, PacketType* pkt) {
        assert(pkt != NULL);
        assert(port == 0);

        uint32 sz = mSizeFunctor(pkt);

        uint32 cur_size = mSize.read();
        uint32 new_size = cur_size + sz;

        if (new_size > mMaxSize) {
            // drop
            delete pkt;
            return false;
        }

        {
            unique_lock guard(mMutex);
            // else store, must recompute since mSize may have changed
            mSize += sz;
            mPackets.push(pkt);
        }

        return true;
    }

    PacketType* pull() {
        return pull(0);
    }
    virtual PacketType* pull(uint32 port) {
        assert(port == 0);

        uint32 cur_size = mSize.read();

        if (cur_size == 0)
            return NULL;

        PacketType* pkt = NULL;
        {
            unique_lock guard(mMutex);

            if (mPackets.empty())
                return NULL;

            pkt = mPackets.front();
            mPackets.pop();
            uint32 sz = mSizeFunctor(pkt);
            mSize -= sz;
        }

        return pkt;
    }

private:
    typedef std::queue<PacketType*> PacketQueue;
    boost::mutex mMutex;
    PacketQueue mPackets;
    const SizeFunctor mSizeFunctor;
    uint32 mMaxSize;
    Sirikata::AtomicValue<uint32> mSize;
}; // class QueueRouterElement

} // namespace CBR

#endif //_CBR_QUEUE_ROUTER_ELEMENT_
