/*  Sirikata Network Utilities
 *  ASIOSocketWrapper.cpp
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
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"

namespace Sirikata { namespace Network {
const uint32 ASIOSocketWrapper::ASYNCHRONOUS_SEND_FLAG;
const uint32 ASIOSocketWrapper::QUEUE_CHECK_FLAG;
const size_t ASIOSocketWrapper::PACKET_BUFFER_SIZE;
void copyHeader(void * destination, const UUID&key, unsigned int num) {
    std::memcpy(destination,TCPStream::STRING_PREFIX(),TCPStream::STRING_PREFIX_LENGTH);
    ((char*)destination)[TCPStream::STRING_PREFIX_LENGTH]='0'+(num/10)%10;
    ((char*)destination)[TCPStream::STRING_PREFIX_LENGTH+1]='0'+(num%10);
    std::memcpy(((char*)destination)+TCPStream::STRING_PREFIX_LENGTH+2,
                key.getArray().begin(),
                UUID::static_size);
}

void ASIOSocketWrapper::shutdownAndClose() {
    try {
        mSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    }catch (boost::system::system_error&err) {
        std::cerr<< "Error shutting down socket "<<err.what()<<std::endl;
    }
    try {
        mSocket->close();
    }catch (boost::system::system_error&err) {                
        std::cerr<< "Error closing socket "<<err.what()<<std::endl;
    }
}

void ASIOSocketWrapper::createSocket(IOService&io) {
    mSocket=new TCPSocket(io);
}

void ASIOSocketWrapper::destroySocket() {
    delete mSocket;
    mSocket=NULL;
}


void ASIOSocketWrapper::rawSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk * chunk) {
#ifdef TCPSSTLOG
    FILE * fp=fopen("RAWSENT","a");
    fputc('S',fp);        fputc('e',fp);        fputc('n',fp);        fputc('d',fp);
    fputc(':',fp);
    for (size_t i=0;i<chunk->size();++i) {
        fputc((*chunk)[i],fp);
    }
    fputc('\n',fp);
    fclose(fp);
#endif
    uint32 current_status=++mSendingStatus;
    //if someone else is currently sending a packet
    if (current_status&ASYNCHRONOUS_SEND_FLAG) {
        //push the packet on the queue
        mSendQueue.push(chunk);
        current_status=--mSendingStatus;
        //may have missed the send
        retryQueuedSend(parentMultiSocket,current_status);
    }else if (current_status==1) {
        mSendingStatus+=(ASYNCHRONOUS_SEND_FLAG-1);//committed to be the sender thread
        sendToWire(parentMultiSocket, chunk);
    }
}
Chunk*ASIOSocketWrapper::constructControlPacket(TCPStream::TCPStreamControlCodes code,const Stream::StreamID&sid){
    const unsigned int max_size=16;
    uint8 dataStream[max_size+2*Stream::uint30::MAX_SERIALIZED_LENGTH];
    unsigned int size=max_size;
    Stream::StreamID controlStream;//control packet
    size=controlStream.serialize(&dataStream[Stream::uint30::MAX_SERIALIZED_LENGTH],size);
    assert(size<max_size);
    dataStream[Stream::uint30::MAX_SERIALIZED_LENGTH+size++]=code;
    unsigned int cur=Stream::uint30::MAX_SERIALIZED_LENGTH+size;
    size=max_size-cur;
    size=sid.serialize(&dataStream[cur],size);
    assert(size+cur<=max_size);   
    Stream::uint30 streamSize=Stream::uint30(size+cur-Stream::uint30::MAX_SERIALIZED_LENGTH);
    unsigned int actualHeaderLength=streamSize.serialize(dataStream,Stream::uint30::MAX_SERIALIZED_LENGTH);
    if (actualHeaderLength!=Stream::uint30::MAX_SERIALIZED_LENGTH) {
        unsigned int retval=streamSize.serialize(dataStream+Stream::uint30::MAX_SERIALIZED_LENGTH-actualHeaderLength,Stream::uint30::MAX_SERIALIZED_LENGTH);
        assert(retval==actualHeaderLength);
    }
    return new Chunk(dataStream+Stream::uint30::MAX_SERIALIZED_LENGTH-actualHeaderLength,dataStream+size+cur);
}

void ASIOSocketWrapper::sendProtocolHeader(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const UUID&value, unsigned int numConnections) {
    UUID return_value=UUID::random();
    
    Chunk *headerData=new Chunk(TCPStream::TcpSstHeaderSize);
    copyHeader(&*headerData->begin(),value,numConnections);
    rawSend(parentMultiSocket,headerData);
}

} }
