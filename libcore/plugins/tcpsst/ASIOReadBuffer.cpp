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
void BufferPrint(void * pointerkey, const char extension[16], const void * vbuf, size_t size) ;
ASIOReadBuffer* MakeASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket, const MemoryReference &strayBytesAfterHeader) {
    ASIOReadBuffer *retval= parentSocket->getASIOSocketWrapper(whichSocket).setReadBuffer(new ASIOReadBuffer(parentSocket,whichSocket));
    if (strayBytesAfterHeader.size()) {
        memcpy(retval->mBuffer, strayBytesAfterHeader.data(),strayBytesAfterHeader.size());
        retval->asioReadIntoFixedBuffer(ASIOReadBuffer::ErrorCode(),strayBytesAfterHeader.size());//fake callback with real data from past header
    }else {
        retval->readIntoFixedBuffer(parentSocket);
    }
    return retval;
}
void ASIOReadBuffer::processError(MultiplexedSocket*parentSocket, const boost::system::error_code &error){
    //parentSocket->hostDisconnectedCallback(mWhichBuffer,error);
    parentSocket->getASIOSocketWrapper(mWhichBuffer).clearReadBuffer();
    delete this;
}
Network::Stream::ReceivedResponse ASIOReadBuffer::processFullChunk(const MultiplexedSocketPtr &parentSocket, unsigned int whichSocket, const Stream::StreamID&id, Chunk&newChunk){
    return parentSocket->receiveFullChunk(whichSocket,id,newChunk);
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
Network::Stream::ReceivedResponse ASIOReadBuffer::processFullZeroDelimChunk(const MultiplexedSocketPtr &parentSocket, unsigned int whichSocket,const uint8*begin, const uint8*end){
    bool parsedId=false;
    Stream::StreamID id;

    Chunk newChunk(((end-begin)*3)/4+3);//maximum of the size;
    int remainderShift=0;
    int currentValue=0;
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
                    if (parsedId==false&&outBuffPosn>5) {
                        id=parseId(newChunk,outBuffPosn);
                        parsedId=true;
                    }
                    // If that was the equals sign, break out of 'for' loop
                    if( sbiDecode == EQUALS_SIGN_ENC ) {
                        break;
                    }   // end if: equals sign
                }   // end if: quartet built
            }   // end if: equals sign or better
        }   // end if: white space, equals sign or better
    }
    assert(outBuffPosn<=(int)newChunk.size());
    if (!parsedId) {
        id=parseId(newChunk,outBuffPosn);
    }
    newChunk.resize(outBuffPosn);
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
        if (thus->isZeroDelim()) {
            asioReadIntoZeroDelimChunk(ErrorCode(),0);
        }else {
            if (processFullChunk(thus,mWhichBuffer,mNewChunkID,mNewChunk) == Stream::AcceptedData) {
                mNewChunk.resize(0);
                mBufferPos=0;
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
    assert(mBufferPos<mNewChunk.size());
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mBufferPos),mNewChunk.size()-mBufferPos),
                       std::tr1::bind(&ASIOReadBuffer::asioReadIntoChunk,
                                   this,
                                   _1,
                                   _2));
}


void ASIOReadBuffer::readIntoZeroDelimChunk(const MultiplexedSocketPtr &parentSocket){

    mReadStatus=READING_NEW_CHUNK;
    assert(mNewChunk.size()>0);//otherwise should have been filtered out by caller
    mNewChunk.resize(mBufferPos+sBufferLength);
    parentSocket
        ->getASIOSocketWrapper(mWhichBuffer).getSocket()
        .async_receive(boost::asio::buffer(&*(mNewChunk.begin()+mBufferPos),mNewChunk.size()-mBufferPos),
                       std::tr1::bind(&ASIOReadBuffer::asioReadIntoZeroDelimChunk,
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
    if (thus->isZeroDelim()) {
        bool readBufferFull=false;
        unsigned int chunkPos=0;
        unsigned int bufferPos=mBufferPos;
        for (unsigned int i=0;i<mBufferPos;++i) {
            if (mBuffer[i]==0) {
                //delimiter
                if (processFullZeroDelimChunk(thus,mWhichBuffer,mBuffer+chunkPos,mBuffer+i) == Stream::AcceptedData) {
                    chunkPos=i+1;
                }else {                    
                    mReadStatus=PAUSED_FIXED_BUFFER;
                    readBufferFull=true;
                    break;
                }
            }
        }
        unsigned int remainder = bufferPos-chunkPos;
        if (remainder>sBufferLength/2&&!readBufferFull) {//FIXME should be smarter...definitely if remainder==sBufferLength
            mNewChunk.resize(0);
            mNewChunk.insert(mNewChunk.end(),&mBuffer[chunkPos],&mBuffer[mBufferPos]);
            if (mBufferPos!=chunkPos) {
                BufferPrint(this,".rcx",&*mNewChunk.begin(),mBufferPos-chunkPos);
            }
            mBufferPos=remainder;
            readIntoZeroDelimChunk(thus);
        }else {
            if (remainder) {
                std::memmove(mBuffer,mBuffer+chunkPos,remainder);
            }
            mBufferPos=remainder;
            if (!readBufferFull) {
                readIntoFixedBuffer(thus);
            }
        }
    }else {
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


void ASIOReadBuffer::asioReadIntoZeroDelimChunk(const ErrorCode&error,std::size_t bytes_read){
    if (bytes_read)
        BufferPrint(this, ".rcz", &*(mNewChunk.begin()+mBufferPos), bytes_read);

    TCPSSTLOG(this,"rcv",&mNewChunk[mBufferPos],bytes_read,error);
    unsigned int curOffset=0;
    unsigned int curZeroScanLocation=0;//FIXME n^2 algorithm on packet size
    bool readBufferFull=false;
    mBufferPos+=bytes_read;
    MultiplexedSocketPtr thus(mParentSocket.lock());
    if (thus) {
        if (error){
            processError(&*thus,error);
        }else {
            for (unsigned int i=curZeroScanLocation;i<mBufferPos;++i) {
                if (mNewChunk[i]==0) {
                    if (processFullZeroDelimChunk(thus,mWhichBuffer,&*(mNewChunk.begin()+curOffset),&*(mNewChunk.begin()+i)) == Stream::AcceptedData) {    
                        curOffset=i+1;
                    }else if (curOffset==0) {//still a single large chunk
                        readBufferFull=true;
                        mReadStatus=PAUSED_NEW_CHUNK;   
                        break;
                    }else {//will definitely fit in the static buffer
                        readBufferFull=true;
                        mReadStatus=PAUSED_FIXED_BUFFER;
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
                if(!readBufferFull)
                    readIntoFixedBuffer(thus);
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
ASIOReadBuffer::ASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket):mParentSocket(parentSocket){
    mReadStatus=READING_FIXED_BUFFER;
    mBufferPos=0;
    mWhichBuffer=whichSocket;
}

} }
