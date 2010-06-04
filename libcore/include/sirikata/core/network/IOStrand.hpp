/*  Sirikata Network Utilities
 *  IOStrand.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
#ifndef _SIRIKATA_IOSTRAND_HPP_
#define _SIRIKATA_IOSTRAND_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/IODefs.hpp>

namespace Sirikata {
namespace Network {

/** IOStrands provide guaranteed serialized event handling.  Strands
 *  allow you to wrap events so that they may be dispatched to any
 *  currently running IOService (in thread), but guarantees that no
 *  two event handlers in the same strand will be executed
 *  concurrently.  This is a a useful synchronization primitive, allow
 *  user code to avoid having to use locks explicitly and allowing the
 *  IOService to make the best use of available threads instead of
 *  locking operations to specific threads, causing unnecessary thread
 *  switching overhead.
 *
 *  Note that strands are never directly allocated -- instead, they
 *  are acquired from an existing IOService.
 */
class SIRIKATA_EXPORT IOStrand {
    IOService& mService;
    InternalIOStrand* mImpl;

    friend class IOService;

    /** Construct an IOStrand associated with the given IOService. */
    IOStrand(IOService& io);

  protected:

    friend class StrandTCPSocket;

  public:

    template<typename CallbackType>
    class WrappedHandler;

    /** Destroy the strand.  Outstanding events on the strand will be
     *  handled safely, i.e. non-concurrently.
     */
    ~IOStrand();

    /** Get an IOService associated with this strand. */
    IOService& service() const;

    /** Request that the given handler be invoked, possibly before
     *  returning, on this strand.
     *  \param handler the handler callback to be called
     */
    void dispatch(const IOCallback& handler);

    /** Request that the given handler be appended to the event queue
     *  and invoked on this strand at a later time.  The handler is
     *  guaranteed not to be invoked will not be invoked during this
     *  method call.
     *  \param handler the handler callback to be called
     */
    void post(const IOCallback& handler);
    /** Request that the given handler be appended to the event queue
     *  and invoked on this strand at a later time. Regardless of the
     *  wait duration requested, the handler is guaranteed not to be
     *  invoked during this method call.
     *  \param waitFor the length of time to wait before invoking the handler
     *  \param handler the handler callback to be called
     */
    void post(const Duration& waitFor, const IOCallback& handler);

    /** Wrap the given handler so that it will be handled in this strand.
     *  \param handler the handler which should be wrapped
     *  \returns a new handler which will cause the original handler
     *           to be invoked in this strand
     */
    IOCallback wrap(const IOCallback& handler);


    /** Wrap the given handler so that it will be handled in this strand.
     *  \param handler the handler which should be wrapped
     *  \returns a new handler which will cause the original handler
     *           to be invoked in this strand
     */
    template<typename CallbackType>
    WrappedHandler<CallbackType> wrap(const CallbackType& handler);
};

} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_IOSERVICE_HPP_
