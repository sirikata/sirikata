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


#include "util/Platform.hpp"
#include "network/TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"

namespace Sirikata { namespace Network {

void ASIOLogBuffer(void * pointerkey, const char extension[16], const uint8* buffer, size_t buffersize){
    char filename[1024];
    sprintf(filename,"%x.%s",(unsigned int)(intptr_t)pointerkey,extension);
    FILE*fp=fopen(filename,"ab");
    fwrite(buffer,buffersize,1,fp);
    fclose(fp);
    
}
void copyHeader(void * destination, const UUID&key, unsigned int num) {
    std::memcpy(destination,TCPStream::STRING_PREFIX(),TCPStream::STRING_PREFIX_LENGTH);
    ((char*)destination)[TCPStream::STRING_PREFIX_LENGTH]='0'+(num/10)%10;
    ((char*)destination)[TCPStream::STRING_PREFIX_LENGTH+1]='0'+(num%10);
    std::memcpy(((char*)destination)+TCPStream::STRING_PREFIX_LENGTH+2,
                key.getArray().begin(),
                UUID::static_size);
}

void ASIOSocketWrapper::finishAsyncSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket) {
    //When this function is called, the ASYNCHRONOUS_SEND_FLAG must be set because this particular context is the one finishing up a send
    assert(mSendingStatus.read()&ASYNCHRONOUS_SEND_FLAG);
    //Turn on the information that the queue is being checked and this means that further pushes to the queue may not be heeded if the queue happened to be empty
    mSendingStatus+=QUEUE_CHECK_FLAG;
    std::deque<Chunk*>toSend;
    mSendQueue.swap(toSend);
    std::size_t num_packets=toSend.size();
    if (num_packets==0) {
        //if there are no packets in the queue, some other send() operation will need to take the torch to send further packets
        mSendingStatus-=(ASYNCHRONOUS_SEND_FLAG+QUEUE_CHECK_FLAG);
    }else {
        //there are packets in the queue, now is the chance to send them out, so get rid of the queue check flag since further items *will* be checked from the queue as soon as the
        //send finishes
        mSendingStatus-=QUEUE_CHECK_FLAG;
        if (num_packets==1)
            sendToWire(parentMultiSocket,toSend.front());
        else
            sendToWire(parentMultiSocket,toSend);
    }
}
void ASIOSocketWrapper::sendLargeChunkItem(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk *toSend, size_t originalOffset, const ErrorCode &error, std::size_t bytes_sent) {
    TCPSSTLOG(this,"snd",&*toSend->begin()+originalOffset,bytes_sent,error);
    if (error)  {
        triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
        SILOG(tcpsst,insane,"Socket disconnected...waiting for recv to trigger error condition\n");
    }else if (bytes_sent+originalOffset!=toSend->size()) {
        sendToWire(parentMultiSocket,toSend,originalOffset+bytes_sent);
    }else {
        delete toSend;
        finishAsyncSend(parentMultiSocket);
    }
}

void ASIOSocketWrapper::sendLargeDequeItem(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*> &const_toSend, size_t originalOffset, const ErrorCode &error, std::size_t bytes_sent) {
    TCPSSTLOG(this,"snd",&*const_toSend.front()->begin()+originalOffset,bytes_sent,error);
    if (error )   {
        triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
        SILOG(tcpsst,insane,"Socket disconnected...waiting for recv to trigger error condition\n");
    } else if (bytes_sent+originalOffset!=const_toSend.front()->size()) {
        sendToWire(parentMultiSocket,const_toSend,originalOffset+bytes_sent);
    }else if (const_toSend.size()<2) {
        //the entire packet got sent and there's no more items left: delete the front item
        delete const_toSend.front();
        //and send further items on the global queue if they are there
        finishAsyncSend(parentMultiSocket);
    }else {
        std::deque<Chunk*> toSend=const_toSend;
        //the first item got sent out
        delete toSend.front();
        toSend.pop_front();
        if (toSend.size()==1) {
            //if there's just one item left, it may be sent by itself
            sendToWire(parentMultiSocket,toSend.front());
        }else {
            //otherwise send the whole queue
            sendToWire(parentMultiSocket,toSend);
        }
    }
}
#define ASIOSocketWrapperBuffer(pointer,size) boost::asio::buffer(pointer,(size))
void ASIOSocketWrapper::sendStaticBuffer(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*>&toSend, uint8* currentBuffer, size_t bufferSize, size_t lastChunkOffset,  const ErrorCode &error, std::size_t bytes_sent) {
    TCPSSTLOG(this,"snd",current_buffer,bytes_sent,error);
    if (!error) {
        //mPacketLogger.insert(mPacketLogger.end(),currentBuffer,currentBuffer+bytes_sent);
    }
    if ( error ) {
        triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
        SILOG(tcpsst,insane,"Socket disconnected...waiting for recv to trigger error condition\n");
    }else if (bytes_sent!=bufferSize) {
		 
		 
        //if the previous send was not able to push the whole buffer out to the network, the rest must be sent
        mSocket->async_send(ASIOSocketWrapperBuffer(currentBuffer+bytes_sent,bufferSize-bytes_sent),
                            std::tr1::bind(&ASIOSocketWrapper::sendStaticBuffer,
                                        this,
                                        parentMultiSocket,
                                        toSend,
                                        currentBuffer+bytes_sent,
                                        bufferSize-bytes_sent,
                                        lastChunkOffset,
                                          _1,
                                          _2));        
    }else {
        if (!toSend.empty()) {
            //if stray items are left in a deque
            if (toSend.size()==1) {
                //if there's one item send that
                sendToWire(parentMultiSocket,toSend.front(),lastChunkOffset);
            }else {
                //otherwise send the whole lot
                sendToWire(parentMultiSocket,toSend,lastChunkOffset);
            }
        }else {
            //no items to send, check the new queue for any more and send those, otherwise sleep
            finishAsyncSend(parentMultiSocket);
        }
    }
}
    

void ASIOSocketWrapper::sendToWire(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk *toSend, size_t bytesSent) {
    //sending a single chunk is a straightforward call directly to asio
     
     
    mSocket->async_send(ASIOSocketWrapperBuffer(&*toSend->begin()+bytesSent,toSend->size()-bytesSent),
                        std::tr1::bind(&ASIOSocketWrapper::sendLargeChunkItem,
                                    this,
                                    parentMultiSocket,
                                    toSend,
                                    bytesSent,
                                          _1,
                                          _2));
}

void ASIOSocketWrapper::sendToWire(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*>&const_toSend, size_t bytesSent){
     
     
    if (const_toSend.front()->size()-bytesSent>PACKET_BUFFER_SIZE||const_toSend.size()==1) {
        //if there's but a single packet, or a single big packet that is bigger than the mBuffer's size...send that one by itself 
        mSocket->async_send(ASIOSocketWrapperBuffer(&*const_toSend.front()->begin()+bytesSent,const_toSend.front()->size()-bytesSent),
                            std::tr1::bind(&ASIOSocketWrapper::sendLargeDequeItem,
                                        this,
                                        parentMultiSocket,
                                        const_toSend,
                                        bytesSent,
                                          _1,
                                          _2));
    }else if (const_toSend.front()->size()){
        //otherwise copy the packets onto the mBuffer and send from the fixed sized buffer
        std::deque<Chunk*> toSend=const_toSend;
        size_t bufferLocation=toSend.front()->size()-bytesSent;
        std::memcpy(mBuffer,&*toSend.front()->begin()+bytesSent,toSend.front()->size()-bytesSent);
        delete toSend.front();
        toSend.pop_front();
        bytesSent=0;
        while (bufferLocation<PACKET_BUFFER_SIZE&&toSend.size()) {            
            if (toSend.front()->size()>PACKET_BUFFER_SIZE-bufferLocation) {
                //if the first packet is too large for the buffer, copy part of it but keep it around
                bytesSent=PACKET_BUFFER_SIZE-bufferLocation;
                std::memcpy(mBuffer+bufferLocation,&*toSend.front()->begin(),bytesSent);
                bufferLocation=PACKET_BUFFER_SIZE;
            }else {
                //if the entire packets fits in the buffer, copy it there and delete the packet
                std::memcpy(mBuffer+bufferLocation,&*toSend.front()->begin(),toSend.front()->size());
                bufferLocation+=toSend.front()->size();
                delete toSend.front();
                toSend.pop_front();
            }
        }
        //send the buffer filled with possibly many packets
        mSocket->async_send(ASIOSocketWrapperBuffer(mBuffer,bufferLocation),
                            std::tr1::bind(&ASIOSocketWrapper::sendStaticBuffer,
                                          this,
                                          parentMultiSocket,
                                          toSend,
                                          mBuffer,
                                          bufferLocation,
                                          bytesSent,
                                          _1,
                                          _2));
    }
}
#undef ASIOSocketWrapperBuffer
void ASIOSocketWrapper::retryQueuedSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, uint32 current_status) {
    bool queue_check=(current_status&QUEUE_CHECK_FLAG)!=0;
    bool sending_packet=(current_status&ASYNCHRONOUS_SEND_FLAG)!=0;
    while (sending_packet==false||queue_check) {
        if (sending_packet==false) {
            //no one should check the queue without being willing to send
            assert(queue_check==false);
            //potentially volunteer to do the send
            current_status=++mSendingStatus;
            if (current_status==1) {//if this thread is the first into the system with nothing else having claimed the status
                //then this thread should take the torch, check the queue and if not empty be willing to send
                mSendingStatus+=(QUEUE_CHECK_FLAG+ASYNCHRONOUS_SEND_FLAG-1);
                std::deque<Chunk*>toSend;
                mSendQueue.swap(toSend);
                if (toSend.empty()) {//the chunk that we put on the queue must have been sent by someone else
                    //nothing to send, let another thread take up the torch if something was placed there by it
                    mSendingStatus-=(QUEUE_CHECK_FLAG+ASYNCHRONOUS_SEND_FLAG);
                    return;
                }else {//the chunk may be on this queue, but we should promise folks to send it
                    //we just set the asynchronous send flag, so it shouuld still be set
                    assert(mSendingStatus.read()&ASYNCHRONOUS_SEND_FLAG);
                    //turn off the queue check since we've got at least one packet to send off and will therefore come around again for further checks
                    mSendingStatus-=QUEUE_CHECK_FLAG;
                    if (toSend.size()==1) {
                        //if there's just one packet to send: send that one
                        sendToWire(parentMultiSocket,toSend.front());
                    }else {
                        //if there are more packets to send, send those
                        sendToWire(parentMultiSocket,toSend);
                    }
                    return;
                }
            }else {
                //it wasn't us doing the sending...see if those queue checks are still going on
                --mSendingStatus;
            }
        }
        //read the current status for another spin of the loop
        current_status=mSendingStatus.read();
        queue_check=(current_status&QUEUE_CHECK_FLAG)!=0;
        sending_packet=(current_status&ASYNCHRONOUS_SEND_FLAG)!=0;
    }
}

void ASIOSocketWrapper::shutdownAndClose() {
    try {
        mSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    }catch (boost::system::system_error&err) {
        SILOG(tcpsst,insane,"Error shutting down socket: "<<err.what());
    }
    try {
        mSocket->close();
    }catch (boost::system::system_error&err) {                
        SILOG(tcpsst,insane,"Error closing socket: "<<err.what());
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
    TCPSSTLOG(this,"raw",&*chunk->begin(),chunk->size(),false);
    uint32 current_status=++mSendingStatus;
    if (current_status==1) {//we are teh chosen thread
        mSendingStatus+=(ASYNCHRONOUS_SEND_FLAG-1);//committed to be the sender thread
        sendToWire(parentMultiSocket, chunk);
    }else {//if someone else is possibly sending a packet
        //push the packet on the queue
        mSendQueue.push(chunk);
        current_status=--mSendingStatus;
        //the packet is out of our hands now...
        //but the other thread could just have been finishing up and we have missed the send
        //this is our opportunity to take up the torch and send if our packet is still there
        retryQueuedSend(parentMultiSocket,current_status);
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
