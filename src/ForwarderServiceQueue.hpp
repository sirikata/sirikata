/*  cbr
 *  ForwarderServiceQueue.hpp
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

#include "Utility.hpp"
#include "FairQueue.hpp"
#include "Message.hpp"

namespace CBR {

/** Fairly distributes inter-space node bandwidth between services, e.g. object
 *  message routing, PINTO, Loc, etc.
 */
class ForwarderServiceQueue {
  private:
    typedef FairQueue<Message, ServerMessageRouter::SERVICES, AbstractQueue<Message*> > OutgoingFairQueue;
    std::vector<OutgoingFairQueue*> mQueues;
    uint32 mQueueSize;

  public:
    ForwarderServiceQueue(uint32 size){
        mQueueSize=size;
    }

    ~ForwarderServiceQueue() {
        for(unsigned int i=0;i<mQueues.size();++i) {
            if(mQueues[i]) {
                delete mQueues[i];
            }
        }
    }

    QueueEnum::PushResult push(ServerID sid, ServerMessageRouter::SERVICES svc, Message* msg) {
        return getFairQueue(sid).push(svc, msg);
    }

    // Get the front element and service it camef
    Message* front(ServerID sid) {
        uint64 size=1<<30;
        ServerMessageRouter::SERVICES svc;

        return getFairQueue(sid).front(&size, &svc);
    }

    Message* pop(ServerID sid) {
        uint64 size=1<<30;
        return getFairQueue(sid).pop(&size);
    }

  private:
    OutgoingFairQueue& getFairQueue(ServerID sid) {
        while (mQueues.size()<=sid) {
            mQueues.push_back(NULL);
        }
        if(!mQueues[sid]) {
            mQueues[sid]=new OutgoingFairQueue();
            for(unsigned int i=0;i<ServerMessageRouter::NUM_SERVICES;++i) {
                mQueues[sid]->addQueue(new Queue<Message*>(mQueueSize),(ServerMessageRouter::SERVICES)i,1.0);
            }
        }
        return *mQueues[sid];
    }

};

} // namespace CBR
