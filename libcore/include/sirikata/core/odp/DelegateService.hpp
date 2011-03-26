/*  Sirikata
 *  DelegateService.hpp
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

#ifndef _SIRIKATA_ODP_DELEGATE_SERVICE_HPP_
#define _SIRIKATA_ODP_DELEGATE_SERVICE_HPP_

#include <sirikata/core/odp/Service.hpp>

namespace Sirikata {
namespace ODP {

class DelegatePort;

/** An implementation of ODP::Service which can be delegated to by providing a
 *  single function for allocating Ports when they are available.
 *  DelegateService takes care of all the other bookkeeping.  Generally this
 *  will be the simplest and best way to provide the Service interface.
 *
 *  This class works in conjunction with the DelegatePort class.  In order to
 *  function properly, your port creation function must return a DelegatePort
 *  (or subclass).
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
private:
    typedef std::tr1::unordered_map<PortID, DelegatePort*, PortID::Hasher> PortMap;
    typedef std::tr1::unordered_map<SpaceObjectReference, PortMap*, SpaceObjectReference::Hasher> SpacePortMap;

    friend class DelegatePort;
    // Deallocates the port by removing it from the data structure.  Should only
    // be used by DelegatePort destructor.
    void deallocatePort(DelegatePort* port);

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
