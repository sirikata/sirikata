/*  Sirikata Network Utilities
 *  TCPStreamListener.cpp
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

#include "util/Standard.hh"
#include "TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "TCPStreamListener.hpp"
#include "ASIOStreamBuilder.hpp"
#include "options/Options.hpp"
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
		SILOG(tcpsst,error, "ERROR IN THE TCP STREAM ACCEPTING PROCESS"<<se.what() << std::endl);
        //FIXME: attempt more?
    }else {
        ASIOStreamBuilder::beginNewStream(socket,io,cb);
        newAcceptPhase(listen,io,cb);
    }
}
bool newAcceptPhase(TCPListener*listen, IOService* io, const Stream::SubstreamCallback &cb) {
    TCPSocket*socket=new TCPSocket(*io);
    //need to use boost bind to avoid TR1 errors about compatibility with boost::asio::placeholders
     
    listen->async_accept(*socket,
                         std::tr1::bind(&handleAccept,socket,listen,io,cb,_1));
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
