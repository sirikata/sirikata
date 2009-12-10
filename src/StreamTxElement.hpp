/*  cbr
 *  StreamTxElement.hpp
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

#ifndef _CBR_STREAM_TX_ELEMENT_HPP_
#define _CBR_STREAM_TX_ELEMENT_HPP_

#include "Utility.hpp"
#include "RouterElement.hpp"
#include <sirikata/network/Stream.hpp>
#include <sirikata/network/IOStrandImpl.hpp>
#include "Statistics.hpp"

namespace CBR {

/** A Sirikata Network Stream transmit element.  This element pulls
 *  packets from upstream and ships them over a user provided network
 *  stream.  Note that this contains a small, 1 packet buffer in order
 *  to avoid dropping packets it actively requests (i.e. the last packet
 *  requested is not going to fit, otherwise we'd request another
 *  packet.
 */
template<typename PacketType>
class StreamTxElement : public DownstreamElementFixed<PacketType, 1> {
public:
    // Note: This isn't a great solution, but until we can set *only* the ReadySendCallback for the stream,
    // we need to provide the code that gives us the stream and creates us a callback to register on our
    // behalf
    typedef std::tr1::function<void()> ReadySendCallback;
private:
    class Impl;
    typedef std::tr1::shared_ptr<Impl> ImplPtr;

    class Impl : public std::tr1::enable_shared_from_this<Impl> {
    public:
        /** Create a stream transmit element which outputs to the specified stream,
         *  works in the specified strand, and is polled at the specified maximum rate.
         *  \param strm the stream to transmit over
         *  \param strand the strand to run requests in
         *  \param max_rate the maximum rate (minimum duration between polls) for
         *         polling for new packets to send
         */
        Impl(StreamTxElement* parent, Context* ctx, Sirikata::Network::Stream* strm, IOStrand* strand, const Duration& max_rate, Trace::MessagePath tag)
         : mParent(parent),
           mContext(ctx),
           mStream(strm),
           mStrand(strand),
           mMaxRate(max_rate),
           mTag(tag),
           mWaitingPacket(NULL),
           mSerialized(NULL),
           mShutdown( false ),
           mTimer( IOTimer::create(strand->service()) ),
           mIterationProfiler(NULL),
           mWorkProfiler(NULL)
        {
            mIterationProfiler = mContext->profiler->addStage("StreamTx Iteration");
            mWorkProfiler = mContext->profiler->addStage("StreamTx Work");
        }

        ~Impl() {
            delete mIterationProfiler;
            delete mWorkProfiler;

            if (mWaitingPacket != NULL)
                delete mWaitingPacket;
            if (mSerialized != NULL)
                delete mSerialized;
        }

        void start() {
            mTimer->setCallback( readySendCallback() );
            mIterationProfiler->started();
            startPoll(this->shared_from_this());
        }

        void shutdown() {
            mShutdown = true;
        }

        ReadySendCallback readySendCallback() {
            return mStrand->wrap( std::tr1::bind(&Impl::doIter, this, this->shared_from_this()) );
        }

    private:
        bool trySend(PacketType* next_pkt) {
            std::string* data = next_pkt->serialize();

            // Try to push to the network
            bool success = mStream->send(
                Sirikata::MemoryReference(&((*data)[0]), data->size()),
                Sirikata::Network::ReliableOrdered
            );

            // If the packet can't fit on the queue, store it and indicate we want
            // a callback when its ready again.
            if (!success) {
                mWaitingPacket = next_pkt;
                mSerialized = data;
                return false;
            }

            if (next_pkt->source_port() == OBJECT_PORT_PING && next_pkt->dest_port() == OBJECT_PORT_PING) {
                TIMESTAMP(next_pkt, mTag);
            }

            // Otherwise the packet was handled and we can clean up and continue
            delete next_pkt;
            delete data;
            return true;
        }

        void doIter(ImplPtr me) {
            mIterationProfiler->finished();
            mWorkProfiler->started();

            pullInput(me);

            mIterationProfiler->started();
            mWorkProfiler->finished();
        }

        // Attempts to pull input. Continues as long as a) input is available and
        // b) the stream continues to accept the input.
        void pullInput(ImplPtr me) {
            if (mShutdown)
                return;

            bool sent = true;

            if (mWaitingPacket) {
                sent = trySend(mWaitingPacket);
                if (!sent)
                    return;
            }

            mWaitingPacket = NULL;
            mSerialized = NULL;

            while(sent) {
                PacketType* next_pkt = mParent->input(0).pull();

                // If we run out of input, start polling and exit
                if (next_pkt == NULL) {
                    startPoll(me);
                    return;
                }

                sent = trySend(next_pkt);

                // If we couldn't send it, store it for next time and wait for a ready callback
                if (!sent) {
                    // FIXME currently we just turn on polling again because we don't
                    // have an easy way to set the stream ready callback.
                    mStream->requestReadySendCallback();
                }
            }
        }

        void startPoll(ImplPtr me) {
            if (mShutdown)
                return;

            mTimer->wait(mMaxRate);
        }

        StreamTxElement* mParent; // This is the one element which may not always be valid
        Context* mContext;
        Sirikata::Network::Stream* mStream;
        IOStrand* mStrand;
        Duration mMaxRate;
        Trace::MessagePath mTag;
        PacketType* mWaitingPacket;
        std::string* mSerialized;
        bool mShutdown;
        IOTimerPtr mTimer;
        TimeProfiler::Stage* mIterationProfiler;
        TimeProfiler::Stage* mWorkProfiler;
    };
public:
    /** Create a stream transmit element which outputs to the specified stream,
     *  , and is polled at the specified maximum rate.
     *  \param strm the stream to transmit over
     *  \param strand the strand to run requests in
     *  \param max_rate the maximum rate (minimum duration between polls) for
     *         polling for new packets to send
     *  \param tag the tag to timestamp with
     */
    StreamTxElement(Context* ctx, Sirikata::Network::Stream* strm, IOStrand* strand, const Duration& max_rate, Trace::MessagePath tag)
     : mImpl( new Impl(this, ctx, strm, strand, max_rate, tag) )
    {
    }

    ~StreamTxElement() {
        mImpl->shutdown();
    }

    virtual bool push(uint32 port, PacketType* pkt) {
        assert(false);
        return false;
    }

    /** Start the polling process for this element.  Until this is called, the element will
     *  not be active.  This allows you time to connect to other elements before processing
     *  begins.
     */
    void run() {
        mImpl->start();
    }

    void shutdown() {
        mImpl->shutdown();
    }

    ReadySendCallback readySendCallback() {
        return mImpl->readySendCallback();
    }
private:
    ImplPtr mImpl;
}; // class StreamTxElement

} // namespace CBR

#endif //_CBR_STREAM_TX_ELEMENT_HPP_
