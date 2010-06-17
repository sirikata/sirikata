/*  Sirikata
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

#ifndef _SIRIKATA_FORWARDER_SERVICE_QUEUE_HPP_
#define _SIRIKATA_FORWARDER_SERVICE_QUEUE_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/queue/FairQueue.hpp>
#include <sirikata/cbrcore/Message.hpp>
#include <boost/thread.hpp>

namespace Sirikata {

/** Fairly distributes inter-space node bandwidth between services, e.g. object
 *  message routing, PINTO, Loc, etc.  Messages are broken down by destination
 *  server, then by service.  The entire ForwarderServiceQueue is a set of fair
 *  queues over services, grouped by destination server.
 */
class ForwarderServiceQueue {
  public:
    typedef uint32 ServiceID;
    typedef AbstractQueue<Message*> MessageQueue;
    // Functor for creating a new input message queue. Must take 2 parameters.
    // The first is the ServerID this queue is being allocated for .  The second
    // is a uint32 parameter specifying its maximum size.  This will be invoked
    // once for each (server, service) pair.
    typedef std::tr1::function<MessageQueue*(ServerID,uint32)> MessageQueueCreator;

    class Listener {
      public:
        virtual ~Listener() {}
        virtual void forwarderServiceMessageReady(ServerID dest_server) = 0;
    };

    ForwarderServiceQueue(ServerID this_server, uint32 size, Listener* listener);
    ~ForwarderServiceQueue();

    void addService(ServiceID svc, MessageQueueCreator creator = 0);

    Message* front(ServerID sid);
    Message* pop(ServerID sid);
    bool empty(ServerID sid);
  private:
    friend class Forwarder;
    friend class ForwarderServerMessageRouter;
    friend class ODPFlowScheduler;

    typedef FairQueue<Message, ServiceID, MessageQueue> OutgoingFairQueue;
    typedef std::tr1::unordered_map<ServerID, OutgoingFairQueue*> ServerQueueMap;
    typedef std::tr1::unordered_map<ServiceID, MessageQueueCreator> MessageQueueCreatorMap;

    ServerID mThisServer;
    MessageQueueCreatorMap mQueueCreators;
    ServerQueueMap mQueues;
    uint32 mQueueSize;
    Listener* mListener;
    boost::mutex mMutex;

    // Normal push
    QueueEnum::PushResult push(ServiceID svc, Message* msg);
    // Use prePush to indicate you need to push to the queue.  This has to be
    // here because we need to keep organized with the
    // Forwarder/ODPFlowSchedulers, which need us to get them allocated before
    // they can be pushed to.
    void prePush(ServerID sid);
    // Notifies the queue that a push has been successfully performed for the
    // given service on the given server.
    void notifyPushFront(ServerID sid, ServiceID svc);

    uint32 size(ServerID sid, ServiceID svc);

    // Utilities

    // Gets the FairQueue over services for the specified server.
    OutgoingFairQueue* getServerFairQueue(ServerID sid);
    // This is just a sanity check -- verifies ofq has an input queue for svc_id
    // and returns ofq.
    OutgoingFairQueue* checkServiceQueue(OutgoingFairQueue* ofq, ServiceID svc_id);
};

} // namespace Sirikata

#endif //_SIRIKATA_FORWARDER_SERVICE_QUEUE_HPP_
