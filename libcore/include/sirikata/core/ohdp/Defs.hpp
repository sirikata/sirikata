// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OHDP_DEFS_HPP_
#define _SIRIKATA_LIBCORE_OHDP_DEFS_HPP_

#include <sirikata/core/xdp/Defs.hpp>

namespace Sirikata {
namespace OHDP {

typedef Sirikata::XDP::PortID PortID;

class SIRIKATA_EXPORT NodeID {
public:
    NodeID();
    NodeID(uint32 rhs);
    NodeID(const NodeID& rhs);

    /** Get a null NodeID. Equivalent to NodeID(0). */
    static const NodeID& null();
    /** Get a self NodeID, the one used for the local endpoint. Equivalent to
     *  null, NodeID(0).
     */
    static const NodeID& self();
    /** Get a NodeID that matches any other NodeID. */
    static const NodeID& any();

    NodeID& operator=(const NodeID& rhs);
    NodeID& operator=(uint32 rhs);

    operator uint32() const;

    bool operator==(const NodeID& rhs) const;
    bool operator!=(const NodeID& rhs) const;
    bool operator>(const NodeID& rhs) const;
    bool operator>=(const NodeID& rhs) const;
    bool operator<(const NodeID& rhs) const;
    bool operator<=(const NodeID& rhs) const;

    String toString() const;

    /** Returns true if the ports match, i.e. if they are equal or
     *  one of them is any().
     */
    bool matches(const NodeID& rhs) const;

    size_t hash() const { return std::tr1::hash<uint32>()(mValue); }

    class Hasher {
    public:
        size_t operator()(const NodeID& p) const {
            return p.hash();
        }
    };

private:
    uint32 mValue;
};

SIRIKATA_FUNCTION_EXPORT std::ostream& operator<<(std::ostream& os, const NodeID& rhs);

/// Combination of SpaceID and NodeID.
class SIRIKATA_EXPORT SpaceNodeID : public TotallyOrdered<SpaceNodeID> {
public:
    SpaceNodeID();
    SpaceNodeID(const SpaceID& s, const NodeID& n);

    const SpaceID& space() const { return mSpace; }
    const NodeID& node() const { return mNode; }

    /** Get a null SpaceNodeID. */
    static const SpaceNodeID& null();

    bool operator==(const SpaceNodeID& rhs) const;
    bool operator<(const SpaceNodeID& rhs) const;

    String toString() const;

    class Hasher {
    public:
        size_t operator()(const SpaceNodeID& p) const;
    };
private:
    SpaceID mSpace;
    NodeID mNode;
};

SIRIKATA_FUNCTION_EXPORT std::ostream& operator<<(std::ostream& os, const SpaceNodeID& rhs);

class Endpoint;

/** A fully qualified OHDP endpoint: SpaceID, NodeID, and PortID. The meaning of
 *  NodeID will depend on where it's used: it will map to individual space
 *  servers on the object host and to individual object hosts on the space
 *  server.
 *  Note that this does not have to be bound to unique values.  For instance,
 *  to specify coverage of all ports, PortID::any() could be used.  However,
 *  depending on context, the use of non-specific values may be invalid.
 */
class SIRIKATA_EXPORT Endpoint : public Sirikata::XDP::Endpoint<NodeID> {
public:
    typedef Sirikata::XDP::Endpoint<NodeID> EndpointBase;

    /** Function signature for an ODP message handler.  Takes a message header,
     *  containing the ODP routing information, and a MemoryReference containing the
     *  payload.
     */
    typedef std::tr1::function<void(const Endpoint& src, const Endpoint& dst, MemoryReference)> MessageHandler;


    Endpoint(const SpaceID& space, const NodeID& node, const PortID& port)
        : EndpointBase(space, node, port)
    {}
    Endpoint(const SpaceNodeID& spacenode, const PortID& port)
        : EndpointBase(spacenode.space(), spacenode.node(), port)
    {}

    /** Get a null Endpoint, i.e. one where each component is null. */
    static const Endpoint& null() {
        static Endpoint null_ep(SpaceID::null(), NodeID::null(), PortID::null());
        return null_ep;
    }
    /** Get an Endpoint that matches any other Endpoint, i.e. where each
     *  component is its respective any() value.
     */
    static const Endpoint& any() {
        static Endpoint any_ep(SpaceID::any(), NodeID::any(), PortID::any());
        return any_ep;
    }

    const NodeID& node() const { return EndpointBase::id(); }

private:
    Endpoint();
}; // class Endpoint

typedef Endpoint::MessageHandler MessageHandler;

} // namespace OHDP
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OHDP_DEFS_HPP_
