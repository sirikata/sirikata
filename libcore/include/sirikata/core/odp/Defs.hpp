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
#include <sirikata/core/util/KnownServices.hpp>


namespace Sirikata {
namespace ODP {

class PortID;
class Endpoint;

/** Identifier for an ODP port. Under the hood this is simply a uint32, but this
 *  class provides additional features: null and any values, matching (which
 *  differs from equality), etc.  Because the format of PortID is fixed to
 *  uint32, the raw value is exposed directly -- a PortID can be be cast
 *  directly to a uint32.
 */
class SIRIKATA_EXPORT PortID {
public:
    PortID();
    PortID(uint32 rhs);
    PortID(const PortID& rhs);

    /** Get a null PortID. Equivalent to PortID(0). */
    static const PortID& null();
    /** Get a PortID that matches any other PortID. */
    static const PortID& any();

    PortID& operator=(const PortID& rhs);
    PortID& operator=(uint32 rhs);

    operator uint32() const;

    bool operator==(const PortID& rhs) const;
    bool operator==(const Services::Ports& compareTo) const;
    bool operator!=(const PortID& rhs) const;
    bool operator!=(const Services::Ports& compareTo) const;
    bool operator>(const PortID& rhs) const;
    bool operator>=(const PortID& rhs) const;
    bool operator<(const PortID& rhs) const;
    bool operator<=(const PortID& rhs) const;

    /** Returns true if the ports match, i.e. if they are equal or
     *  one of them is any().
     */
    bool matches(const PortID& rhs) const;

    class Hasher {
    public:
        size_t operator()(const PortID& p) const {
            return std::tr1::hash<uint32>()(p.mValue);
        }
    };

private:
    uint32 mValue;
};

/** Function signature for an ODP message handler.  Takes a message header,
 *  containing the ODP routing information, and a MemoryReference containing the
 *  payload.
 */
typedef std::tr1::function<void(const Endpoint& src, const Endpoint& dst, MemoryReference)> MessageHandler;
typedef std::tr1::function<void(const RoutableMessageHeader&, MemoryReference)> OldMessageHandler;


/** A fully qualified ODP endpoint: SpaceID, ObjectReference, and PortID.
 *  Note that this does not have to be bound to unique values.  For instance,
 *  to specify coverage of all ports, PortID::any() could be used.  However,
 *  depending on context, the use of non-specific values may be invalid.
 */
class SIRIKATA_EXPORT Endpoint {
public:
    Endpoint(const SpaceID& space, const ObjectReference& obj, const PortID& port);
    Endpoint(const SpaceObjectReference& space_obj, const PortID& port);

    bool operator==(const Endpoint& rhs) const;
    bool operator!=(const Endpoint& rhs) const;
    bool operator>(const Endpoint& rhs) const;
    bool operator>=(const Endpoint& rhs) const;
    bool operator<(const Endpoint& rhs) const;
    bool operator<=(const Endpoint& rhs) const;

    /** Returns true if the endpoint matches this one, i.e. if all
     *  components match, either precisely or because one of them is
     *  any().
     */
    bool matches(const Endpoint& rhs) const;

    /** Get a null Endpoint, i.e. one where each component is null. */
    static const Endpoint& null();
    /** Get an Endpoint that matches any other Endpoint, i.e. where each
     *  component is its respective any() value.
     */
    static const Endpoint& any();

    const SpaceID& space() const;
    const ObjectReference& object() const;
    const PortID& port() const;

    String toString() const;

    class Hasher {
    public:
        size_t operator()(const Endpoint& p) const {
            return (
                SpaceID::Hasher()(p.mSpace) ^
                ObjectReference::Hasher()(p.mObject) ^
                PortID::Hasher()(p.mPort)
            );
        }
    };
private:
    Endpoint();

    SpaceID mSpace;
    ObjectReference mObject;
    PortID mPort;
}; // class Endpoint

SIRIKATA_FUNCTION_EXPORT std::ostream& operator<<(std::ostream& os, const Sirikata::ODP::Endpoint& ep);

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_DEFS_HPP_
