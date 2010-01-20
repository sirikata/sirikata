/*  Sirikata Network Utilities
 *  ASIOSocketWrapper.hpp
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
#include "util/UUID.hpp"
#include "util/SizedThreadSafeQueue.hpp"
#include <util/Time.hpp>
#include <util/EWA.hpp>

#define SEND_LATENCY_EWA_ALPHA .1

namespace Sirikata { namespace Network {
class ASIOSocketWrapper;
class ASIOReadBuffer;

void triggerMultiplexedConnectionError(MultiplexedSocket*,ASIOSocketWrapper*,const boost::system::error_code &error);
void ASIOLogBuffer(void * pointerkey, const char extension[16], const uint8* buffer, size_t buffersize);
class ASIOSocketWrapper {
    TCPSocket*mSocket;

    ASIOReadBuffer *mReadBuffer;
    /**
     * The status of sending threads: odd number indicates asynchronous send... number >0  indicates other waiting threads number&ASYNCHRONOUS_SEND_FLAG indicates a thread is
     * currently sending data off. number&QUEUE_CHECK_FLAG means queue is currently being swapped away and items added to the queue may not ever be processed if the queue is empty
     * unless a thread takes up the torch and does it
     */
    AtomicValue<uint32> mSendingStatus;

    struct TimestampedChunk {
        TimestampedChunk()
         : chunk(NULL), time(Time::null())
        {}

        TimestampedChunk(Chunk* _c)
         : chunk(_c), time(Time::local())
        {}

        uint32 size() const {
            return chunk->size();
        }

        Duration sinceCreation() const {
            return Time::local() - time;
        }

        Chunk* chunk;
        Time time;
    };

    /**
     * The queue of packets to send while an active async_send is doing its job
     */
    SizedThreadSafeQueue<TimestampedChunk>mSendQueue;
	enum {
		ASYNCHRONOUS_SEND_FLAG=(1<<29),
		QUEUE_CHECK_FLAG=(1<<30),

	};
    EWA<Duration> mAverageSendLatency;

    std::vector<Stream::StreamID> mPausedSendStreams;

    /** Call this any time a chunk finishes being sent so statistics can be collected. */
    void finishedSendingChunk(const TimestampedChunk& tc);


    typedef boost::system::error_code ErrorCode;
    /**
     * This function sets the QUEUE_CHECK_FLAG and checks the sendQueue for additional packets to send out.
     * If nothing is in the queue then it unsets the ASYNCHRONOUS_SEND_FLAG and QUEUE_CHECK_FLAGS
     * If something is present in the queue it calls sendToWire with the queue
     */
    void finishAsyncSend(const MultiplexedSocketPtr&parentMultiSocket);

    /**
     * The callback for when a single large Chunk at the front of a chunk deque was sent.
     * If the whole large Chunk was not sent then the rest of the Chunk is passed back to sendToWire
     * If the whole Chunk was shipped off, the sendToWire function is called with the rest of the queue unless it is empty in which case the finishAsyncSend is called
     */
    void sendManyDequeItems(const MultiplexedSocketPtr&parentMultiSocket, const std::deque<TimestampedChunk> &const_toSend, const ErrorCode &error, std::size_t bytes_sent);

/**
 * When there's a single packet to be sent to the network, mSocket->async_send is simply called upon the Chunk to be sent
 */
    void sendToWire(const MultiplexedSocketPtr&parentMultiSocket, TimestampedChunk toSend);

/**
 *  This function sends a while queue of packets to the network
 * The function sends each item using a vector of asio::buffers made from the passed in deque
 */
    void sendToWire(const MultiplexedSocketPtr&parentMultiSocket, const std::deque<TimestampedChunk>&const_toSend);

/**
 * If another thread claimed to be sending data asynchronously
 * This function checks to see if the send is still proceeding after the queue push
 * This thread waits until all queue checks are done, then it sees if someone took up the torch of sending the data
 * If no data is being sent, this thread takes up the torch and sets the queue check flag and sees if data is on the queue
 * If no data is on the queue at this point, since it pushed its own data to the queue it assumes someone else successfully sent it off
 * If data is on the queue, it goes ahead and sends the data to the wire by using the appropriate overload of the sendToWire function
 * it then unsets the QUEUE_CHECK_FLAG and if data isn't being set also the ASYNCHRONOUS_SEND_FLAG
 */
    void retryQueuedSend(const MultiplexedSocketPtr&parentMultiSocket, uint32 current_status);

