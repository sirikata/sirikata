/*  cbr
 *  ODPFlowScheduler.hpp
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

#ifndef _ODP_FLOW_SCHEDULER_HPP_
#define _ODP_FLOW_SCHEDULER_HPP_

#include "AbstractQueue.hpp"
#include "ForwarderServiceQueue.hpp"

namespace CBR {

/** An ODPFlowScheduler acts as a filter and queue for ODP messages for a single
 *  server. It has 2 primary roles. First, it acts as an ODP input queue for
 *  ForwarderServiceQueue; i.e. queues ODP messages, converts them to server
 *  messages, and serves them up for ForwarderServiceQueue.  Therefore it
 *  implements AbstractQueue<Message*>, but uses the
 *  ForwarderService::notifyPush back channel, and forces failure if somebody
 *  tries to push(Message*) to it.
 *
 *  Second, it collects statistics about active flows and makes that data
 *  available to the rest of the system.  This data is used in 2 ways: first, on
 *  the send side it is used to set the weights on the ServerMessageQueue to
 *  schedule between destination servers: i.e. its really just used to create a
 *  hierarchical fair queue.  It is also used by the receive side: the Forwarder
 *  has a control channel which forwards these weights to the destination, which
 *  uses them to determine the total weight of active flows for each source
 *  server, allowing it to properly balance the input rates.
 */
class ODPFlowScheduler : public AbstractQueue<Message*> {
public:
    typedef AbstractQueue<Message*>::Type Type;

    ODPFlowScheduler(SpaceContext* ctx, ForwarderServiceQueue* parent, ServerID sid, uint32 serv_id)
     : mContext(ctx),
       mParent(parent),
       mDestServer(sid),
       mServiceID(serv_id)
    {}

    virtual ~ODPFlowScheduler() {}

    // Interface: AbstractQueue<Message*>
    virtual QueueEnum::PushResult push(const Type& msg) { assert(false); }
    virtual const Type& front() const = 0;
    virtual Type& front() = 0;
    virtual Type pop() = 0;
    virtual bool empty() const = 0;
    virtual uint32 size() const = 0;

    // ODP push interface
    virtual bool push(CBR::Protocol::Object::ObjectMessage* msg) = 0;

    // Get the sum of the weights of active queues.
    virtual float totalActiveWeight() = 0;

protected:
    // Should be called by implementations when an ODP message is successfully added.
    void notifyPushFront() {
        mParent->notifyPushFront(mDestServer, mServiceID);
    }

    Message* createMessageFromODP(CBR::Protocol::Object::ObjectMessage* obj_msg, ServerID dest_serv) {
        Message* svr_obj_msg = new Message(
            mContext->id(),
            SERVER_PORT_OBJECT_MESSAGE_ROUTING,
            dest_serv,
            SERVER_PORT_OBJECT_MESSAGE_ROUTING,
            obj_msg
        );
        return svr_obj_msg;
    }

    SpaceContext* mContext;
    ForwarderServiceQueue* mParent;
    ServerID mDestServer;
    uint32 mServiceID;
}; // class ODPFlowScheduler

} // namespace CBR

#endif //_ODP_FLOW_SCHEDULER_HPP_
