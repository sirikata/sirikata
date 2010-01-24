/*  Sirikata Network Utilities
 *  IOService.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _SIRIKATA_IOSERVICE_HPP_
#define _SIRIKATA_IOSERVICE_HPP_

#include "../util/Platform.hpp"
#include "IODefs.hpp"

namespace Sirikata {
namespace Network {

/** IOService provides queuing, processing, and dispatch for
 *  asynchronous IO, including timers, sockets, resolvers, and simple
 *  tasks.
 *
 *  Note that currently you cannot add your own services to this.
 *  Therefore, the only way to extend the abilities of the IOService
 *  is via the existing mechanisms -- the default implementations for
 *  sockets and timers or via periodic tasks.
 */
class SIRIKATA_EXPORT IOService {
    InternalIOService* mImpl;
    bool mOwn;

    IOService();
    ~IOService();

    friend class IOServiceFactory;

  protected:

    friend class InternalIOStrand;
    friend class InternalIOWork;
    friend class TCPSocket;
    friend class TCPListener;
    friend class TCPResolver;
    friend class UDPSocket;
    friend class UDPResolver;
    friend class DeadlineTimer;
    friend class IOTimer;

    /** Get the underlying IOService.  Only made available to allow for
     *  efficient implementation of ASIO provided functionality such as
     *  tcp/udp sockets and deadline timers.
     */
    InternalIOService& asioService() {
        return *mImpl;
    }
    /** Get the underlying IOService.  Only made available to allow for
     *  efficient implementation of ASIO provided functionality such as
     *  tcp/udp sockets and deadline timers.
     */
    const InternalIOService& asioService() const {
        return *mImpl;
    }

    /** Generated an IOService wrapper from the internal version.  The
     *  generated IOService will *not* take ownership.  This should only
     *  be used to allow IO objects to return a reference to their owner.
     */
    IOService(InternalIOService* bs);

public:
    /* Copies the IOService.  The underlying resources remain owned by the
     * original and will be controlled by its lifetime.
     * \param cpy the original IOService to copy
     */
    IOService(const IOService& cpy);

    IOService& operator=(IOService& rhs);

    /** Creates a new IOStrand. */
    IOStrand* createStrand();

    /** Run at most one handler in the event queue.
     *  \returns the number of handlers executed
     */
    uint32 pollOne();
    /** Run as many handlers as can be executed without blocking.
     *  \returns the number of handlers executed
     */
    uint32 poll();

    /** Run at most one handler, blocking if appropriate.
     *  \returns the number of handlers executed
     */
    uint32 runOne();
    /** Run as many handlers as are available, blocking as
     *  appropriate.  This will not return unless no more work is
     *  available.
     *  \returns the number of handlers executed
     */
    uint32 run();

    /** Stop event processing, cancelling events as necessary. */
    void stop();
    /** Reset the event processing, discarding events as necessary and
     *  preparing it for additional run/runOne/poll/pollOne calls.
     */
    void reset();

    /** Request that the given handler be invoked, possibly before
     *  returning.
     *  \param handler the handler callback to be called
     */
    void dispatch(const IOCallback& handler);

    /** Request that the given handler be appended to the event queue
     *  and invoked later.  The handler will not be invoked during
     *  this method call.
     *  \param handler the handler callback to be called
     */
    void post(const IOCallback& handler);
    /** Request that the given handler be appended to the event queue
     *  and invoked later.  Regardless of the wait duration requested,
     *  the handler will never be invoked during this method call.
     *  \param waitFor the length of time to wait before invoking the handler
     *  \param handler the handler callback to be called
     */
    void post(const Duration& waitFor, const IOCallback& handler);
};

} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_IOSERVICE_HPP_
