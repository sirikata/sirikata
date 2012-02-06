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
    mAsioReadIntoZeroDelimChunk =
        strand->wrap(
            std::tr1::bind(&ASIOReadBuffer::asioReadIntoZeroDelimChunk,
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
    bool user_paused_stream = false;
    parentSocket->receiveFullChunk(
        whichSocket,id,newChunk,
        std::tr1::bind(ASIOReadBufferUtil::_mark_pause_bool_true, &user_paused_stream, pauseReceive)
    );
    return (user_paused_stream ? PausedStream : AcceptedData);
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

//Adding this bug to the stream library and turning on base64 triggers a space server crash, so we need to fix it ASAP
ASIOReadBuffer::ReceivedResponse ASIOReadBuffer::processFullZeroDelimChunk(const MultiplexedSocketPtr &parentSocket, unsigned int whichSocket,const uint8*begin, const uint8*end, const Stream::PauseReceiveCallback& pauseReceive){
    if (mCachedRejectedChunk) {
        bool user_paused_stream_cached = false;
        parentSocket->receiveFullChunk(
            whichSocket,mNewChunkID,*mCachedRejectedChunk,
            std::tr1::bind(ASIOReadBufferUtil::_mark_pause_bool_true, &user_paused_stream_cached, pauseReceive)
        );
        if (!user_paused_stream_cached) {
            delete mCachedRejectedChunk;
            mCachedRejectedChunk=NULL;
        }
        return (user_paused_stream_cached ? PausedStream : AcceptedData);
    }
    Stream::StreamID id;

    assert(begin!=end&&*begin<=127);
    ++begin;//begin should have 0x00-0x7f as initial byte to indicate 0xff delimitation
    unsigned int streamIdOffset=(unsigned int)(end-begin);
    bool parsedId=id.unserializeFromHex(begin,streamIdOffset);
    if (parsedId==false) {
        return AcceptedData;//burn it, runt packet
    }
    begin+=streamIdOffset;


    Chunk newChunk(((end-begin)*3)/4+3);//maximum of the size;
    int remainderShift=0;

    int outBuffPosn=0;
    signed char b4[4];
    uint8 b4Posn=0;
    for (;begin!=end;++begin) {
        uint8 cur=(*begin);
        uint8 sbiCrop = (uint8)(cur & 0x7f); // Only the low seven bits
        signed char sbiDecode = URLSAFEDECODABET[ sbiCrop ];   // Special value

        // White space, Equals sign, or legit Base64 character
        // Note the values such as -5 and -9 in the
        // DECODABETs at the top of the file.
        if( sbiDecode >= WHITE_SPACE_ENC && cur==sbiCrop )  {
            if( sbiDecode >= EQUALS_SIGN_ENC ) {
                b4[ b4Posn++ ] = sbiDecode;           // Save non-whitespace
                if( b4Posn > 3 ) {                  // Time to decode?
                    outBuffPosn += decode4to3( b4, newChunk, outBuffPosn );
                    b4Posn = 0;
                    // If that was the equals sign, break out of 'for' loop
                    if( sbiDecode == EQUALS_SIGN_ENC ) {
                        break;
                    }   // end if: equals sign
                }   // end if: quartet built
            }   // end if: equals sign or better
        }   // end if: white space, equals sign or better
    }
    assert(outBuffPosn<=(int)newChunk.size());
    newChunk.resize(outBuffPosn);

    bool user_paused_stream = false;
    parentSocket->receiveFullChunk(
        whichSocket,id,newChunk,
        std::tr1::bind(ASIOReadBufferUtil::_mark_pause_bool_true, &user_paused_stream, pauseReceive)
    );
    if (user_paused_stream) {
        mCachedRejectedChunk=new Chunk;
        mCachedRejectedChunk->swap(newChunk);
        mNewChunkID=id;
    }
    return (user_paused_stream ? PausedStream : AcceptedData);
}


void ASIOReadBuffer::readIntoFixedBuffer(const MultiplexedSocketPtr &parentSocket){
    mReadStatus=READING_FIXED_BUFFER;

    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(mBuffer+mBufferPos,sBufferLength-mBufferPos),mAsioReadIntoFixedBuffer);
}
void ASIOReadBuffer::ioReactorThreadResumeRead(MultiplexedSocketPtr&thus) {
    SerializationCheck::Scoped ss(thus.get());

    switch(mReadStatus) {
      case PAUSED_FIXED_BUFFER:
        translateBuffer(thus);
        break;
      case PAUSED_NEW_DELIM_CHUNK:
            asioReadIntoZeroDelimChunk(ErrorCode(),0);
            break;
      case PAUSED_NEW_CHUNK:
          {
              ReceivedResponse resp = processFullChunk(
                  thus,mWhichBuffer,mNewChunkID,mNewChunk,
                  ASIOReadBufferUtil::_pause_receive_callback_noop
              );
              if (resp == AcceptedData) {
                  mNewChunk.resize(0);
                  mBufferPos=0;
                  readIntoFixedBuffer(thus);
              }
          }
        break;
      case READING_FIXED_BUFFER:
      case READING_NEW_CHUNK:
      case READING_NEW_DELIM_CHUNK:
        break;
    }
}
void ASIOReadBuffer::readIntoChunk(const MultiplexedSocketPtr &parentSocket){

    mReadStatus=READING_NEW_CHUNK;
    assert(mNewChunk.size()>0);//otherwise should have been filtered out by caller
    assert(mBufferPos<mNewChunk.size());
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mBufferPos),mNewChunk.size()-mBufferPos),mAsioReadIntoChunk);
}


