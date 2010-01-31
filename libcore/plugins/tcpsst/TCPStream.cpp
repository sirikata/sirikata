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

#include "util/Platform.hpp"
#include "util/AtomicTypes.hpp"

#include "network/Asio.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "TCPSetCallbacks.hpp"
#include "network/IOServiceFactory.hpp"
#include "network/IOService.hpp"
#include "options/Options.hpp"
#include "VariableLength.hpp"
#include <boost/thread.hpp>
namespace Sirikata { namespace Network {

using namespace boost::asio::ip;
TCPStream::TCPStream(const MultiplexedSocketPtr&shared_socket,const Stream::StreamID&sid):mSocket(shared_socket),mID(sid),mSendStatus(new AtomicValue<int>(0)) {
    mNumSimultaneousSockets=shared_socket->numSockets();
    assert(mNumSimultaneousSockets);
    mSendBufferSize=shared_socket->getASIOSocketWrapper(0).getResourceMonitor().maxSize();
    boost::asio::socket_base::receive_buffer_size option;
    shared_socket->getASIOSocketWrapper(0).getSocket().get_option(option);
    mKernelReceiveBufferSize=option.value();
    boost::asio::socket_base::send_buffer_size optionS;
    shared_socket->getASIOSocketWrapper(0).getSocket().get_option(optionS);
    mKernelSendBufferSize=optionS.value();
    boost::asio::ip::tcp::no_delay optionND;
    shared_socket->getASIOSocketWrapper(0).getSocket().get_option(optionND);
    mNoDelay=optionND.value();
    mZeroDelim=shared_socket->isZeroDelim();
}

Duration TCPStream::averageSendLatency() const {
    return mSocket->averageSendLatency();
}

Duration TCPStream::averageReceiveLatency() const {
    return mSocket->averageReceiveLatency();
}

void TCPStream::readyRead() {
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        SILOG(tcpsst,debug,"Called TCPStream::readyRead() on closed stream." << getID().read());
        return;
    }

    MultiplexedSocketWPtr mpsocket(socket_copy);
    socket_copy->getASIOService().post(
                               std::tr1::bind(&MultiplexedSocket::ioReactorThreadResumeRead,
                                              mpsocket,
                                              mID));
}

void TCPStream::requestReadySendCallback() {
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        SILOG(tcpsst,debug,"Called TCPStream::pauseSend() on closed stream." << getID().read());
        return;
    }

