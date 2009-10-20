/*  Sirikata Network Utilities
 *  AsioStrandIO.hpp
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

#ifndef _SIRIKATA_ASIO_STRAND_IO_HPP_
#define _SIRIKATA_ASIO_STRAND_IO_HPP_

/** Note: The classes provided by this file are thin wrappers around the
 *  normal Asio classes, but override most methods to guarantee that
 *  all operations are handled from a user specified strand. These are
 *  merely a convenience, since the user could always manually wrap handlers
 *  using the strand they provide.
 */

#include "IODefs.hpp"
#include "Asio.hpp"
#include "IOStrand.hpp"
#include "IOStrandImpl.hpp"

namespace Sirikata {
namespace Network {

/** A TCPSocket which guarantees all handlers for events it generates are
 *  handled in the user specified strand.
 */
class SIRIKATA_EXPORT StrandTCPSocket : public TCPSocket {
  private:
    IOStrand* mStrand;

  public:
    /** Creates a new TCP socket which will handle events in the specified
     *  strand.
     *  \param strand the strand in which socket events will be handled
     */
    StrandTCPSocket(IOStrand* strand);


    typedef std::tr1::function<void(const boost::system::error_code&)> ConnectHandler;
    typedef std::tr1::function<void(const boost::system::error_code&, std::size_t)> ReadHandler;
    typedef std::tr1::function<void(const boost::system::error_code&, std::size_t)> WriteHandler;


    void async_connect(const endpoint_type& peer_endpoint, ConnectHandler handler);

    template<typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(const MutableBufferSequence & buffers, ReadHandler handler) {
        TCPSocket::async_read_some<MutableBufferSequence, ReadHandler>(buffers, mStrand->wrap_any(handler));
    }

    template<typename MutableBufferSequence, typename ReadHandler>
    void async_receive(const MutableBufferSequence & buffers, ReadHandler handler) {
        TCPSocket::async_receive<MutableBufferSequence, ReadHandler>(buffers, mStrand->wrap_any(handler));
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_send(const ConstBufferSequence & buffers, WriteHandler handler) {
        TCPSocket::async_send<ConstBufferSequence, WriteHandler>(buffers, mStrand->wrap_any(handler));
    }

    template<typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(const ConstBufferSequence & buffers, WriteHandler handler) {
        TCPSocket::async_write_some<ConstBufferSequence, WriteHandler>(buffers, mStrand->wrap_any(handler));
    }
};


} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_ASIO_STRAND_IO_HPP_
