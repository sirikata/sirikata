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

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/network/Asio.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include "TcpsstUtil.hpp"
#include "TCPStream.hpp"
#include <sirikata/core/queue/ThreadSafeQueue.hpp>
#include "ASIOSocketWrapper.hpp"
#include "MultiplexedSocket.hpp"
#include "ASIOReadBuffer.hpp"
#include "VariableLength.hpp"
namespace Sirikata { namespace Network {

struct ASIOReadBufferUtil {
    // A couple of static utility methods to help us track whether streams are being
    // paused or not:
    // 1. Wrapper for PauseReceiveCallbacks which allows the invoking method to
    // track whether the callback was invoked without knowing anything about the
    // callback itself
    static void _mark_pause_bool_true(bool* pause_bool, const Stream::PauseReceiveCallback& cb) {
        *pause_bool = true;
        cb();
    }
    // 2. A noop pause receive callback
    static void _pause_receive_callback_noop() {
        return;
    }
    // 3. Callback which will set a read status and bool indicating if the read
    //    buffer is full.
    static void _pause_receive_callback__status_full(ASIOReadBuffer::ReadStatus* read_status, ASIOReadBuffer::ReadStatus read_status_value, bool* read_buffer_full) {
        *read_status = read_status_value;
        if (read_buffer_full) *read_buffer_full = true;
    }
};

void ASIOReadBuffer::bindFunctions(IOStrand* strand) {
    mAsioReadIntoChunk =
        strand->wrap(
            std::tr1::bind(&ASIOReadBuffer::asioReadIntoChunk,
                this,
                _1,
                _2
            )
        );
    mAsioReadIntoFixedBuffer =
        strand->wrap(
            std::tr1::bind(&ASIOReadBuffer::asioReadIntoFixedBuffer,
                this,
                _1,
                _2
            )
        );
}

void BufferPrint(void * pointerkey, const char extension[16], const void * vbuf, size_t size) ;
ASIOReadBuffer* MakeASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket, const MemoryReference &strayBytesAfterHeader, TCPStream::StreamType type) {
    ASIOReadBuffer *retval= parentSocket->getASIOSocketWrapper(whichSocket).setReadBuffer(new ASIOReadBuffer(parentSocket,whichSocket,type));
    if (strayBytesAfterHeader.size()) {
        memcpy(retval->mBuffer, strayBytesAfterHeader.data(),strayBytesAfterHeader.size());
        retval->asioReadIntoFixedBuffer(ASIOReadBuffer::ErrorCode(),strayBytesAfterHeader.size());//fake callback with real data from past header
    }else {
        retval->readIntoFixedBuffer(parentSocket);
    }
    return retval;
}
void ASIOReadBuffer::processError(MultiplexedSocket*parentSocket, const boost::system::error_code &error){
    parentSocket->hostDisconnectedCallback(mWhichBuffer,error);
    parentSocket->getASIOSocketWrapper(mWhichBuffer).clearReadBuffer();
    delete this;
}
ASIOReadBuffer::ReceivedResponse ASIOReadBuffer::processFullChunk(const MultiplexedSocketPtr &parentSocket, unsigned int whichSocket, const Stream::StreamID&id, Chunk&newChunk, const Stream::PauseReceiveCallback& pauseReceive){
    if (*(int*)mDataMask) {
        for (unsigned int i = 0; i < newChunk.size(); i++) {
            newChunk[i] ^= mDataMask[i & 3];
        }
    }
    *(int*)mDataMask = 0; // Mask no longer applies after one packet.
    if (mLastFrame) {
        bool user_paused_stream = false;
        parentSocket->receiveFullChunk(
            whichSocket,id,newChunk,
            std::tr1::bind(ASIOReadBufferUtil::_mark_pause_bool_true, &user_paused_stream, pauseReceive)
            );
        if (!user_paused_stream) {
            mNewChunk.resize(0);
            mChunkBufferPos=0;
        }
        return (user_paused_stream ? PausedStream : StreamNotPaused);
    } else {
        return StreamNotPaused;
    }
}
static signed char WHITE_SPACE_ENC = -5; // Indicates white space in encoding
static signed char EQUALS_SIGN_ENC = -1; // Indicates equals sign in encoding
static int decode4to3(const signed char source[4],Chunk& destination, int destOffset) {
    uint32 outBuf=((source[0]<<18)|(source[1]<<12));
    destination[destOffset]=(uint8)(outBuf/65536);
    if (source[2]==EQUALS_SIGN_ENC) {
        return 1;
    }
    outBuf|=(source[2]<<6);
    destination[destOffset+1]=(uint8)((outBuf/256)&255);
    if (source[3]==EQUALS_SIGN_ENC) {
            return 2;
    }
    outBuf|=source[3];
    destination[destOffset+2]=(uint8)(outBuf&255);
    return 3;
}

static signed char DECODABET [] = {
        -9,-9,-9,-9,-9,-9,-9,-9,-9,                 // Decimal  0 -  8
        -5,-5,                                      // Whitespace: Tab and Linefeed
        -9,-9,                                      // Decimal 11 - 12
        -5,                                         // Whitespace: Carriage Return
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 14 - 26
        -9,-9,-9,-9,-9,                             // Decimal 27 - 31
        -5,                                         // Whitespace: Space
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,              // Decimal 33 - 42
        62,                                         // Plus sign at decimal 43
        -9,-9,-9,                                   // Decimal 44 - 46
        63,                                         // Slash at decimal 47
        52,53,54,55,56,57,58,59,60,61,              // Numbers zero through nine
        -9,-9,-9,                                   // Decimal 58 - 60
        -1,                                         // Equals sign at decimal 61
        -9,-9,-9,                                      // Decimal 62 - 64
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,            // Letters 'A' through 'N'
        14,15,16,17,18,19,20,21,22,23,24,25,        // Letters 'O' through 'Z'
        -9,-9,-9,-9,-9,-9,                          // Decimal 91 - 96
        26,27,28,29,30,31,32,33,34,35,36,37,38,     // Letters 'a' through 'm'
        39,40,41,42,43,44,45,46,47,48,49,50,51,     // Letters 'n' through 'z'
        -9,-9,-9,-9,-9                                 // Decimal 123 - 126
};
static signed char URLSAFEDECODABET [] = {
        -9,-9,-9,-9,-9,-9,-9,-9,-9,                 // Decimal  0 -  8
        -5,-5,                                      // Whitespace: Tab and Linefeed
        -9,-9,                                      // Decimal 11 - 12
        -5,                                         // Whitespace: Carriage Return
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 14 - 26
        -9,-9,-9,-9,-9,                             // Decimal 27 - 31
        -5,                                         // Whitespace: Space
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,              // Decimal 33 - 42
        62,                                         // Plus sign at decimal 43
        -9,62,-9,                                   // Decimal 44 - 46
        63,                                         // Slash at decimal 47
        52,53,54,55,56,57,58,59,60,61,              // Numbers zero through nine
        -9,-9,-9,                                   // Decimal 58 - 60
        -1,                                         // Equals sign at decimal 61
        -9,-9,-9,                                      // Decimal 62 - 64
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,            // Letters 'A' through 'N'
        14,15,16,17,18,19,20,21,22,23,24,25,        // Letters 'O' through 'Z'
        -9,-9,-9,-9,63,-9,                          // Decimal 91 - 94 underscore 95
        26,27,28,29,30,31,32,33,34,35,36,37,38,     // Letters 'a' through 'm'
        39,40,41,42,43,44,45,46,47,48,49,50,51,     // Letters 'n' through 'z'
        -9,-9,-9,-9,-9                                 // Decimal 123 - 127
};
static Stream::StreamID parseId(Chunk&newChunk,int&outBuffPosn) {
    Stream::StreamID id;
    unsigned int headerLength=outBuffPosn;
    id.unserialize(&*newChunk.begin(),headerLength);
    if ((int)headerLength!=outBuffPosn) {
        std::memmove(&*newChunk.begin(),&*(newChunk.begin()+headerLength),outBuffPosn-headerLength);//move the rest of the packet out
    }
    outBuffPosn-=headerLength;
    return id;
}



void ASIOReadBuffer::readIntoFixedBuffer(const MultiplexedSocketPtr &parentSocket){
    mReadStatus=READING_FIXED_BUFFER;

    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(mBuffer+mFixedBufferPos,sBufferLength-mFixedBufferPos),mAsioReadIntoFixedBuffer);
}
void ASIOReadBuffer::ioReactorThreadResumeRead(MultiplexedSocketPtr&thus) {
    SerializationCheck::Scoped ss(thus.get());

    switch(mReadStatus) {
      case PAUSED_FIXED_BUFFER:
        translateFixedBuffer(thus);
        break;
      case PAUSED_NEW_CHUNK:
          {
              ReceivedResponse resp = processFullChunk(
                  thus,mWhichBuffer,mNewChunkID,mNewChunk,
                  ASIOReadBufferUtil::_pause_receive_callback_noop
              );
              if (resp == StreamNotPaused) {
                  mFixedBufferPos = 0;
                  readIntoFixedBuffer(thus);
              }
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
    assert(mChunkBufferPos<mNewChunk.size());
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mChunkBufferPos),mNewChunk.size()-mChunkBufferPos),mAsioReadIntoChunk);
}