    MultiplexedSocketWPtr mpsocket(socket_copy);
    socket_copy->getASIOService().post(
                               std::tr1::bind(&MultiplexedSocket::ioReactorThreadPauseSend,
                                              mpsocket,
                                              mID));
}
bool TCPStream::canSend(size_t dataSize)const {
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        SILOG(tcpsst,debug,"Called TCPStream::canSend() on closed stream." << getID().read());
        return false;
    }

    uint8 serializedStreamId[StreamID::MAX_SERIALIZED_LENGTH];
    unsigned int streamIdLength=StreamID::MAX_SERIALIZED_LENGTH;
    unsigned int successLengthNeeded=mID.serialize(serializedStreamId,streamIdLength);
    size_t totalSize=dataSize+successLengthNeeded;
    VariableLength packetLength=VariableLength(totalSize);
    uint8 packetLengthSerialized[VariableLength::MAX_SERIALIZED_LENGTH];
    unsigned int packetHeaderLength=packetLength.serialize(packetLengthSerialized,VariableLength::MAX_SERIALIZED_LENGTH);
    totalSize+=packetHeaderLength;
    return socket_copy->canSendBytes(mID,totalSize);
}
bool TCPStream::send(const Chunk&data, StreamReliability reliability) {
    return send(MemoryReference(data),reliability);
}
bool TCPStream::send(MemoryReference firstChunk, StreamReliability reliability) {
    return send(firstChunk,MemoryReference::null(),reliability);
}
bool TCPStream::send(MemoryReference firstChunk, MemoryReference secondChunk, StreamReliability reliability) {
    MultiplexedSocket::RawRequest toBeSent;
    // only allow 3 of the four possibilities because unreliable ordered is tricky and usually useless
    switch(reliability) {
      case Unreliable:
        toBeSent.unordered=true;
        toBeSent.unreliable=true;
        break;
      case ReliableOrdered:
        toBeSent.unordered=false;
        toBeSent.unreliable=false;
        break;
      case ReliableUnordered:
        toBeSent.unordered=true;
        toBeSent.unreliable=false;
        break;
    }
    toBeSent.originStream=getID();
    ///this function should never return something larger than the  MAX_SERIALIZED_LEGNTH
    if (mZeroDelim) {
        uint8 serializedStreamId[StreamID::MAX_HEX_SERIALIZED_LENGTH];
        unsigned int streamIdLength=StreamID::MAX_HEX_SERIALIZED_LENGTH;
        unsigned int successLengthNeeded=toBeSent.originStream.serializeToHex(serializedStreamId,streamIdLength);
        assert(successLengthNeeded<=streamIdLength);


        MemoryReference streamIdBytes(serializedStreamId,successLengthNeeded);
        toBeSent.data = ASIOSocketWrapper::toBase64ZeroDelim(firstChunk,
                                                             secondChunk,
                                                             MemoryReference(NULL,0),
                                                             &streamIdBytes);
    }else {
        uint8 serializedStreamId[StreamID::MAX_SERIALIZED_LENGTH];
        unsigned int streamIdLength=StreamID::MAX_SERIALIZED_LENGTH;
        unsigned int successLengthNeeded=toBeSent.originStream.serialize(serializedStreamId,streamIdLength);
        assert(successLengthNeeded<=streamIdLength);
        streamIdLength=successLengthNeeded;
        size_t totalSize=firstChunk.size()+secondChunk.size();
        totalSize+=streamIdLength;
        VariableLength packetLength=VariableLength(totalSize);
        uint8 packetLengthSerialized[VariableLength::MAX_SERIALIZED_LENGTH];
        unsigned int packetHeaderLength=packetLength.serialize(packetLengthSerialized,VariableLength::MAX_SERIALIZED_LENGTH);
        //allocate a packet long enough to take both the length of the packet and the stream id as well as the packet data. totalSize = size of streamID + size of data and
        //packetHeaderLength = the length of the length component of the packet
        toBeSent.data=new Chunk(totalSize+packetHeaderLength);
        
        uint8 *outputBuffer=&(*toBeSent.data)[0];
        std::memcpy(outputBuffer,packetLengthSerialized,packetHeaderLength);
        std::memcpy(outputBuffer+packetHeaderLength,serializedStreamId,streamIdLength);
        if (firstChunk.size()) {
            std::memcpy(&outputBuffer[packetHeaderLength+streamIdLength],
                        firstChunk.data(),
                        firstChunk.size());
        }
        if (secondChunk.size()) {
            std::memcpy(&outputBuffer[packetHeaderLength+streamIdLength+firstChunk.size()],
                        secondChunk.data(),
                        secondChunk.size());
        }
    }
    bool didsend=false;
    //indicate to other would-be TCPStream::close()ers that we are sending and they will have to wait until we give up control to actually ack the close and shut down the stream
    unsigned int sendStatus=++(*mSendStatus);
    if ((sendStatus&(3*SendStatusClosing))==0) {///max of 3 entities can close the stream at once (FIXME: should implement |= on atomic ints), but as of now at most the recv thread the sender responsible and a user close() is all that is allowed at once...so 3 is fine)
        MultiplexedSocketPtr socket_copy = mSocket;
        if (socket_copy.get() == NULL)
            didsend = false;
        else
            didsend=MultiplexedSocket::sendBytes(socket_copy,toBeSent,mSendBufferSize);
    }
    //relinquish control to a potential closer
    --(*mSendStatus);
    if (!didsend) {
        //if the data was not sent, its our job to clean it up
        delete toBeSent.data;
        if ((mSendStatus->read()&(3*SendStatusClosing))!=0) {///max of 3 entities can close the stream at once (FIXME: should implement |= on atomic ints), but as of now at most the recv thread the sender responsible and a user close() is all that is allowed at once...so 3 is fine)
            SILOG(tcpsst,debug,"printing to closed stream id "<<getID().read());
        }
    }
    return didsend;
}
///This function waits on the sendStatus clearing up so no outstanding sends are being made (and no further ones WILL be made cus of the SendStatusClosing flag that is on
bool TCPStream::closeSendStatus(AtomicValue<int>&vSendStatus) {
    int sendStatus=vSendStatus.read();
    bool incd=false;
    if ((sendStatus&(SendStatusClosing*3))==0) {
        ///FIXME we want to |= here
        incd=((vSendStatus+=SendStatusClosing)==SendStatusClosing);
    }
    //Wait until it's a pure sendStatus value without any 'remainders' caused by outstanding sends
    while ((sendStatus=vSendStatus.read())!=SendStatusClosing&&
           sendStatus!=2*SendStatusClosing&&
           sendStatus!=3*SendStatusClosing) {
    }
    return incd;
}
void TCPStream::close() {
    // For thread safety, make a copy so we are ensured it is valid throughout this method
    MultiplexedSocketPtr socket_copy(mSocket);

    // If its already been closed, there's nothing to do
    if (socket_copy.get() == NULL)
        return;

    //Otherwise, set the stream closed as soon as sends are done
    bool justClosed=closeSendStatus(*mSendStatus);
    if (justClosed) {
        //obliterate all incoming callback to this stream
        socket_copy->addCallbacks(getID(),NULL);
        //send out that the stream is now closed on all sockets
        MultiplexedSocket::closeStream(socket_copy,getID());
    }

    // And get rid of the reference to the socket so it can (maybe) be cleaned up
    mSocket.reset();
}
TCPStream::~TCPStream() {
    close();
}
TCPStream::TCPStream(IOService&io,OptionSet*options):mSendStatus(new AtomicValue<int>(0)) {
    mIO=&io;
    OptionValue *numSimultSockets=options->referenceOption("parallel-sockets");
    OptionValue *sendBufferSize=options->referenceOption("send-buffer-size");
    OptionValue *kernelSendBufferSize=options->referenceOption("ksend-buffer-size");
    OptionValue *kernelReceiveBufferSize=options->referenceOption("kreceive-buffer-size");
    OptionValue *noDelay=options->referenceOption("no-delay");
    OptionValue *zeroDelim=options->referenceOption("base64");
    assert(numSimultSockets&&sendBufferSize);
    mNumSimultaneousSockets=(unsigned char)numSimultSockets->as<unsigned int>();
    assert(mNumSimultaneousSockets);
    mSendBufferSize=sendBufferSize->as<unsigned int>();
    mKernelSendBufferSize=kernelSendBufferSize->as<unsigned int>();
    mKernelReceiveBufferSize=kernelReceiveBufferSize->as<unsigned int>();
    mNoDelay=noDelay->as<bool>();
    mZeroDelim=zeroDelim->as<bool>();
}

