/*  Sirikata Network Utilities
 *  TCPReadBuffer.hpp
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

namespace Sirikata { namespace Network {

class TCPReadBuffer {
    static const unsigned int sBufferLength=1440;
    static const unsigned int mLowWaterMark=256;
    uint8 mBuffer[sBufferLength];
    unsigned int mBufferPos;
    unsigned int mWhichBuffer;
    Chunk mNewChunk;
    Stream::StreamID mNewChunkID;
    std::tr1::weak_ptr<MultiplexedSocket> mParentSocket;
    typedef boost::system::error_code ErrorCode;
public:
    TCPReadBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket,unsigned int whichSocket):mParentSocket(parentSocket){
        mBufferPos=0;
        mWhichBuffer=whichSocket;
        readIntoFixedBuffer(parentSocket);
    }
    void processError(MultiplexedSocket*parentSocket, const ErrorCode &error);
    void processFullChunk(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket,
                          unsigned int whichSocket,
                          const Stream::StreamID&,
                          const Chunk&mNewChunk);
    void readIntoFixedBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket);
    void readIntoChunk(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket);
    Stream::StreamID processPartialChunk(uint8* dataBuffer, uint32 packetLength, uint32 &bufferReceived, Chunk&retval) {
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
    void translateBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &thus) {
        unsigned int chunkPos=0;
        unsigned int packetHeaderLength;
        Stream::uint30 packetLength;
        while ((packetHeaderLength=mBufferPos-chunkPos)!=0&&packetLength.unserialize(mBuffer+chunkPos,packetHeaderLength)) {
            if (mBufferPos-chunkPos<packetLength.read()+packetHeaderLength) {
                if (mBufferPos-chunkPos<mLowWaterMark) {
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
                processFullChunk(thus,mWhichBuffer,resultID,resultChunk);
                chunkPos+=packetHeaderLength+packetLength.read();
            }
        }
        if (chunkPos!=0&&mBufferPos!=chunkPos) {//move partial bytes to beginning
            std::memmove(mBuffer,mBuffer+chunkPos,mBufferPos-chunkPos);
        }
        mBufferPos-=chunkPos;
        readIntoFixedBuffer(thus);
    }
    void asioReadIntoChunk(const ErrorCode&error,std::size_t bytes_read){
#ifdef TCPSSTLOG
        char TESTs[1024];
        sprintf(TESTs,"%d.socket",(int)(size_t)this);
        FILE * fp=fopen(TESTs,"a");
        fwrite(&mNewChunk[mBufferPos],bytes_read,1,fp);
        fclose(fp);
#endif
        mBufferPos+=bytes_read;
        std::tr1::shared_ptr<MultiplexedSocket> thus(mParentSocket.lock());

        if (thus) {
            if (error){
                processError(&*thus,error);
            }else {
                if (mBufferPos>=mNewChunk.size()){
                    assert(mBufferPos==mNewChunk.size());
                    processFullChunk(thus,mWhichBuffer,mNewChunkID,mNewChunk);
                    mNewChunk.resize(0);
                    mBufferPos=0;
                    readIntoFixedBuffer(thus);
                }else {
                    readIntoChunk(thus);
                }
            }
        }else {
            delete this;
        }
    }
    void asioReadIntoFixedBuffer(const ErrorCode&error,std::size_t bytes_read){
#ifdef TCPSSTLOG
        char TESTs[1024];
        sprintf(TESTs,"%d.socket",(int)(size_t)this);
        FILE * fp=fopen(TESTs,"a");
        fwrite(&mBuffer[mBufferPos],bytes_read,1,fp);
        fclose(fp);
#endif
        mBufferPos+=bytes_read;
        std::tr1::shared_ptr<MultiplexedSocket> thus(mParentSocket.lock());

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
};
} }
