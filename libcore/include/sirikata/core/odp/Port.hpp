/*  Sirikata
 *  Port.hpp
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

#ifndef _SIRIKATA_ODP_PORT_HPP_
#define _SIRIKATA_ODP_PORT_HPP_

#include <sirikata/core/odp/Defs.hpp>

namespace Sirikata {
namespace ODP {

/** ODP::Port is the interface for bound ports that allows sending and receiving
 *  ODP messages.  By default no listeners are bound and messages will bypass
 *  the Port and be received by any more generic listeners registered with the
 *  parent ODP::Service.  To receive messages via this port, register a listener
 *  with receive() or receiveFrom().  Messages can only be sent via this port
 *  to endpoints in the same Space as this Port.
 *
 *  It is possible for multiple registered receive callbacks to be able to
 *  handle a message since the components of an endpoint support wildcards.  For
 *  example, if handlers were registered for SpaceID X, ObjectReference Y, and
 *  one with PortID 1 and one with PortID::any(), both would be capable of
 *  handling a message to (X, Y, 1). Resolution of handlers is always from
 *  more-detailed to less-detailed, *with PortID being highest priority,
 *  followed by SpaceID, with ObjectReference being lowest priority*.  This
 *  order is used because generally PortID's will be associated with a
 *  particular service or protocol, which is likely to function across multiple
 *  spaces and be indifferent to the object it is communicating with (for
 *  instance, a protocol which simply responds to requests for meshes).
 *  Therefore, handler resolution proceeds as follows for a message from
 *  endpoint (S, O, P):
 *   -# Check for handler for (S, O, P)
 *   -# Check for handler for (S, any, P)
 *   -# Check for handler for (any, any, P)
 *   -# Check for handler for (S, O, any)
 *   -# Check for handler for (S, any, any)
 *   -# Check for handler for (any, any, any)
 *  Note that combinations which have an object specified but not match any space
 *  are not checked.  This combination generally doesn't make sense since an
 *  object reference won't be valid across spaces.
 */
class SIRIKATA_EXPORT Port {
public:
    virtual ~Port() {}

    /** Get the endpoint associated with this port. Note that this will always
     *  be a full and unique endpoint, i.e. none of its components will be
     *  their corresponding ANY sentinal values.
     */
    virtual const Endpoint& endpoint() const = 0;

    /** Send an ODP message from this port. The destination endpoint *must* be
     *  unique and have the same SpaceID as this Port's endpoint.
     *  \param to the endpoint to send the message to
     *  \param payload the message payload
     *  \returns true if the message was accepted, false if the underlying
     *           networking layer couldn't immediately accept the message
     */
    virtual bool send(const Endpoint& to, MemoryReference payload) = 0;

    /** Register a default handler for messages received on this port. This is
     *  a utility method that is a shorthand for receiveFrom(Endpoint::any(),
     *  cb).
     *  \param cb handler to invoke when messages are receive
     */
    void receive(const MessageHandler& cb) {
        receiveFrom(Endpoint::any(), cb);
    }

    /** Register a handler for messages from the specified endpoint.
     *  \param from endpoint to dispatch messages from
     *  \param cb handler to invoke when messages are received
     */
    virtual void receiveFrom(const Endpoint& from, const MessageHandler& cb) = 0;
}; // class Port

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_PORT_HPP_
