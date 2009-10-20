#include "util/Standard.hh"
#include "Asio.hpp"
#include "IOService.hpp"

namespace Sirikata {
namespace Network {


InternalIOStrand::InternalIOStrand(IOService &io)
 : boost::asio::io_service::strand(io.asioService())
{
}


TCPSocket::TCPSocket(IOService&io):
    boost::asio::ip::tcp::socket(io.asioService())
{
}


TCPListener::TCPListener(IOService&io, const boost::asio::ip::tcp::endpoint&ep):
    boost::asio::ip::tcp::acceptor(io.asioService(),ep)
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


UDPSocket::UDPSocket(IOService&io):
    boost::asio::ip::udp::socket(io.asioService())
{
}

UDPResolver::UDPResolver(IOService&io)
    : boost::asio::ip::udp::resolver(io.asioService())
{
}

DeadlineTimer::DeadlineTimer(IOService& io)
    : boost::asio::deadline_timer(io.asioService()) {
}


} // namespace Network
} // namespace Sirikata
