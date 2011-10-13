// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OHDP_SERVICE_HPP_
#define _SIRIKATA_LIBCORE_OHDP_SERVICE_HPP_

#include <sirikata/core/ohdp/Defs.hpp>
#include <sirikata/core/xdp/Port.hpp>

namespace Sirikata {
namespace OHDP {

typedef Sirikata::XDP::Port<Endpoint> Port;

/** OHDP::Service is the interface provided by classes that are able to send OHDP
 *  messages. OHDP::Service mainly handles management of OHDP::Ports, which in
 *  turn allow sending and receiving of OHDP messages.
 *
 *  A Service allocates Ports and (behind the scenes) handles requests on the
 *  Ports, but once allocated the Port is owned by the allocator. The allocator
 *  is responsible for deleting all allocated Ports, even if the Service that
 *  generated them was destroyed.
 */
class SIRIKATA_EXPORT Service {
public:
    typedef Endpoint::MessageHandler MessageHandler;

    virtual ~Service() {}

    /** Bind an OHDP port for use.
     *  \param space the Space to communicate via
     *  \param node the node to communicate via
     *  \param port the PortID to attempt to bind
     *  \returns an OHDP Port object which can be used immediately, or NULL if
     *           the port is already bound
     *  \throws PortAllocationError if the Service cannot allocate the port for
     *          some reason other than it already being allocated.
     */
    virtual Port* bindOHDPPort(const SpaceID& space, const NodeID& node, PortID port) = 0;

    /** Bind a random, unused OHDP port for use.
     *  \param space the Space to communicate via
     *  \returns an OHDP Port object which can be used immediately, or, in
     *           extremely rare cases, NULL when an unused port isn't available
     *  \throws PortAllocationError if the Service cannot allocate the port for
     *          some reason other than it already being allocated.
     */
    virtual Port* bindOHDPPort(const SpaceID& space, const NodeID& node) = 0;

    /** Get a random, unused OHDP port. */
    virtual PortID unusedOHDPPort(const SpaceID& space, const NodeID& node) = 0;

    /** Register a handler for messages that arrive on unbound ports.  By
     *  default there is no handler and such messages are ignored.  Note that
     *  this handler will not be invoked for messages arriving at a bound port
     *  for which no handler has been registered.
     *  \param cb the handler for messages arriving at unbound ports
     */
    virtual void registerDefaultOHDPHandler(const MessageHandler& cb) = 0;

}; // class Service

} // namespace OHDP
} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OHDP_SERVICE_HPP_
