/*  Sirikata
 *  Defs.hpp
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

#ifndef _SIRIKATA_ODP_DEFS_HPP_
#define _SIRIKATA_ODP_DEFS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/xdp/Defs.hpp>

namespace Sirikata {
namespace ODP {

typedef Sirikata::XDP::PortID PortID;

class Endpoint;

/** A fully qualified ODP endpoint: SpaceID, ObjectReference, and PortID.
 *  Note that this does not have to be bound to unique values.  For instance,
 *  to specify coverage of all ports, PortID::any() could be used.  However,
 *  depending on context, the use of non-specific values may be invalid.
 */
class SIRIKATA_EXPORT Endpoint : public Sirikata::XDP::Endpoint<ObjectReference> {
public:
    typedef Sirikata::XDP::Endpoint<ObjectReference> EndpointBase;

    /** Function signature for an ODP message handler.  Takes a message header,
     *  containing the ODP routing information, and a MemoryReference containing the
     *  payload.
     */
    typedef std::tr1::function<void(const Endpoint& src, const Endpoint& dst, MemoryReference)> MessageHandler;


    Endpoint(const SpaceID& space, const ObjectReference& obj, const PortID& port)
        : EndpointBase(space, obj, port)
    {}
    Endpoint(const SpaceObjectReference& space_obj, const PortID& port)
        : EndpointBase(space_obj.space(), space_obj.object(), port)
    {}


    /** Get a null Endpoint, i.e. one where each component is null. */
    static const Endpoint& null() {
        static Endpoint null_ep(SpaceID::null(), ObjectReference::null(), PortID::null());
        return null_ep;
    }
    /** Get an Endpoint that matches any other Endpoint, i.e. where each
     *  component is its respective any() value.
     */
    static const Endpoint& any() {
        static Endpoint any_ep(SpaceID::any(), ObjectReference::any(), PortID::any());
        return any_ep;
    }

    const ObjectReference& object() const { return EndpointBase::id(); }
    SpaceObjectReference spaceObject() const { return SpaceObjectReference(space(), object()); }

private:
    Endpoint();
}; // class Endpoint

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_DEFS_HPP_
