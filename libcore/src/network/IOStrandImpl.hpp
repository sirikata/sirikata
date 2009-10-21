/*  Sirikata Network Utilities
 *  IOStrandImpl.hpp
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
#ifndef _SIRIKATA_IOSTRAND_INTERNAL_HPP_
#define _SIRIKATA_IOSTRAND_INTERNAL_HPP_

#include "IOStrand.hpp"
#include "Asio.hpp"

namespace Sirikata {
namespace Network {

// Note: We isolate this here because it requires including all the Boost.Asio
// headers to get the templatized version.  This functionality is only available
// internally -- only wrapped sockets and the like will use it -- so we
// factor it out so it only needs to be included for those implementations.

/** Handles wrapping callbacks for strands.  Should never actually be instantiated,
 *  simply provides one static template method which performs wrapping. The class
 *  only exists to allow us to implement this without exposing all the types
 *  to classes that
 */
class StrandWrapper {
public:
    /** Wrap the given handler so that it will be handled in this strand.
     *  Note: This
     *  \param strand the strand to wrap the handler for
     *  \param handler the handler which should be wrapped
     *  \returns a new handler which will cause the original handler
     *           to be invoked in this strand
     */
    template<typename CallbackType>
    static boost::asio::detail::wrapped_handler<boost::asio::io_service::strand, CallbackType> wrap(IOStrand& strand, const CallbackType& handler) {
        return strand.mImpl->wrap(handler);
    }
    /** Wrap the given handler so that it will be handled in this strand.
     *  Note: This
     *  \param strand the strand to wrap the handler for
     *  \param handler the handler which should be wrapped
     *  \returns a new handler which will cause the original handler
     *           to be invoked in this strand
     */
    template<typename CallbackType>
    static boost::asio::detail::wrapped_handler<boost::asio::io_service::strand, CallbackType> wrap(IOStrand* strand, const CallbackType& handler) {
        return strand->mImpl->wrap(handler);
    }
};


} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_IOSTRAND_INTERNAL_HPP_
