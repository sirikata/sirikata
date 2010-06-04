/*  Sirikata
 *  DelegatePort.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>

namespace Sirikata {
namespace ODP {

DelegatePort::DelegatePort(DelegateService* parent, const Endpoint& ep, SendFunction send_func)
 : mParent(parent),
   mEndpoint(ep),
   mSendFunc(send_func),
   mFromHandlers()
{
}

DelegatePort::~DelegatePort() {
    // Get it out of the parent map to ensure we won't get any more messages
    mParent->deallocatePort(this);
}

const Endpoint& DelegatePort::endpoint() const {
    return mEndpoint;
}

bool DelegatePort::send(const Endpoint& to, MemoryReference payload) {
    return mSendFunc(to, payload);
}

void DelegatePort::receiveFrom(const Endpoint& from, const MessageHandler& cb) {
    mFromHandlers[from] = cb;
}

bool DelegatePort::deliver(const RoutableMessageHeader& header, MemoryReference data) const {
    Endpoint ep(header.source_space(), header.source_object(), header.source_port());

    // See ODP::Port documentation for details on this ordering.
    if (tryDeliver(ep, header, data)) return true;
    if (tryDeliver(Endpoint(ep.space(), ObjectReference::any(), ep.port()), header, data)) return true;
    if (tryDeliver(Endpoint(SpaceID::any(), ObjectReference::any(), ep.port()), header, data)) return true;
    if (tryDeliver(Endpoint(ep.space(), ep.object(), PortID::any()), header, data)) return true;
    if (tryDeliver(Endpoint(ep.space(), ObjectReference::any(), PortID::any()), header, data)) return true;
    if (tryDeliver(Endpoint(SpaceID::any(), ObjectReference::any(), PortID::any()), header, data)) return true;

    return false;
}

bool DelegatePort::tryDeliver(const Endpoint& ep, const RoutableMessageHeader& header, MemoryReference data) const {
    ReceiveFromHandlers::const_iterator rit = mFromHandlers.find(ep);
    if (rit == mFromHandlers.end())
        return false;

    const MessageHandler& handler = rit->second;
    handler(header, data);
    return true;
}

} // namespace ODP
} // namespace Sirikata
