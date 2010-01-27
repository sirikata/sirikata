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

#ifndef _CBR_FORWARDER_SERVICE_QUEUE_HPP_
#define _CBR_FORWARDER_SERVICE_QUEUE_HPP_

#include "Utility.hpp"
#include "FairQueue.hpp"
#include "Message.hpp"

namespace CBR {

/** Fairly distributes inter-space node bandwidth between services, e.g. object
 *  message routing, PINTO, Loc, etc.
 */
class ForwarderServiceQueue {
  public:
    typedef uint32 ServiceID;

    class Listener {
      public:
        virtual void forwarderServiceMessageReady(ServerID dest_server) = 0;
    };

    ForwarderServiceQueue(ServerID this_server, uint32 size, Listener* listener);
    ~ForwarderServiceQueue();

    Message* front(ServerID sid);
    Message* pop(ServerID sid);
  private:
    friend class ForwarderServerMessageRouter;

    typedef FairQueue<Message, ServiceID, AbstractQueue<Message*> > OutgoingFairQueue;
    typedef std::tr1::unordered_map<ServerID, OutgoingFairQueue*> ServerQueueMap;

    ServerID mThisServer;
    ServerQueueMap mQueues;
    uint32 mQueueSize;
    Listener* mListener;
    boost::mutex mMutex;

    QueueEnum::PushResult push(ServiceID svc, Message* msg);
    uint32 size(ServerID sid, ServiceID svc);

    // Utilities

    OutgoingFairQueue* getFairQueue(ServerID sid);
    OutgoingFairQueue* checkServiceQueue(OutgoingFairQueue* ofq, ServiceID svc_id);
};

} // namespace CBR

#endif //_CBR_FORWARDER_SERVICE_QUEUE_HPP_
