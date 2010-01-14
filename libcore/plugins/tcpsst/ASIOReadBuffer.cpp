/*  Sirikata Network Utilities
 *  ASIOReadBuffer.cpp
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
#include "TcpsstUtil.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOReadBuffer.hpp"
namespace Sirikata { namespace Network {
ASIOReadBuffer* MakeASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket) {
    return parentSocket->getASIOSocketWrapper(whichSocket).setReadBuffer(new ASIOReadBuffer(parentSocket,whichSocket));
}
void ASIOReadBuffer::processError(MultiplexedSocket*parentSocket, const boost::system::error_code &error){
    //parentSocket->hostDisconnectedCallback(mWhichBuffer,error);
    parentSocket->getASIOSocketWrapper(mWhichBuffer).clearReadBuffer();
    delete this;
}
Network::Stream::ReceivedResponse ASIOReadBuffer::processFullChunk(const MultiplexedSocketPtr &parentSocket, unsigned int whichSocket, const Stream::StreamID&id, Chunk&newChunk){
    return parentSocket->receiveFullChunk(whichSocket,id,newChunk);
}


void ASIOReadBuffer::readIntoFixedBuffer(const MultiplexedSocketPtr &parentSocket){
    mReadStatus=READING_FIXED_BUFFER;

    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(mBuffer+mBufferPos,sBufferLength-mBufferPos),
                       std::tr1::bind(&ASIOReadBuffer::asioReadIntoFixedBuffer,
                                   this,
                                   _1,
                                   _2));
}
void ASIOReadBuffer::ioReactorThreadResumeRead(MultiplexedSocketPtr&thus) {
    SerializationCheck::Scoped ss(thus.get());

    switch(mReadStatus) {
      case PAUSED_FIXED_BUFFER:
        translateBuffer(thus);
        break;
      case PAUSED_NEW_CHUNK:
        if (processFullChunk(thus,mWhichBuffer,mNewChunkID,mNewChunk) == Stream::AcceptedData) {
            mNewChunk.resize(0);
            mBufferPos=0;
            readIntoFixedBuffer(thus);
        }
        break;
      case READING_FIXED_BUFFER:
      case READING_NEW_CHUNK:
        break;
    }
}
void ASIOReadBuffer::readIntoChunk(const MultiplexedSocketPtr &parentSocket){

    mReadStatus=READING_NEW_CHUNK;
    assert(mNewChunk.size()>0);//otherwise should have been filtered out by caller
    assert(mBufferPos<mNewChunk.size());
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mBufferPos),mNewChunk.size()-mBufferPos),
                       std::tr1::bind(&ASIOReadBuffer::asioReadIntoChunk,
                                   this,
                                   _1,
                                   _2));
}

Stream::StreamID ASIOReadBuffer::processPartialChunk(uint8* dataBuffer, uint32 packetLength, uint32 &bufferReceived, Chunk&retval) {
    unsigned int headerLength=bufferReceived;
    Stream::StreamID retid;
    retid.unserialize(dataBuffer,headerLength);
    assert(headerLength<=bufferReceived&&"High water mark must be greater than maximum StreamID size");
    bufferReceived-=headerLength;
    assert(headerLength<=packetLength&&"High water mark must be greater than maximum StreamID size");
    retval.resize(packetLength-headerLength);
    if (packetLength>headerLength) {
        std::memcpy(&*retval.begin(),dataBuffer+headerLength,bufferReceived);
    }
    return retid;
}
void ASIOReadBuffer::translateBuffer(const MultiplexedSocketPtr &thus) {
        unsigned int chunkPos=0;
        unsigned int packetHeaderLength;
        vuint32 packetLength;
        bool readBufferFull=false;
        while ((packetHeaderLength=mBufferPos-chunkPos)!=0&&packetLength.unserialize(mBuffer+chunkPos,packetHeaderLength)) {
            if (mBufferPos-chunkPos<packetLength.read()+packetHeaderLength) {
                if (mBufferPos-chunkPos<sLowWaterMark) {
                    break;//go directly to memmov code and move remnants to beginning of buffer to read a large portion at a time
                }else {
                    mBufferPos-=chunkPos;
                    mBufferPos-=packetHeaderLength;
                    assert(mNewChunk.size()==0);
                    mNewChunkID = processPartialChunk(mBuffer+chunkPos+packetHeaderLength,packetLength.read(),mBufferPos,mNewChunk);
                    readIntoChunk(thus);
                    return;
                }
            }else {
                uint32 chunkLength=packetLength.read();
                Chunk resultChunk;
                Stream::StreamID resultID=processPartialChunk(mBuffer+chunkPos+packetHeaderLength,packetLength.read(),chunkLength,resultChunk);
                size_t vectorSize=resultChunk.size();
                if (processFullChunk(thus,mWhichBuffer,resultID,resultChunk) == Stream::AcceptedData) {
                    chunkPos+=packetHeaderLength+packetLength.read();
                }else {
                    assert(resultChunk.size()==vectorSize);//if the user rejects the packet they should not munge it
                    mReadStatus=PAUSED_FIXED_BUFFER;
                    readBufferFull=true;
                    break;//FIXME
                }
            }
        }
        if (chunkPos!=0&&mBufferPos!=chunkPos) {//move partial bytes to beginning
            std::memmove(mBuffer,mBuffer+chunkPos,mBufferPos-chunkPos);
        }
        mBufferPos-=chunkPos;
        if (!readBufferFull) {
            readIntoFixedBuffer(thus);
        }
}
ASIOReadBuffer::~ASIOReadBuffer() {
}

void ASIOReadBuffer::asioReadIntoChunk(const ErrorCode&error,std::size_t bytes_read){
    TCPSSTLOG(this,"rcv",&mNewChunk[mBufferPos],bytes_read,error);
    mBufferPos+=bytes_read;
    MultiplexedSocketPtr thus(mParentSocket.lock());

    if (thus) {
        if (error){
            processError(&*thus,error);
        }else {
            if (mBufferPos>=mNewChunk.size()){
                size_t vectorSize=mNewChunk.size();
                assert(mBufferPos==vectorSize);
                if (processFullChunk(thus,mWhichBuffer,mNewChunkID,mNewChunk) == Stream::AcceptedData) {
                    mNewChunk.resize(0);
                    mBufferPos=0;
                    readIntoFixedBuffer(thus);
                }else{
                    assert(mNewChunk.size()==vectorSize);//if the user rejects the packet they should not munge it. This is a high level check of that
                    mReadStatus=PAUSED_NEW_CHUNK;
                }
            }else {
                readIntoChunk(thus);
            }
        }
    }else {
        delete this;
    }
}

void ASIOReadBuffer::asioReadIntoFixedBuffer(const ErrorCode&error,std::size_t bytes_read){
    TCPSSTLOG(this,"rcv",&mBuffer[mBufferPos],bytes_read,error);
    mBufferPos+=bytes_read;
    MultiplexedSocketPtr thus(mParentSocket.lock());

    if (thus) {
        if (error){
            processError(&*thus,error);
        }else {
            translateBuffer(thus);
        }
    }else {
        delete this;// the socket is deleted
    }
}
ASIOReadBuffer::ASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket):mParentSocket(parentSocket){
    mReadStatus=READING_FIXED_BUFFER;
    mBufferPos=0;
    mWhichBuffer=whichSocket;
    readIntoFixedBuffer(parentSocket);
}

} }
