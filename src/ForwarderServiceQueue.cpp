/*  cbr
 *  ForwarderServiceQueue.cpp
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


#include "ForwarderServiceQueue.hpp"
#include <boost/thread/locks.hpp>

namespace CBR {

ForwarderServiceQueue::ForwarderServiceQueue(ServerID this_server, uint32 size, Listener* listener)
        : mThisServer(this_server),
          mQueueSize(size),
          mListener(listener)
{
}

ForwarderServiceQueue::~ForwarderServiceQueue() {
    for(unsigned int i=0;i<mQueues.size();++i) {
        if(mQueues[i]) {
            delete mQueues[i];
        }
    }
}

Message* ForwarderServiceQueue::front(ServerID sid) {
    boost::lock_guard<boost::mutex> lock(mMutex);

    uint64 size=1<<30;
    ServiceID svc;

    return getFairQueue(sid)->front(&size, &svc);
}

Message* ForwarderServiceQueue::pop(ServerID sid) {
    boost::lock_guard<boost::mutex> lock(mMutex);

    uint64 size=1<<30;
    return getFairQueue(sid)->pop(&size);
}

QueueEnum::PushResult ForwarderServiceQueue::push(ServiceID svc, Message* msg) {
    assert(msg->source_server() == mThisServer);
    assert(msg->dest_server() != mThisServer);

    ServerID dest_server = msg->dest_server();

    QueueEnum::PushResult success;
    {
        boost::lock_guard<boost::mutex> lock(mMutex);
        OutgoingFairQueue* ofq = checkServiceQueue(getFairQueue(msg->dest_server()), svc);
        success = ofq->push(svc, msg);
    }
    if (success == QueueEnum::PushSucceeded)
        mListener->forwarderServiceMessageReady(dest_server);
    return success;
}

uint32 ForwarderServiceQueue::size(ServerID sid, ServiceID svc) {
    boost::lock_guard<boost::mutex> lock(mMutex);
    return checkServiceQueue(getFairQueue(sid), svc)->size(svc);
}


ForwarderServiceQueue::OutgoingFairQueue* ForwarderServiceQueue::getFairQueue(ServerID sid) {
    ServerQueueMap::iterator it = mQueues.find(sid);
    if (it == mQueues.end()) {
        mQueues[sid] = new OutgoingFairQueue();
        it = mQueues.find(sid);
    }
    assert(it != mQueues.end());

    return it->second;
}

ForwarderServiceQueue::OutgoingFairQueue* ForwarderServiceQueue::checkServiceQueue(OutgoingFairQueue* ofq, ServiceID svc_id) {
    if (!ofq->hasQueue(svc_id))
        ofq->addQueue(new Queue<Message*>(mQueueSize), svc_id, 1.0);
    return ofq;
}

} // namespace CBR
