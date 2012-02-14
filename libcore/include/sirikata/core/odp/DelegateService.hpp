// Copyright (c) 2010 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_ODP_DELEGATE_SERVICE_HPP_
#define _SIRIKATA_ODP_DELEGATE_SERVICE_HPP_

#include <sirikata/core/odp/Service.hpp>
#include <sirikata/core/xdp/DelegatePort.hpp>

namespace Sirikata {
namespace ODP {

class DelegateService;

typedef Sirikata::XDP::DelegatePort<Endpoint, DelegateService, Port> DelegatePort;

/** An implementation of ODP::Service which can be delegated to by providing a
 *  single function for allocating Ports when they are available.
 *  DelegateService takes care of all the other bookkeeping.  Generally this
 *  will be the simplest and best way to provide the Service interface.
 *
 *  This class works in conjunction with the DelegatePort class.  In order to
 *  function properly, your port creation function must return a DelegatePort
 *  (or subclass).
 *
 *  To ease memory management, the DelegateService notifies DelegatePorts when
 *  they have become invalid (e.g. the DelegateService is being deleted). As the
 *  Port is owned by the allocator, it remains valid memory (it is not deleted)
 *  but it will no longer function normally, producing warnings that it has been
 *  invalidated.
 */
class SIRIKATA_EXPORT DelegateService : public Service {
public:
    typedef std::tr1::function<DelegatePort*(DelegateService*,const SpaceObjectReference&,PortID)> PortCreateFunction;

    /** Create a DelegateService that uses create_func to generate new ports.
     *  \param create_func a function which accepts a SpaceID and PortID and
     *                     returns a new Port object. It only needs to generate
     *                     the ports; the DelegateService class will handle
     *                     other bookkeeping and error checking.
     */
    DelegateService(PortCreateFunction create_func);

    virtual ~DelegateService();

    // Service Interface
    virtual Port* bindODPPort(const SpaceID& space, const ObjectReference& objref, PortID port);
    virtual Port* bindODPPort(const SpaceObjectReference& sor, PortID port);
    virtual Port* bindODPPort(const SpaceID& space, const ObjectReference& objref);
    virtual Port* bindODPPort(const SpaceObjectReference& sor);
    virtual PortID unusedODPPort(const SpaceID& space, const ObjectReference& objref);
    virtual PortID unusedODPPort(const SpaceObjectReference& sor);
    virtual void registerDefaultODPHandler(const MessageHandler& cb);

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
    typedef std::tr1::unordered_map<PortID, DelegatePort*, PortID::Hasher> PortMap;
    typedef std::tr1::unordered_map<SpaceObjectReference, PortMap*, SpaceObjectReference::Hasher> SpacePortMap;

    // Helper. Gets an existing PortMap for the specified space or returns NULL
    // if one doesn't exist yet.
    PortMap* getPortMap(const SpaceObjectReference& sor) const;
    // Helper. Gets an existing PortMap for the specified space or creates one
    // if one doesn't exist yet.
    PortMap* getOrCreatePortMap(const SpaceObjectReference& sor);

    PortCreateFunction mCreator;
    SpacePortMap mSpacePortMap;
    MessageHandler mDefaultHandler;
}; // class DelegateService

} // namespace ODP
} // namespace Sirikata

#endif //_SIRIKATA_ODP_DELEGATE_SERVICE_HPP_