void ASIOReadBuffer::readIntoZeroDelimChunk(const MultiplexedSocketPtr &parentSocket){

    mReadStatus=READING_NEW_DELIM_CHUNK;
    assert(mNewChunk.size()>0);//otherwise should have been filtered out by caller
    mNewChunk.resize(mBufferPos+sBufferLength);
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mBufferPos),mNewChunk.size()-mBufferPos),mAsioReadIntoZeroDelimChunk);
}



Stream::StreamID ASIOReadBuffer::processPartialChunk(uint8* dataBuffer, uint32 packetLength, uint32 &bufferReceived, Chunk&retval) {
    unsigned int headerLength=bufferReceived;
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
    bool readBufferFull=false;
    unsigned int chunkPos=0;
    VariableLength packetLength;
    bool delimitedPacket=false;
    while (mBufferPos!=chunkPos) {
        if (mStreamType == TCPStream::BASE64_ZERODELIM && mBuffer[chunkPos]<=127) {//if this is a 0xff delimited packet
            uint8*where=std::find(mBuffer+chunkPos+1,mBuffer+mBufferPos,0xff);
            if (where==mBuffer+mBufferPos) {
                unsigned int remainder = mBufferPos-chunkPos;
                if (remainder>sBufferLength/2) {
                    mNewChunk.resize(0);
                    mNewChunk.insert(mNewChunk.end(),mBuffer+chunkPos,mBuffer+mBufferPos);
                    BufferPrint(this,".rcx",&*mNewChunk.begin(),mBufferPos-chunkPos);
                    mBufferPos=remainder;
                    readIntoZeroDelimChunk(thus);
                    return;
                }else {
                    break;
                }
            }else {
                ReceivedResponse process_resp = processFullZeroDelimChunk(
                    thus,mWhichBuffer,mBuffer+chunkPos,where,
                    std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_FIXED_BUFFER, &readBufferFull)
                );
                if (process_resp == AcceptedData) {
                    chunkPos=(where-mBuffer)+1;
                }
                else { // Paused
                    break;
                }
            }
        }else if (mStreamType == TCPStream::RFC_6455) {
            if (mBufferPos - chunkPos < 2) {
                break;
            }
            uint8 flags = mBuffer[chunkPos];
            uint8 type = flags & 0x0f;
            bool fin = (flags & 0x80) ? true : false;
            if (type == 0x00) {
                // FIXME: Continuation frame.
            }
            if (!fin) {
                // FIXME: We are reading a partial packet.
            }
            unsigned int length = mBuffer[chunkPos + 1];
            bool masked = (length & 0x80) ? true: false;
            uint8 mask[4] = {0};
            length &= 0x7f;
            if (type >= 0x08) {
                // Control packet -- not fragmented; length <= 125
                if (mBufferPos - chunkPos < 2 + length) {
                    break;
                }
                if (type == 0x09) { // ping
                    thus->receivePing(mWhichBuffer, MemoryReference(&mBuffer[chunkPos + 2], length), false);
                }
                if (type == 0x0a) { // pong
                    thus->receivePing(mWhichBuffer, MemoryReference(&mBuffer[chunkPos + 2], length), true);
                }
                chunkPos += (2 + length);
                continue;
            }
            unsigned int packetHeaderLength = 2 + (masked ? 4 : 0) + (length == 127 ? 8 : (length == 126 ? 2 : 0));
            if (mBufferPos - chunkPos < packetHeaderLength) {
                break;
            }
            chunkPos += 2;
            if (length == 126) {
                length = (mBuffer[chunkPos] << 8) | mBuffer[chunkPos + 1];
                chunkPos += 2;
            } else if (length == 127) {
                // Ignore higher-order bits since we assume 32-bit lengths.
                length = //(mBuffer[chunkPos] << 56) | (mBuffer[chunkPos + 1] << 48) |
                         //(mBuffer[chunkPos + 2] << 40) | (mBuffer[chunkPos + 3] << 32) |
                         (mBuffer[chunkPos + 4] << 24) | (mBuffer[chunkPos + 5] << 16) |
                         (mBuffer[chunkPos + 6] << 8) | mBuffer[chunkPos + 7];
                chunkPos += 8;
            }
            if (masked) {
                *(int*)mDataMask = *(int*)(&mBuffer[chunkPos]);
                chunkPos += 4;
            } else {
                *(int*)mDataMask = 0;
            }

            if (mBufferPos-chunkPos<length) {
                if (mBufferPos-chunkPos<sLowWaterMark) {
                    chunkPos -= packetHeaderLength;
                    break;//go directly to memmov code and move remnants to beginning of buffer to read a large portion at a time
                }else {
                    mBufferPos-=chunkPos;
                    assert(mNewChunk.size()==0);
                    mNewChunkID = processPartialChunk(mBuffer+chunkPos,length,mBufferPos,mNewChunk);
                    readIntoChunk(thus);
                    return;
                }
            }else {
                Chunk resultChunk;
                unsigned int bufferReceived = length;
                Stream::StreamID resultID = processPartialChunk(mBuffer+chunkPos,length,bufferReceived,resultChunk);
                size_t vectorSize = resultChunk.size();

// UNMASK!!!!
                ReceivedResponse process_resp = processFullChunk(
                    thus,mWhichBuffer,resultID,resultChunk,
                    std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_FIXED_BUFFER, &readBufferFull)
                );
                if (process_resp == AcceptedData) {
                    chunkPos += length;
                } else { // Paused, most work already handled by callback
                    chunkPos -= packetHeaderLength;
                    assert(resultChunk.size()==vectorSize);//if the user rejects the packet they should not munge it
                    break;
                }
            }
        }else {
            unsigned int packetHeaderLength= mBufferPos-chunkPos;
            if (packetLength.unserialize(mBuffer+chunkPos,packetHeaderLength)) {//if there is enough room to parse the length of the length-delimited packet
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

                    ReceivedResponse process_resp = processFullChunk(
                        thus,mWhichBuffer,resultID,resultChunk,
                        std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_FIXED_BUFFER, &readBufferFull)
                    );
                    if (process_resp == AcceptedData) {
                        chunkPos+=packetHeaderLength+packetLength.read();
                    } else { // Paused, most work already handled by callback
                        assert(resultChunk.size()==vectorSize);//if the user rejects the packet they should not munge it
                        break;
                    }
                }
            }else {//length delimited packet without sufficient data to determine the length
                break;
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
    if (bytes_read)
        BufferPrint(this, ".rcc", &*mNewChunk.begin()+mBufferPos, bytes_read);
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

                ReceivedResponse resp = processFullChunk(
                    thus,mWhichBuffer,mNewChunkID,mNewChunk,
                    std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, PAUSED_NEW_CHUNK, (bool*)NULL)
                );
                if (resp == AcceptedData) {
                    mNewChunk.resize(0);
                    mBufferPos=0;
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


void ASIOReadBuffer::asioReadIntoZeroDelimChunk(const ErrorCode&error,std::size_t bytes_read){
    if (bytes_read)
        BufferPrint(this, ".rcz", &*(mNewChunk.begin()+mBufferPos), bytes_read);

    TCPSSTLOG(this,"rcv",&mNewChunk[mBufferPos],bytes_read,error);
    unsigned int curOffset=0;
    unsigned int curZeroScanLocation=(bytes_read==0/*if the packet was rejected before*/?0:mBufferPos);
    bool readBufferFull=false;
    mBufferPos+=bytes_read;
    bool additionalLengthDelimMessagesToParse=false;//set to true if the user has sent length delimited packets after a large zero delim packets
    MultiplexedSocketPtr thus(mParentSocket.lock());
    if (thus) {
        if (error){
            processError(&*thus,error);
        }else {
            for (unsigned int i=curZeroScanLocation;i<mBufferPos;++i) {
                if (mNewChunk[i]==0xff) {
                    ReadStatus read_status_value =
                        (curOffset == 0) ? PAUSED_NEW_DELIM_CHUNK : PAUSED_FIXED_BUFFER;
                    ReceivedResponse resp = processFullZeroDelimChunk(
                        thus,mWhichBuffer,&*(mNewChunk.begin()+curOffset),&*(mNewChunk.begin()+i),
                        std::tr1::bind(ASIOReadBufferUtil::_pause_receive_callback__status_full, &mReadStatus, read_status_value, &readBufferFull)
                    );
                    if (resp == AcceptedData) {
                        curOffset=i+1;
                        if (curOffset<mBufferPos&&mNewChunk[curOffset]>=128) {
                            //switch from zero delim mode to length mode, handled in translateBuffer loop
                            //eliminates need for duplicate length parsing code
                            //and since it's a rare case that users switch modes midstream,
                            //we can afford an extra copy during the transition for simpler code
                            additionalLengthDelimMessagesToParse=true;
                            break;
                        }
                    } else { // PauseReceive
                        break;
                    }
                }
            }
            if (curOffset==0) {//no packet processed, still large
                if (!readBufferFull)
                    readIntoZeroDelimChunk(thus);
            } else {
                assert (mBufferPos-curOffset<=sBufferLength);//only read that much in--would only have read it in if there was one less byte for the last zero delim of the big
                                                           //packet, so we should need to read in at least one byte
                if (mBufferPos!=curOffset) {
                    std::memcpy(mBuffer,&*(mNewChunk.begin()+curOffset),mBufferPos-curOffset);
                }
                mBufferPos-=curOffset;
                mNewChunk.clear();

                if(!readBufferFull) {
                    if (additionalLengthDelimMessagesToParse) {
                        translateBuffer(thus);
                    }else {
                        readIntoFixedBuffer(thus);
                    }
                }
            }
        }
    }else {
        delete this;
    }
}



void ASIOReadBuffer::asioReadIntoFixedBuffer(const ErrorCode&error,std::size_t bytes_read){
    TCPSSTLOG(this,"rcv",&mBuffer[mBufferPos],bytes_read,error);
    if (bytes_read)
        BufferPrint(this, ".rcv", &(mBuffer[mBufferPos]), bytes_read);

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
ASIOReadBuffer::ASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket, TCPStream::StreamType type):mParentSocket(parentSocket){
    *(int*)mDataMask = 0;
    IOStrand* strand = parentSocket->getStrand();
    bindFunctions(strand);
    mReadStatus=READING_FIXED_BUFFER;
    mBufferPos=0;
    mWhichBuffer=whichSocket;
    mCachedRejectedChunk=NULL;
    mStreamType = type;
}

} }
