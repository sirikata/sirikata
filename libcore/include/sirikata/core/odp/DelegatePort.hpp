/*  Sirikata
 *  DelegatePort.hpp
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

#ifndef _SIRIKATA_ODP_DELEGATE_PORT_HPP_
#define _SIRIKATA_ODP_DELEGATE_PORT_HPP_

#include <sirikata/core/odp/Port.hpp>

namespace Sirikata {
namespace ODP {

class DelegateService;

/** An implementation of ODP::Port that handles all the bookkeeping and sanity
 *  checking, but delegates real operations to the user of the class via some
 *  callback functions registered at creation.
 *
 *  Note that this class works in conjunction with DelegateService.
 */
class SIRIKATA_EXPORT DelegatePort : public Port {
public:
    typedef std::tr1::function<bool(const Endpoint&, MemoryReference payload)> SendFunction;

    /** Create a new DelegatePort with the given properties which handles sends
     *  via send_func.
     *  \param parent the parent DelegateService that manages this port
     *  \param ep the endpoint identifier for this port, must be fully qualified
     *  \param send_func functor to invoke to send messages
     */
    DelegatePort(DelegateService* parent, const Endpoint& ep, SendFunction send_func);
    virtual ~DelegatePort();

    // Port Interface
    virtual const Endpoint& endpoint() const;
    virtual bool send(const Endpoint& to, MemoryReference payload);
    virtual void receiveFrom(const Endpoint& from, const MessageHandler& cb);
    virtual void receiveFrom(const Endpoint& from, const OldMessageHandler& cb);

    /** Deliver a message via this port.  Returns true if a receiver was found
     *  for the message, false if none was found and it was ignored.
     *  \deprecated Prefer the version using ODP::Endpoints
     */
    bool deliver(const RoutableMessageHeader& header, MemoryReference data) const;

    /** Deliver a message via this port.  Returns true if a receiver was found
     *  for the message, false if none was found and it was ignored.
     */
    bool deliver(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference data) const;

private:
    // Worker method for deliver, tries to deliver to the handler for this exact
    // endpoint.
    // \deprecated Prefer the version using both ODP::Endpoints
    bool tryDeliver(const Endpoint& ep, const RoutableMessageHeader& header, MemoryReference data) const;

    // Worker method for deliver, tries to deliver to the handler for this exact
    // endpoint.
    // \deprecated Prefer the version using both ODP::Endpoints
    bool tryDeliver(const Endpoint& src, const Endpoint& dst, MemoryReference data) const;

    typedef std::tr1::unordered_map<Endpoint, MessageHandler, Endpoint::Hasher> ReceiveFromHandlers;
    typedef std::tr1::unordered_map<Endpoint, OldMessageHandler, Endpoint::Hasher> ReceiveFromOldHandlers;

    DelegateService* mParent;
    Endpoint mEndpoint;
    SendFunction mSendFunc;
    ReceiveFromHandlers mFromHandlers;
    ReceiveFromOldHandlers mFromOldHandlers;
}; // class DelegatePort

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_DELEGATE_PORT_HPP_