TCPStream::TCPStream(IOService&io,unsigned char numSimultSockets,unsigned int sendBufferSize,bool noDelay, bool zeroDelim, unsigned int kernelSendBufferSize,unsigned int kernelReceiveBufferSize):mSendStatus(new AtomicValue<int>(0)) {
    mIO=&io;
    mNumSimultaneousSockets=(unsigned char)numSimultSockets;
    assert(mNumSimultaneousSockets);
    mSendBufferSize=sendBufferSize;
    mNoDelay=noDelay;
    mKernelSendBufferSize=kernelSendBufferSize;
    mKernelReceiveBufferSize=kernelReceiveBufferSize;
    mZeroDelim=zeroDelim;
}

void TCPStream::connect(const Address&addy,
                        const SubstreamCallback &substreamCallback,
                        const ConnectionCallback &connectionCallback,
                        const ReceivedCallback&bytesReceivedCallback,
                        const ReadySendCallback&readySendCallback) {
    MultiplexedSocketPtr socket = MultiplexedSocket::construct<MultiplexedSocket>(mIO,substreamCallback,mZeroDelim);
    mSocket = socket;
    *mSendStatus=0;
    mID=StreamID(1);
    socket->addCallbacks(getID(),new Callbacks(connectionCallback,
                                                bytesReceivedCallback,
                                                readySendCallback,
                                                mSendStatus));
    socket->connect(addy,mNumSimultaneousSockets,mSendBufferSize,mNoDelay,mKernelSendBufferSize,mKernelReceiveBufferSize);
}

Stream*TCPStream::factory(){
    return new TCPStream(*mIO,mNumSimultaneousSockets,mSendBufferSize,mNoDelay,mZeroDelim,mKernelSendBufferSize,mKernelReceiveBufferSize);
}
Stream* TCPStream::clone(const SubstreamCallback &cloneCallback) {
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        return NULL;
    }

    TCPStream *retval=new TCPStream(*mIO,mNumSimultaneousSockets,mSendBufferSize,mNoDelay,mZeroDelim, mKernelSendBufferSize,mKernelReceiveBufferSize);
    retval->mSocket = socket_copy;

    StreamID newID = socket_copy->getNewID();
    retval->mID=newID;
    TCPSetCallbacks setCallbackFunctor(socket_copy.get(), retval);
    cloneCallback(retval,setCallbackFunctor);
    return retval;
}

Stream* TCPStream::clone(const ConnectionCallback &connectionCallback,
                         const ReceivedCallback&chunkReceivedCallback,
                         const ReadySendCallback&readySendCallback) {
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        return NULL;
    }

    TCPStream *retval=new TCPStream(*mIO,mNumSimultaneousSockets,mSendBufferSize,mNoDelay,mZeroDelim, mKernelSendBufferSize,mKernelReceiveBufferSize);
    retval->mSocket = socket_copy;

    StreamID newID = socket_copy->getNewID();
    retval->mID=newID;
    TCPSetCallbacks setCallbackFunctor(socket_copy.get(), retval);
    setCallbackFunctor(connectionCallback,chunkReceivedCallback, readySendCallback);
    return retval;
}


///Only returns a legitimate address if ConnectionStatus called back, otherwise return Address::null()
Address TCPStream::getRemoteEndpoint() const{
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        SILOG(tcpsst,debug,"Called TCPStream::getRemoveEndpoint() on closed stream." << getID().read());
        return Address::null();
    }

    return socket_copy->getRemoteEndpoint(mID);
}
///Only returns a legitimate address if ConnectionStatus called back, otherwise return Address::null()
Address TCPStream::getLocalEndpoint() const{
    MultiplexedSocketPtr socket_copy = mSocket;
    if (socket_copy.get() == NULL) {
        SILOG(tcpsst,debug,"Called TCPStream::getLocalEndpoint() on closed stream." << getID().read());
        return Address::null();
    }

    return socket_copy->getLocalEndpoint(mID);
}

}  }