void ASIOReadBuffer::processPartialChunk(uint8* dataBuffer, uint32 packetLength, uint32 &bufferReceived, Chunk&currentChunk) {
    unsigned int headerLength=bufferReceived;
    if (mFirstFrame) {
        Stream::StreamID retid;
        if (*(int*)mDataMask) {
            uint8 tempId[16];
            unsigned int i;
            for (i = 0; i < 16 && i < headerLength; i++) {
                tempId[i] = dataBuffer[i] ^ mDataMask[i & 3];
            }
            retid.unserialize(tempId,headerLength);
            for (i = 0; i < (headerLength & 3); i++) {
                uint8 tmpMask = mDataMask[0];
                mDataMask[0] = mDataMask[1];
                mDataMask[1] = mDataMask[2];
                mDataMask[2] = mDataMask[3];
                mDataMask[3] = tmpMask;
            }
        } else {
            retid.unserialize(dataBuffer,headerLength);
        }
        if (headerLength > bufferReceived) {
            retid = 0;
            headerLength = 0;
            SILOG(network,debug,"High water mark must be greater than maximum StreamID size");
        }
        mNewChunkID = retid;
        bufferReceived-=headerLength;
    } else {
        headerLength = 0; // FIXME: Need to deal with header split over continuation frames
        // This can happen if the browser decides to split a frame into a size smaller than the length
        // of the full stream id.
    }
    currentChunk.resize(mChunkBufferPos + packetLength - headerLength);
    if (packetLength>headerLength) {
        std::memcpy(&*currentChunk.begin() + mChunkBufferPos, dataBuffer + headerLength, bufferReceived);
    }
}
void ASIOReadBuffer::translateFixedBuffer(const MultiplexedSocketPtr &thus) {
    bool readBufferFull=false;
    unsigned int currentFixedBufferPos=0;
    VariableLength packetLength;
    bool delimitedPacket=false;
    while (mFixedBufferPos!=currentFixedBufferPos) {
        unsigned int length;
        unsigned int packetHeaderLength;
        if (mStreamType == TCPStream::RFC_6455) {
            if (mFixedBufferPos - currentFixedBufferPos < 2) {
                break;
            }
            uint8 flags = mBuffer[currentFixedBufferPos];
            uint8 type = flags & 0x0f;
            bool fin = (flags & 0x80) ? true : false;
            // 0x00 is a continuation frame.
            mFirstFrame = (type != 0x00);
            mLastFrame = fin;

            length = mBuffer[currentFixedBufferPos + 1];
            bool masked = (length & 0x80) ? true: false;
            uint8 mask[4] = {0};
            length &= 0x7f;
            if (type >= 0x08) {
                // Control packet -- not fragmented; length <= 125
                if (mFixedBufferPos - currentFixedBufferPos < 2 + length) {
                    break;
                }
                if (type == 0x09) { // ping
                    thus->receivePing(mWhichBuffer, MemoryReference(&mBuffer[currentFixedBufferPos + 2], length), false);
                }
                if (type == 0x0a) { // pong
                    thus->receivePing(mWhichBuffer, MemoryReference(&mBuffer[currentFixedBufferPos + 2], length), true);
                }
                currentFixedBufferPos += (2 + length);
                continue;
            }
            packetHeaderLength = 2 + (masked ? 4 : 0) + (length == 127 ? 8 : (length == 126 ? 2 : 0));
            if (mFixedBufferPos - currentFixedBufferPos < packetHeaderLength) {
                break;
            }
            currentFixedBufferPos += 2;
            if (length == 126) {
                length = (mBuffer[currentFixedBufferPos] << 8) | mBuffer[currentFixedBufferPos + 1];
                currentFixedBufferPos += 2;
            } else if (length == 127) {
                // Ignore higher-order bits since we assume 32-bit lengths.
                length = //(mBuffer[currentFixedBufferPos] << 56) | (mBuffer[currentFixedBufferPos + 1] << 48) |
                         //(mBuffer[currentFixedBufferPos + 2] << 40) | (mBuffer[currentFixedBufferPos + 3] << 32) |
                         (mBuffer[currentFixedBufferPos + 4] << 24) | (mBuffer[currentFixedBufferPos + 5] << 16) |
                         (mBuffer[currentFixedBufferPos + 6] << 8) | mBuffer[currentFixedBufferPos + 7];
                currentFixedBufferPos += 8;
            }
            if (masked) {
                *(int*)mDataMask = *(int*)(&mBuffer[currentFixedBufferPos]);
                currentFixedBufferPos += 4;
            } else {
                *(int*)mDataMask = 0;
            }
        } else { // LENGTH_DELIM
            mFirstFrame = mLastFrame = true; // Application packets can't span more than one protocol-level packet.
            *(int*)mDataMask = 0;
            packetHeaderLength= mFixedBufferPos-currentFixedBufferPos;
            if (packetLength.unserialize(mBuffer+currentFixedBufferPos,packetHeaderLength)) {
                currentFixedBufferPos += packetHeaderLength;
                //if there is enough room to parse the length of the length-delimited packet
                length = packetLength.read();
            } else {
                //length delimited packet without sufficient data to determine the length
                break;
            }
        }
        // Now we have the packet length and the header length. Process the data.
        {
            if (mFixedBufferPos-currentFixedBufferPos<length) {
                if (mFixedBufferPos-currentFixedBufferPos+packetHeaderLength<sLowWaterMark) {
                    currentFixedBufferPos -= packetHeaderLength;
                    break;//go directly to memmov code and move remnants to beginning of buffer to read a large portion at a time
                }else {
                    mFixedBufferPos-=currentFixedBufferPos;
                    assert(!mFirstFrame || mNewChunk.size()==0);
                    processPartialChunk(mBuffer+currentFixedBufferPos,length,mFixedBufferPos,mNewChunk);
                    mChunkBufferPos += mFixedBufferPos;
                    mFixedBufferPos = 0;
                    readIntoChunk(thus);
                    return;
                }
            }else {
                unsigned int bufferReceived = length;
                // We may copy this packet into mNewChunk, but we are not going to update the position
                // If we pause in this case, we will overwrite with the same data next time.
                // No other state should be affected.
                processPartialChunk(mBuffer+currentFixedBufferPos,length,bufferReceived,mNewChunk);
                size_t vectorSize = mNewChunk.size();
                mChunkBufferPos += bufferReceived;

                ReceivedResponse process_resp = processFullChunk(
                    thus,mWhichBuffer,mNewChunkID,mNewChunk,
                    std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_FIXED_BUFFER, &readBufferFull)
                );
                if (process_resp == StreamNotPaused) {
                    currentFixedBufferPos += length;
                } else { // Paused, most work already handled by callback
                    // Let's forget everything that happened in this loop iteration.
                    currentFixedBufferPos -= packetHeaderLength;
                    mChunkBufferPos -= bufferReceived;
                    assert(mNewChunk.size()==vectorSize);//if the user rejects the packet they should not munge it
                    break;
                }
            }
        }
// Old protocol version. Delete soon.
/*
        else {
            mFirstPacket = mLastPacket = true; // Application packets can't span more than one protocol-level packet.

            unsigned int packetHeaderLength= mFixedBufferPos-currentFixedBufferPos;
            if (packetLength.unserialize(mBuffer+currentFixedBufferPos,packetHeaderLength)) {//if there is enough room to parse the length of the length-delimited packet
                if (mFixedBufferPos-currentFixedBufferPos<packetLength.read()+packetHeaderLength) {
                    if (mFixedBufferPos-currentFixedBufferPos<sLowWaterMark) {
                        break;//go directly to memmov code and move remnants to beginning of buffer to read a large portion at a time
                    }else {
                        mFixedBufferPos-=currentFixedBufferPos;
                        mFixedBufferPos-=packetHeaderLength;
                        assert(mNewChunk.size()==0);
                        processPartialChunk(mBuffer+currentFixedBufferPos+packetHeaderLength,packetLength.read(),mFixedBufferPos,mNewChunk);
                        mChunkBufferPos = mFixedBufferPos;
                        mFixedBufferPos = 0;
                        readIntoChunk(thus);
                        return;
                    }
                }else {
                    uint32 chunkLength=packetLength.read();
                    mNewChunk.resize(chunkLength);
                    processPartialChunk(mBuffer+currentFixedBufferPos+packetHeaderLength,packetLength.read(),chunkLength,mNewChunk);
                    mChunkBufferPos += chunkLength;
                    size_t vectorSize=mNewChunk.size();

                    ReceivedResponse process_resp = processFullChunk(
                        thus,mWhichBuffer,mNewChunkID,mNewChunk,
                        std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_FIXED_BUFFER, &readBufferFull)
                    );
                    if (process_resp == StreamNotPaused) {
                        currentFixedBufferPos+=packetHeaderLength+packetLength.read();
                    } else { // Paused, most work already handled by callback
                        assert(resultChunk.size()==vectorSize);//if the user rejects the packet they should not munge it
                        break;
                    }
                }
            }else {//length delimited packet without sufficient data to determine the length
                break;
            }
        }
*/
    }
    if (currentFixedBufferPos!=0&&mFixedBufferPos!=currentFixedBufferPos) {//move partial bytes to beginning
        std::memmove(mBuffer,mBuffer+currentFixedBufferPos,mFixedBufferPos-currentFixedBufferPos);
    }
    mFixedBufferPos-=currentFixedBufferPos;
    if (!readBufferFull) {
        readIntoFixedBuffer(thus);
    }
}
ASIOReadBuffer::~ASIOReadBuffer() {
}