public:

    ASIOSocketWrapper(TCPSocket* socket,uint32 queuedBufferSize,uint32 sendBufferSize)
     : mSocket(socket),
       mReadBuffer(NULL),
       mSendingStatus(0),
       mSendQueue(SizedResourceMonitor(queuedBufferSize)),
       mAverageSendLatency(SEND_LATENCY_EWA_ALPHA)
    {
        //mPacketLogger.reserve(268435456);
    }

    ASIOSocketWrapper(const ASIOSocketWrapper& socket)
     : mSocket(socket.mSocket),
       mReadBuffer(NULL),
       mSendingStatus(0),
       mSendQueue(socket.getResourceMonitor()),
       mAverageSendLatency(SEND_LATENCY_EWA_ALPHA)
    {
        //mPacketLogger.reserve(268435456);
    }

    ASIOSocketWrapper(uint32 queuedBufferSize,uint32 sendBufferSize)
     : mSocket(NULL),
       mReadBuffer(NULL),
       mSendingStatus(0),
       mSendQueue(SizedResourceMonitor(queuedBufferSize)),
       mAverageSendLatency(SEND_LATENCY_EWA_ALPHA)
    {
    }

    ~ASIOSocketWrapper() {

    }

    ASIOSocketWrapper&operator=(const ASIOSocketWrapper& socket){
        mSocket=socket.mSocket;
        return *this;
    }
    const SizedResourceMonitor&getResourceMonitor()const{return mSendQueue.getResourceMonitor();}

    void clearReadBuffer(){
        mReadBuffer=NULL;
    }
    ASIOReadBuffer*getReadBuffer()const{
        return mReadBuffer;
    }
    ASIOReadBuffer* setReadBuffer(ASIOReadBuffer*arb) {
        return mReadBuffer=arb;
    }
    TCPSocket&getSocket() {return *mSocket;}

    const TCPSocket&getSocket()const {return *mSocket;}

    ///close this socket by disallowing sends, then closing
    void shutdownAndClose();

    ///Creates a lowlevel TCPSocket using the following io service
    void createSocket(IOService&io, unsigned int kernelSendBufferSize, unsigned int kernelReceiveBufferSize);

    ///Destroys the lowlevel TCPSocket
    void destroySocket();
    /**
     * Sends the exact bytes contained within the typedeffed vector
     * \param parentMultiSocket the parent multisocket object this socket
     *                          belongs to
     * \param chunk is the exact bytes to put on the network (including streamID
     *              and framing data)
     * \param force if true, force the data to be enqueued even if the queue
     *              policy indicates no more space is available.
     */
    bool rawSend(const MultiplexedSocketPtr&parentMultiSocket, Chunk * chunk, bool force);
    bool canSend(size_t dataSize)const;
    static Chunk*constructControlPacket(const MultiplexedSocketPtr&parentMultiSocket, TCPStream::TCPStreamControlCodes code,const Stream::StreamID&sid);
    /**
     *  Sends a streamID #0 packet with further control data on it.
     *  To start with only stream disconnect and the ack thereof are allowed
     */
    void sendControlPacket(const MultiplexedSocketPtr&parentMultiSocket, TCPStream::TCPStreamControlCodes code,const Stream::StreamID&sid) {
        rawSend(parentMultiSocket,constructControlPacket(parentMultiSocket, code,sid),true);
    }
    /**
     * Sends 24 byte header that indicates version of SST, a unique ID and how many TCP connections should be established
     */
    void sendProtocolHeader(const MultiplexedSocketPtr&parentMultiSocket, const Address& address, const UUID&value, unsigned int numConnections);
    void sendServerProtocolHeader(const MultiplexedSocketPtr& thus, const std::string&origin, const std::string&host, const std::string&port, const std::string&resource_name, const std::string&subprotocol);
    
    void ioReactorThreadPauseStream(const MultiplexedSocketPtr&parentMultiSocket, Stream::StreamID sid);
    void unpauseSendStreams(const MultiplexedSocketPtr&parentMultiSocket);
    Address getRemoteEndpoint()const;
    Address getLocalEndpoint()const;

    // -- Statistics
    Duration averageSendLatency() const;
    Duration averageReceiveLatency() const;
    //converts 3 arrays into a contiguous array of base64 numbers, delimited with a '\0' at the end.
    static Chunk* toBase64ZeroDelim(const MemoryReference&a, const MemoryReference&b, const MemoryReference&c);
    ///makes sure the UUID only consists of unicode-allowed characters and has no null values inside
    static UUID massageUUID(const UUID&);
class CheckCRLF {
    const Array<uint8,TCPStream::MaxWebSocketHeaderSize> *mArray;
    unsigned int mLastTransferred;
public:
    CheckCRLF(const Array<uint8,TCPStream::MaxWebSocketHeaderSize>*array) {
        mArray=array;
        mLastTransferred=0;
        
    }
    size_t operator() (const ErrorCode&error, size_t bytes_transferred);
};


};



} }
