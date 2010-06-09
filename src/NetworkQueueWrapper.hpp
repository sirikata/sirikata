/*  Sirikata
 *  NetworkQueueWrapper.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef _NETWORK_QUEUE_WRAPPER
#define _NETWORK_QUEUE_WRAPPER

#include "SpaceNetwork.hpp"
#include "Message.hpp"
#include "Statistics.hpp"

namespace Sirikata {
class NetworkQueueWrapper {
    Context* mContext;
    SpaceNetwork::ReceiveStream* mReceiveStream;
    Message* mFront;
    Trace::MessagePath mPathTag;
    typedef Network::Chunk Chunk;

    Message* parse(Chunk* c) {
        Message* msg = Message::deserialize(*c);

        if (msg == NULL) {
            // FIXME if this happens we're probably going to never remove the chunk from the network...
            SILOG(net,warning,"[NET] Couldn't parse message.");
            return NULL;
        }

        if (msg->source_server() != mReceiveStream->id()) {
            // FIXME if this happens we're probably going to never remove the chunk from the network...
            SILOG(net,warning,"[NET] Message source doesn't match connection's ID");
            delete msg;
            return NULL;
        }

        TIMESTAMP_PAYLOAD(msg, mPathTag);

        return msg;
    }
public:
    typedef Message* ElementType;

    NetworkQueueWrapper(Context* ctx, SpaceNetwork::ReceiveStream* rstrm, Trace::MessagePath tag)
     : mContext(ctx),
       mReceiveStream(rstrm),
       mFront(NULL),
       mPathTag(tag)
    {}

    ~NetworkQueueWrapper(){}

    QueueEnum::PushResult push(const Message *msg){
        return QueueEnum::PushExceededMaximumSize;
    }

    Message* front() {
        if (mFront == NULL) {
            Chunk* c = mReceiveStream->front();
            if (c != NULL)
                mFront = parse(c);
        }

        return mFront;
    }

    Message* pop(){
        Chunk* c = mReceiveStream->pop();

        if (c == NULL) {
            assert(mFront == NULL);
            return NULL;
        }

        Message* result = NULL;
        if (mFront != NULL) {
            result = mFront;
            mFront = NULL;
        }
        else {
            result = parse(c);
        }

        delete c;
        return result;
    }

    bool empty() const {
        return mFront == NULL && mReceiveStream->front() == NULL;
    }
};
}

#endif