void ASIOReadBuffer::asioReadIntoChunk(const ErrorCode&error,std::size_t bytes_read){
    if (bytes_read)
        BufferPrint(this, ".rcc", &*mNewChunk.begin()+mChunkBufferPos, bytes_read);
    TCPSSTLOG(this,"rcv",&mNewChunk[mChunkBufferPos],bytes_read,error);
    mChunkBufferPos+=bytes_read;
    MultiplexedSocketPtr thus(mParentSocket.lock());

    if (thus) {
        if (error){
            processError(&*thus,error);
        }else {
            if (mChunkBufferPos>=mNewChunk.size()){
                size_t vectorSize=mNewChunk.size();
                assert(mChunkBufferPos==vectorSize);

                ReceivedResponse resp = processFullChunk(
                    thus,mWhichBuffer,mNewChunkID,mNewChunk,
                    std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_NEW_CHUNK, (bool*)NULL)
                );
                if (resp == StreamNotPaused) {
                    mFixedBufferPos=0;
                    readIntoFixedBuffer(thus);
                } else {
                    // Paused, status already set by callback, just do this check
                    assert(mNewChunk.size()==vectorSize);//if the user rejects the packet they should not munge it. This is a high level check of that
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
    TCPSSTLOG(this,"rcv",&mBuffer[mFixedBufferPos],bytes_read,error);
    if (bytes_read)
        BufferPrint(this, ".rcv", &(mBuffer[mFixedBufferPos]), bytes_read);

    mFixedBufferPos+=bytes_read;
    MultiplexedSocketPtr thus(mParentSocket.lock());

    if (thus) {
        if (error){
            processError(&*thus,error);
        }else {
            translateFixedBuffer(thus);
        }
    }else {
        delete this;// the socket is deleted
    }
}
ASIOReadBuffer::ASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket, TCPStream::StreamType type):mParentSocket(parentSocket){
    *(int*)mDataMask = 0;
    IOStrand* strand = parentSocket->getStrand();
    bindFunctions(strand);
    mReadStatus=READING_FIXED_BUFFER;
    mFixedBufferPos=0;
    mChunkBufferPos=0;
    mFirstFrame = mLastFrame = false;
    mWhichBuffer=whichSocket;
    mCachedRejectedChunk=NULL;
    mStreamType = type;
}

} }
