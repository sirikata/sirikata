/*  Sirikata Network Utilities
 *  ASIOStreamBuilder.cpp
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

#include "util/Platform.hpp"
#include "network/Asio.hpp"
#include "TCPStream.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "TCPSetCallbacks.hpp"

namespace Sirikata {
namespace Network {
namespace ASIOStreamBuilder{

typedef Array<uint8,TCPStream::TcpSstHeaderSize> TcpSstHeaderArray;

class IncompleteStreamState {
public:
    int mNumSockets;
    std::vector<TCPSocket*>mSockets;
};

namespace {
typedef std::map<UUID,IncompleteStreamState> IncompleteStreamMap;
std::deque<UUID> sStaleUUIDs;
IncompleteStreamMap sIncompleteStreams;
}

///gets called when a complete 24 byte header is actually received: uses the UUID within to match up appropriate sockets
void buildStream(TcpSstHeaderArray *buffer,
                 TCPSocket *socket,
                 IOService *ioService,
                 Stream::SubstreamCallback callback,
                 const boost::system::error_code &error,
                 std::size_t bytes_transferred) {
    if (error || std::memcmp(buffer->begin(),TCPStream::STRING_PREFIX(),TCPStream::STRING_PREFIX_LENGTH)!=0) {
        SILOG(tcpsst,warning,"Connection received with incomprehensible header");
    }else {
        UUID context=UUID(buffer->begin()+(TCPStream::TcpSstHeaderSize-16),16);
        IncompleteStreamMap::iterator where=sIncompleteStreams.find(context);
        unsigned int numConnections=(((*buffer)[TCPStream::STRING_PREFIX_LENGTH]-'0')%10)*10+(((*buffer)[TCPStream::STRING_PREFIX_LENGTH+1]-'0')%10);
        if (numConnections>99) numConnections=99;//FIXME: some option in options
        if (where==sIncompleteStreams.end()){
            sIncompleteStreams[context].mNumSockets=numConnections;
            where=sIncompleteStreams.find(context);
            assert(where!=sIncompleteStreams.end());
        }
        if ((int)numConnections!=where->second.mNumSockets) {
            SILOG(tcpsst,warning,"Single client disagrees on number of connections to establish: "<<numConnections<<" != "<<where->second.mNumSockets);
            sIncompleteStreams.erase(where);
        }else {
            where->second.mSockets.push_back(socket);
            if (numConnections==(unsigned int)where->second.mSockets.size()) {
                MultiplexedSocketPtr shared_socket(
                    MultiplexedSocket::construct<MultiplexedSocket>(ioService,context,where->second.mSockets,callback));
                MultiplexedSocket::sendAllProtocolHeaders(shared_socket,UUID::random());
                sIncompleteStreams.erase(where);
                Stream::StreamID newID=Stream::StreamID(1);
                TCPStream * strm=new TCPStream(shared_socket,newID);

                TCPSetCallbacks setCallbackFunctor(&*shared_socket,strm);
                callback(strm,setCallbackFunctor);
                if (setCallbackFunctor.mCallbacks==NULL) {
                    SILOG(tcpsst,error,"Client code for stream "<<newID.read()<<" did not set listener on socket");
                    shared_socket->closeStream(shared_socket,newID);
                }
            }else{
                sStaleUUIDs.push_back(context);
            }
        }
    }
    delete buffer;
}

void beginNewStream(TCPSocket* socket, IOService* ioService, const Stream::SubstreamCallback& cb) {
    TcpSstHeaderArray *buffer = new TcpSstHeaderArray;

    boost::asio::async_read(*socket,
                            boost::asio::buffer(buffer->begin(),TCPStream::TcpSstHeaderSize),
                            boost::asio::transfer_at_least(TCPStream::TcpSstHeaderSize),
                            std::tr1::bind(&ASIOStreamBuilder::buildStream,buffer,socket,ioService,cb,_1,_2));
}

} // namespace ASIOStreamBuilder
} // namespace Network
} // namespace Sirikata
