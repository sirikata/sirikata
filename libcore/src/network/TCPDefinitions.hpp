/*  Sirikata Network Utilities
 *  TCPDefinitions.hpp
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

#ifndef _TCPDefinitions_HPP_
#define _TCPDefinitions_HPP_
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/system/system_error.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread.hpp>
namespace Sirikata { namespace Network {

typedef boost::asio::io_service InternalIOService;
typedef boost::asio::ip::tcp::socket InternalTCPSocket;
typedef  boost::asio::ip::tcp::acceptor InternalTCPAcceptor;
class IOServiceFactory;

class SIRIKATA_EXPORT IOService:public InternalIOService {
    friend class IOServiceFactory;
    IOService();
    ~IOService();
public:
};
class SIRIKATA_EXPORT TCPSocket: public InternalTCPSocket {
  public:
    TCPSocket(IOService&io);
};

class SIRIKATA_EXPORT TCPListener :public InternalTCPAcceptor {
public:
    TCPListener(IOService&io, const boost::asio::ip::tcp::endpoint&);
    void async_accept(TCPSocket&socket,
                      const std::tr1::function<void(const boost::system::error_code& ) > &cb);
};
#define TCPSSTLOG(thisname,extension,buffer,buffersize,error)
// #define TCPSSTLOG(thisname,extension,buffer,buffersize,error)  if (!error) {Sirikata::Network::ASIOLogBuffer(thisname,extension,(buffersize)?(buffer):NULL,buffersize);}

} }
#endif
