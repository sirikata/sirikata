// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_XDP_DEFS_HPP_
#define _SIRIKATA_LIBCORE_XDP_DEFS_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/util/SpaceID.hpp>

namespace Sirikata {
/** XDP stands for X datagram protocol. It isn't used directly, but has shared
 * classes and template classes for datagram protocol interfaces, e.g. ODP for
 * objects and OHDP for object hosts.
 */
namespace XDP {

class PortID;
template<typename IdentifierType>
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
    bool operator!=(const PortID& rhs) const;
    bool operator>(const PortID& rhs) const;
    bool operator>=(const PortID& rhs) const;
    bool operator<(const PortID& rhs) const;
    bool operator<=(const PortID& rhs) const;

    PortID& operator++ ();
    PortID operator++ (int);

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
// We can't actually define this here since it depends on the IdentifierType
//typedef std::tr1::function<void(const Endpoint& src, const Endpoint& dst, MemoryReference)> MessageHandler;

/** A fully qualified endpoint, including the unique identifier and port.
 *  Note that this does not have to be bound to unique values.  For instance,
 *  to specify coverage of all ports, PortID::any() could be used.  However,
 *  depending on context, the use of non-specific values may be invalid.
 *
 *  Endpoints always include a space, unique identifier, and port. This class is
 *  templated on the identifier part, which must be comparable, have a Hasher
 *  nested class, and have static any() and null() methods.
 */
template<typename IdentifierType>
class Endpoint {
public:
    typedef IdentifierType Identifier;

    Endpoint(const SpaceID& space, const Identifier& id, const PortID& port)
     : mSpace(space),
       mID(id),
       mPort(port)
    {
    }

    bool operator==(const Endpoint& rhs) const {
        return (
            mSpace == rhs.mSpace &&
            mID == rhs.mID &&
            mPort == rhs.mPort
        );
    }
    bool operator!=(const Endpoint& rhs) const {
        return !(*this == rhs);
    }
    bool operator>(const Endpoint& rhs) const {
        return (
            mSpace > rhs.mSpace || (
                mSpace == rhs.mSpace && (
                    mID > rhs.mID || (
                        mID == rhs.mID && (
                            mPort > rhs.mPort
                        )
                    )
                )
            )
        );
    }
    bool operator>=(const Endpoint& rhs) const {
        return !(*this < rhs);
    }
    bool operator<(const Endpoint& rhs) const {
        return (
            mSpace < rhs.mSpace || (
                mSpace == rhs.mSpace && (
                    mID < rhs.mID || (
                        mID == rhs.mID && (
                            mPort < rhs.mPort
                        )
                    )
                )
            )
        );
    }
    bool operator<=(const Endpoint& rhs) const {
        return !(*this > rhs);
    }


    /** Returns true if the endpoint matches this one, i.e. if all
     *  components match, either precisely or because one of them is
     *  any().
     */
    bool matches(const Endpoint& rhs) const {
        return (
            mSpace.matches(rhs.mSpace) &&
            mID.matches(rhs.mID) &&
            mPort.matches(rhs.mPort)
        );
    }

    const SpaceID& space() const { return mSpace; }
    const Identifier& id() const { return mID; }
    const PortID& port() const { return mPort; }

    String toString() const {
        std::ostringstream ss;
        ss << (*this);
        return ss.str();
    }

    class Hasher {
    public:
        size_t operator()(const Endpoint& p) const {
            return (
                SpaceID::Hasher()(p.mSpace) ^
                typename Identifier::Hasher()(p.mID) ^
                PortID::Hasher()(p.mPort)
            );
        }
    };
private:
    Endpoint();

    SpaceID mSpace;
    Identifier mID;
    PortID mPort;
}; // class Endpoint

template<typename IdentifierType>
std::ostream& operator<<(std::ostream& os, const Sirikata::XDP::Endpoint<IdentifierType>& ep) {
    os << ep.space() << ":" << ep.id() << ":" << ep.port();
    return os;
}

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_DEFS_HPP_
