#include "util/Standard.hh"
#include "TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "TCPStreamListener.hpp"

namespace Sirikata { namespace Network {
using namespace boost::asio::ip;

TCPStreamListener::TCPStreamListener(IOService&io) {
    mIOService=&io;
    mTCPAcceptor=NULL;
}
bool newAcceptPhase(TCPListener*listen, IOService* io,const Stream::SubstreamCallback &cb);
void handleAccept(TCPSocket*socket,TCPListener*listen, IOService* io,const Stream::SubstreamCallback &cb,const boost::system::error_code& error){
    if(error) {
		boost::system::system_error se(error);
		std::cerr << se.what() << std::endl;
        assert(0&&"ERROR IN THE TCP STREAM ACCEPTING PROCESS");
        //FIXME: attempt more?
    }else {
        beginNewStream(socket,cb);
        newAcceptPhase(listen,io,cb);
    }
}
bool newAcceptPhase(TCPListener*listen, IOService* io, const Stream::SubstreamCallback &cb) {
    TCPSocket*socket=new TCPSocket(*io);
    //need to use boost bind to avoid TR1 errors about compatibility with boost::asio::placeholders
    listen->async_accept(*socket,
                         boost::bind(&handleAccept,socket,listen,io,cb,boost::asio::placeholders::error));
    return true;
}
bool TCPStreamListener::listen (const Address&address,
                                const Stream::SubstreamCallback&newStreamCallback) {

    mTCPAcceptor = new TCPListener(*mIOService,tcp::endpoint(tcp::v4(), atoi(address.getService().c_str())));
    return newAcceptPhase(mTCPAcceptor,mIOService,newStreamCallback);
}
TCPStreamListener::~TCPStreamListener() {
    delete mTCPAcceptor;
}
String TCPStreamListener::listenAddressName() const {
    std::stringstream retval;
    retval<<mTCPAcceptor->local_endpoint().address().to_string()<<':'<<mTCPAcceptor->local_endpoint().port();
    return retval.str();
}

Address TCPStreamListener::listenAddress() const {
    std::stringstream port;
    port << mTCPAcceptor->local_endpoint().port();
    return Address(mTCPAcceptor->local_endpoint().address().to_string(),
                   port.str());
}
void TCPStreamListener::close(){
    delete mTCPAcceptor;
    mTCPAcceptor=NULL;
}

} }
