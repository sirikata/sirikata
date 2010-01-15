/*  Sirikata Network Utilities
 *  ASIOReadBuffer.hpp
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

#include "MultiplexedSocket.hpp"

namespace Sirikata {
namespace Network {

class ASIOReadBuffer {
    enum {
        ///The length of the fixed buffer.  This should only affect the largest chunk of data delivered at once,
        ///since async_receive should return as soon as data is available.  Therefore we use a relatively large
        ///buffer to avoid too much overhead from IO operations.
        sBufferLength=64*1024,
        ///The low water mark is the point at which reads are shifted from the fixed sized buffer into a preallocated
        ///chunk.  Since the chunk is preallocated, this has to be greater than the longest possible header length.
        ///It also must be less than sBufferLength.  Since this is only really necessary to handle large packets and
        ///avoid extra copying (of the available packet info from end of buffer to front of buffer, then over into the
        ///real packet), we set this to be as large as possible, sBufferLength.
        sLowWaterMark=sBufferLength
    };
    enum ReadStatus{
        PAUSED_FIXED_BUFFER=0x0,
        READING_FIXED_BUFFER=0x1,
        PAUSED_NEW_CHUNK=0x2,
        READING_NEW_CHUNK=0x3
    }mReadStatus;
    ///A fixed length buffer to read incoming requests when the data is unknown in size or so far small in size
    uint8 mBuffer[sBufferLength];
    ///Where is ASIO writing to in mBuffer
    unsigned int mBufferPos;
    ///Which actual low level tcp socket from the mParentSocket is used for communication
    unsigned int mWhichBuffer;
    ///A new chunk being read directly into--usually this member is only used to hold a large packet of information, otherwise the fixed length buffer is used
    Chunk mNewChunk;
    ///The StreamID of a new, partially examined new chunk
    Stream::StreamID mNewChunkID;
    ///The shared structure responsible for holding state about the associated TCPStream that this class reads and interprets data from
    std::tr1::weak_ptr<MultiplexedSocket> mParentSocket;
    typedef boost::system::error_code ErrorCode;
    /**
     * This forwards the error message to the MultiplexedSocket so the appropriate action may be taken
     * (including,possibly, disconnecting and shutting down the socket connections and all associated streams
     */
    void processError(MultiplexedSocket*parentSocket, const ErrorCode &error);
    /**
     * This function passes the contents of a chunk to the multiplexed socket for callback handling
     * \param parentSocket is the MultiplexedSocket responsible for this stream with the relevant callback information
     * \param whichSocket is the current ASIO socket responsible for having read the data. It must equal mWhichBuffer
     * \param sid is the StreamID that sent the data which made it to this socket and got processed. It will help determine which callback to call
     * \param newChunk is the chunk that was sent from the other side to this side and is ready for client processing (or server processing if sid==Stream::StreamID())
     * \returns whether the host can accept a packet into a lower queue
     */
    Network::Stream::ReceivedResponse processFullChunk(const MultiplexedSocketPtr &parentSocket,
                          unsigned int whichSocket,
                          const Stream::StreamID& sid,
                          Chunk&newChunk);

    /**
     * This function passes the contents of a chunk to the multiplexed socket for callback handling
     * \param parentSocket is the MultiplexedSocket responsible for this stream with the relevant callback information
     * \param whichSocket is the current ASIO socket responsible for having read the data. It must equal mWhichBuffer
     * \param bufferStart is the start of the chunk that was sent from the other side to this side and is ready for client processing (or server processing if sid==Stream::StreamID())
     * \param bufferEnd is the end of the chunk that was sent from the other side to this side and is ready for processing
     * \returns whether the host can accept a packet into a lower queue
     */
    Network::Stream::ReceivedResponse processFullZeroDelimChunk(const MultiplexedSocketPtr &parentSocket,
                                                                unsigned int whichSocket,
                                                                const uint8*bufferStart,const uint8*bufferEnd);
    /**
     *  This function is called when either 0 information is known about the data to be read (such as size, etc)
     *  or if the data is known but the packet is sufficiently small that other packets may be conjoined with it in the buffer
     *  This function tells asio to read data from the socket into mBuffer at offset mBufferPos upto the end of the buffer
     */
    void readIntoFixedBuffer(const MultiplexedSocketPtr &parentSocket);
    /**
     *  This function is called when a sufficiently large chunk needs to be filled up from a previous readIntoFixedBuffer call.
     *  This function will tell ASIO to read directly into the mNewChunk class, offset by the mBufferPos upto the value of mNewChunk.size()
     */
    void readIntoChunk(const MultiplexedSocketPtr &parentSocket);

    /**
     *  This function is called when a sufficiently large chunk needs to be filled up from a previous readIntoFixedBuffer call.
     *  This function will tell ASIO to read directly into the mNewChunk class, offset by the mBufferPos to an unknown length
     *  Can be called with size 0 to resume a read on a zero delimited chunk
     */
    void readIntoZeroDelimChunk(const MultiplexedSocketPtr &parentSocket);

    /**
     * Examines a buffer of bytes and converts it into a partially or totally filled chunk
     * and gives back information about unprocessed bytes and the StreamID that sent the chunk
     * \param dataBuffer is the buffer to be read and turned into an active Chunk
     * \param packetLength is the length of the to-be-returned Chunk plus the length of that chunk's streamID
     * \param bufferReceived is the length of the dataBuffer, and the value returned in the bufferReceived is number of useful bytes copied to the returned chunk
     * \param retval is the newly allocated chunk sized appropriately to hold all data that will ever be copied to it
     * \returns the StreamID that this chunk was sent from
     */
    Stream::StreamID processPartialChunk(uint8* dataBuffer, uint32 packetLength, uint32 &bufferReceived, Chunk&retval);

    /**
     * Examines the class variable mBuffer from the beginning to mBufferPos and translates all packets contained within to chunks and calls the appropriate callback
     * If the information in the last unprocessed chunk is less than sLowWaterMark that excess information is moved to the front of the buffer and readIntoFixedBuffer is called
     * If the information in the last unprocessed chunk is greater than the sLowWaterMark
     * then a new chunk is made specifically for the remaining data using the processPartialChunk function and readIntoChunk is called
     */
    void translateBuffer(const MultiplexedSocketPtr &thus);

    /**
     * The ASIO callback when ASIO was reading into a singleChunk
     * The function reacts to errors by calling processErrors or a missing MultiplexedSocket by deleting this
     * Otherwise the function reacts to
     *     a partially full packet by calling readIntoFixedChunk and
     *     a full packet by calling processFullChunk to invoke appropriate callbacks then readIntoFixedBuffer
     */
    void asioReadIntoChunk(const ErrorCode&error,std::size_t bytes_read);

    /**
     * The ASIO callback when ASIO was reading into a singleChunk
     * The function reacts to errors by calling processErrors or a missing MultiplexedSocket by deleting this
     * Otherwise the function reacts to
     *     a partially full packet by calling readIntoFixedChunk and
     *     a full packet by calling processFullChunk to invoke appropriate callbacks then readIntoFixedBuffer
     */
    void asioReadIntoZeroDelimChunk(const ErrorCode&error,std::size_t bytes_read);


    /**
     * The ASIO callback when ASIO was reading into the mBuffer from mBufferPos
     * The function reacts to errors by calling processErrors or a missing MultiplexedSocket by deleting this
     * Otherwise the function farms work off to translateBuffer
     */
    void asioReadIntoFixedBuffer(const ErrorCode&error,std::size_t bytes_read);
public:
    /**
     *  The only public interface to TCPReadBuffer is the constructor which takes in a MultiplexedSocket and an integer offset
     *  \param parentSocket the MultiplexedSocket which defines the whole connection (if the weak_ptr fails, the connection is bunk
     *  \param whichSocket indicates which substream this read buffer is for, so the appropriate ASIO socket can be retrieved
     */
    ASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket);
    /**
     *  If read was paused by a user that could not proocess a packet
     *  This call attempts to deliver the packet to a user again
     *  If the user accepts the packet then the system is asked for future packets
     *  This function may be called if the read was not paused
     */
    void ioReactorThreadResumeRead(MultiplexedSocketPtr&thus);
    ~ASIOReadBuffer();
};

ASIOReadBuffer* MakeASIOReadBuffer(const MultiplexedSocketPtr &parentSocket,unsigned int whichSocket);

} // namespace Network
} // namespace Sirikata
