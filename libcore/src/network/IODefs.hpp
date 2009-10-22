/*  Sirikata Network Utilities
 *  IODefs.hpp
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
#ifndef _SIRIKATA_NETWORK_IODEFS_HPP_
#define _SIRIKATA_NETWORK_IODEFS_HPP_

#include "util/Platform.hpp"


// First we forward declare a bunch of Boost.Asio specific classes
// so we can use them without including everything that Boost.Asio
// includes indirectly
namespace boost {

namespace system {
class error_code;
class system_error;
class error_condition;
class error_category;
}

namespace asio {
class io_service;

namespace detail {
template <typename Dispatcher, typename Handler>
class wrapped_handler;
} // namespace detail

} // namespace asio
} // namespace boost



namespace Sirikata {
namespace Network {

// Exposes internal implementation service, allowing implementations to hook,
// directly in, but assumes that we could load alternative plugins with a
// different implementation
typedef boost::asio::io_service InternalIOService;

// Useful typedefs used throughout the Network IO API
typedef std::tr1::function<void()> IOCallback;

// The real classes we provide which attempt to abstract the event queue / IO
// services.
class IOService;
class IOServiceFactory;
class IOTimer;
class IOStrand;

// Subclasses of internal classes, exposed to allow for safe cross-library
// allocation and use.
class InternalIOStrand;
class TCPSocket;
class TCPListener;
class TCPResolver;
class UDPSocket;
class UDPResolver;
class DeadlineTimer;

// Subclasses of internal classes which are wrapped by a strand, guaranteeing
// serializability of their event handlers
class StrandTCPSocket;

} // namespace Network
} // namespace Sirikata


#endif //_SIRIKATA_NETWORK_IODEFS_HPP_
