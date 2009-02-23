/*  Sirikata Network Utilities
 *  TCPStream.cpp
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
#include "util/AtomicTypes.hpp"

#include "TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "TCPSetCallbacks.hpp"
#include <boost/thread.hpp>
namespace Sirikata { namespace Network {
const int TCPStream::SendStatusClosing=(1<<30);

using namespace boost::asio::ip;






TCPStream::TCPStream(const std::tr1::shared_ptr<MultiplexedSocket>&shared_socket,const Stream::StreamID&sid):mSocket(shared_socket),mID(sid),mSendStatus(0) {

}
namespace TCPStreamBuilder{
    class IncompleteStreamState {
    public:
        int mNumSockets;
        std::vector<TCPSocket*>mSockets;
    };
    typedef std::map<UUID,IncompleteStreamState> IncompleteStreamMap;
    std::deque<UUID> sStaleUUIDs;
    IncompleteStreamMap sIncompleteStreams;
    ///gets called when a complete 24 byte header is actually received: uses the UUID within to match up appropriate sockets
void buildStream(Array<uint8,TCPStream::TcpSstHeaderSize> *buffer,
                 TCPSocket *socket,
                 Stream::SubstreamCallback callback,
                 const boost::system::error_code &error,
                 std::size_t bytes_transferred) {
        if (error || std::memcmp(buffer->begin(),TCPStream::STRING_PREFIX(),TCPStream::STRING_PREFIX_LENGTH)!=0) {
            std::cerr<< "Connection received with incomprehensible header";
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
                std::cerr<< "Single client disagrees on number of connections to establish";
                sIncompleteStreams.erase(where);
            }else {
                where->second.mSockets.push_back(socket);
                if (numConnections==(unsigned int)where->second.mSockets.size()) {
                    std::tr1::shared_ptr<MultiplexedSocket> shared_socket(MultiplexedSocket::construct(context,where->second.mSockets,callback));
                    MultiplexedSocket::sendAllProtocolHeaders(shared_socket,UUID::random());
                    sIncompleteStreams.erase(where);
                    Stream::StreamID newID=Stream::StreamID(1);
                    TCPStream * strm=new TCPStream(shared_socket,newID);

                    TCPSetCallbacks setCallbackFunctor(&*shared_socket,strm);
                    callback(strm,setCallbackFunctor);
                    if (setCallbackFunctor.mCallbacks==NULL) {
                        std::cerr<<"Forgot to set listener on socket\n";
                        shared_socket->closeStream(shared_socket,newID);
                    }
                }else{
                    sStaleUUIDs.push_back(context);
                }
            }
        }
        delete buffer;
    }
};
void TCPStream::send(const Chunk&data, Stream::Reliability reliability) {
    MultiplexedSocket::RawRequest toBeSent;
    switch(reliability) {
      case Unreliable:
        toBeSent.unordered=true;
        toBeSent.unreliable=true;
        break;
      case Reliable:
        toBeSent.unordered=false;
        toBeSent.unreliable=false;
        break;
      case ReliableUnordered:
        toBeSent.unordered=true;
        toBeSent.unreliable=false;
        break;
    }
    toBeSent.originStream=getID();
    uint8 serializedStreamId[StreamID::MAX_SERIALIZED_LENGTH];//={255,255,255,255,255,255,255,255};
    unsigned int streamIdLength=StreamID::MAX_SERIALIZED_LENGTH;
    unsigned int successLengthNeeded=toBeSent.originStream.serialize(serializedStreamId,streamIdLength);
    assert(successLengthNeeded<=streamIdLength);
    streamIdLength=successLengthNeeded;
    size_t totalSize=data.size();
    totalSize+=streamIdLength;
    uint30 packetLength=uint30(totalSize);
    uint8 packetLengthSerialized[uint30::MAX_SERIALIZED_LENGTH];
    unsigned int packetHeaderLength=packetLength.serialize(packetLengthSerialized,uint30::MAX_SERIALIZED_LENGTH);
    toBeSent.data=new Chunk(totalSize+packetHeaderLength);
    
    uint8 *outputBuffer=&(*toBeSent.data)[0];
    std::memcpy(outputBuffer,packetLengthSerialized,packetHeaderLength);
    std::memcpy(outputBuffer+packetHeaderLength,serializedStreamId,streamIdLength);
    if (data.size())
        std::memcpy(&outputBuffer[packetHeaderLength+streamIdLength],
                    &data[0],
                    data.size());
    unsigned int sendStatus=++mSendStatus;
    if ((sendStatus&SendStatusClosing)==0) {
        MultiplexedSocket::sendBytes(mSocket,toBeSent);
    }
    --mSendStatus;
}
void TCPStream::close() {
    mSendStatus+=SendStatusClosing;
    while (mSendStatus.read()!=SendStatusClosing)
        ;
    mSocket->addCallbacks(getID(),NULL);
    MultiplexedSocket::closeStream(mSocket,getID());
}
TCPStream::TCPStream(IOService&io):mIO(&io),mSendStatus(0) {
}
void TCPStream::connect(const Address&addy,
                        const SubstreamCallback &substreamCallback,
                        const ConnectionCallback &connectionCallback,
                        const BytesReceivedCallback&bytesReceivedCallback) {
    mSocket=MultiplexedSocket::construct(mIO,substreamCallback);
    mSendStatus=0;
    mID=StreamID(1);
    mSocket->addCallbacks(getID(),new Callbacks(connectionCallback,
                                                bytesReceivedCallback));
    mSocket->connect(addy,3);
}
Stream* TCPStream::factory() {
    return new TCPStream(*mIO);
}
bool TCPStream::cloneFrom(Stream*otherStream,
                          const ConnectionCallback &connectionCallback,
                          const BytesReceivedCallback&bytesReceivedCallback) {
    TCPStream * toBeCloned=dynamic_cast<TCPStream*>(otherStream);
    if (NULL==toBeCloned)
        return false;
    mSocket=toBeCloned->mSocket;
    StreamID newID=mSocket->getNewID();
    mID=newID;
    mSocket->addCallbacks(newID,new Callbacks(connectionCallback,
                                              bytesReceivedCallback));
    return true;
}

void beginNewStream(TCPSocket * socket, const Stream::SubstreamCallback& cb) {
    Array<uint8,TCPStream::TcpSstHeaderSize> *buffer=new Array<uint8,TCPStream::TcpSstHeaderSize>;
    boost::asio::async_read(*socket,
                            boost::asio::buffer(buffer->begin(),TCPStream::TcpSstHeaderSize),
                            boost::asio::transfer_at_least(TCPStream::TcpSstHeaderSize),
                            std::tr1::bind(&TCPStreamBuilder::buildStream,buffer,socket,cb,_1,_2));
}

}  }
