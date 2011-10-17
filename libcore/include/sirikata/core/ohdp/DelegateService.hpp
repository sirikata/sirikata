// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_OHDP_DELEGATE_SERVICE_HPP_
#define _SIRIKATA_OHDP_DELEGATE_SERVICE_HPP_

#include <sirikata/core/ohdp/Service.hpp>
#include <sirikata/core/xdp/DelegatePort.hpp>

namespace Sirikata {
namespace OHDP {

class DelegateService;

typedef Sirikata::XDP::DelegatePort<Endpoint, DelegateService, Port> DelegatePort;

/** An implementation of OHDP::Service which manages most bookkeeping for you,
 *  delegating only for Port creation. See ODP::Service for more details as
 *  these are essentially the same.
 */
class SIRIKATA_EXPORT DelegateService : public Service {
public:
    typedef std::tr1::function<DelegatePort*(DelegateService*, const Endpoint&)> PortCreateFunction;

    /** Create a DelegateService that uses create_func to generate new ports.
     *  \param create_func a function which accepts a SpaceID and PortID and
     *                     returns a new Port object. It only needs to generate
     *                     the ports; the DelegateService class will handle
     *                     other bookkeeping and error checking.
     */
    DelegateService(PortCreateFunction create_func);

    virtual ~DelegateService();

    // Service Interface
    virtual Port* bindOHDPPort(const SpaceID& space, const NodeID& node, PortID port);
    virtual Port* bindOHDPPort(const SpaceID& space, const NodeID& node);
    virtual PortID unusedOHDPPort(const SpaceID& space, const NodeID& node);
    virtual void registerDefaultOHDPHandler(const MessageHandler& cb);

    /** Deliver a message to this subsystem.
     *  \param src source endpoint
     *  \param dst destination endpoint
     *  \param data the payload of the message
     *  \returns true if the message was handled, false otherwise.
     */
    bool deliver(const Endpoint& src, const Endpoint& dst, MemoryReference data) const;

    // Deallocates the port by removing it from the data structure.  Should only
    // be used by DelegatePort destructor.
    // This should be private with friend class DelegatePort but that doesn't
    // work because DelegatePort is a typedef. It doesn't particularly matter
    // since other's shouldn't see DelegatePort* anyway, they use Port*.
    void deallocatePort(DelegatePort* port);
private:
    // We need a key that includes SpaceID and NodeID. This is a minimal pair
    // class to serve that purpose.
    struct SpaceIDNodeID : public TotallyOrdered<SpaceIDNodeID> {
        SpaceIDNodeID() {}
        SpaceIDNodeID(const SpaceID& _s, const NodeID& _n)
         : space(_s), node(_n)
        {}

        // Basic comparisons, rest are provided by TotallyOrdered
        bool operator==(const SpaceIDNodeID& rhs) const;
        bool operator<(const SpaceIDNodeID& rhs) const;

        struct Hasher {
            size_t operator()(const SpaceIDNodeID& snid) const;
        };

        SpaceID space;
        NodeID node;
    };

    typedef std::tr1::unordered_map<PortID, DelegatePort*, PortID::Hasher> PortMap;
    typedef std::tr1::unordered_map<SpaceIDNodeID, PortMap*, SpaceIDNodeID::Hasher> SpacePortMap;

    // Helper. Gets an existing PortMap for the specified space or returns NULL
    // if one doesn't exist yet.
    PortMap* getPortMap(const SpaceIDNodeID& snid) const;
    // Helper. Gets an existing PortMap for the specified space or creates one
    // if one doesn't exist yet.
    PortMap* getOrCreatePortMap(const SpaceIDNodeID& snid);

    PortCreateFunction mCreator;
    SpacePortMap mSpacePortMap;
    MessageHandler mDefaultHandler;
}; // class DelegateService

} // namespace OHDP
} // namespace Sirikata

#endif //_SIRIKATA_OHDP_DELEGATE_SERVICE_HPP_
