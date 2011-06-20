/*  Sirikata
 *  Asio.cpp
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

#include <sirikata/core/util/Standard.hh>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/network/IOService.hpp>

namespace Sirikata {
namespace Network {

InternalIOWork::InternalIOWork(IOService& serv, const String& name)
 : InternalIOService::work(serv.asioService()),
   mName(name)
{
    logEvent("created");
}

InternalIOWork::InternalIOWork(IOService* serv, const String& name)
 : InternalIOService::work(serv->asioService()),
   mName(name)
{
    logEvent("created");
}

InternalIOWork::~InternalIOWork() {
    logEvent("destroyed");
}

void InternalIOWork::logEvent(const String& evt) {
    if (mName != "")
        SILOG(io,insane,"IOWork event: " << mName << " -> " << evt);
}



InternalIOStrand::InternalIOStrand(IOService &io)
 : boost::asio::io_service::strand(io.asioService())
{
}

InternalIOStrand::InternalIOStrand(IOService* io)
 : boost::asio::io_service::strand(io->asioService())
{
}


TCPSocket::TCPSocket(IOService&io):
    boost::asio::ip::tcp::socket(io.asioService())
{
}

TCPSocket::TCPSocket(IOService* io):
    boost::asio::ip::tcp::socket(io->asioService())
{
}

TCPSocket::~TCPSocket()
{
}


TCPListener::TCPListener(IOService&io, const boost::asio::ip::tcp::endpoint&ep):
    boost::asio::ip::tcp::acceptor(io.asioService(),ep)
{
}

TCPListener::TCPListener(IOService* io, const boost::asio::ip::tcp::endpoint&ep):
    boost::asio::ip::tcp::acceptor(io->asioService(),ep)
{
}

TCPListener::~TCPListener()
{
}

void TCPListener::async_accept(TCPSocket&socket,
                               const std::tr1::function<void(const boost::system::error_code&)>&cb) {
    this->InternalTCPAcceptor::async_accept(socket,cb);
}


TCPResolver::TCPResolver(IOService&io)
    : boost::asio::ip::tcp::resolver(io.asioService())
{
}

TCPResolver::TCPResolver(IOService* io)
    : boost::asio::ip::tcp::resolver(io->asioService())
{
}

TCPResolver::~TCPResolver()
{
}


UDPSocket::UDPSocket(IOService&io):
    boost::asio::ip::udp::socket(io.asioService())
{
}

UDPSocket::UDPSocket(IOService*io):
    boost::asio::ip::udp::socket(io->asioService())
{
}

UDPSocket::~UDPSocket()
{
}

UDPResolver::UDPResolver(IOService&io)
    : boost::asio::ip::udp::resolver(io.asioService())
{
}

UDPResolver::UDPResolver(IOService* io)
    : boost::asio::ip::udp::resolver(io->asioService())
{
}


UDPResolver::~UDPResolver()
{
}

DeadlineTimer::DeadlineTimer(IOService& io)
    : boost::asio::deadline_timer(io.asioService()) {

}

DeadlineTimer::DeadlineTimer(IOService* io)
    : boost::asio::deadline_timer(io->asioService()) {

}
DeadlineTimer::~DeadlineTimer () {

}

} // namespace Network
} // namespace Sirikata
