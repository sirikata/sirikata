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

#include "util/Standard.hh"
#include "util/AtomicTypes.hpp"

#include "TCPDefinitions.hpp"
#include "TCPStream.hpp"
#include "util/ThreadSafeQueue.hpp"
#include <boost/thread.hpp>
namespace Sirikata { namespace Network {
const int TCPStream::SendStatusClosing=(1<<30);
#define SIRIKATA_TCP_STREAM_HEADER "SSTTCP"
#define SIRIKATA_TCP_STREAM_HEADER_LENGTH 6
class TCPSetCallbacks:public Stream::SetCallbacks {
public:
    TCPStream::Callbacks* mCallbacks;
    TCPStream*mStream;
    MultiplexedSocket*mMultiSocket;
    TCPSetCallbacks(MultiplexedSocket*ms,TCPStream *strm) {
        mCallbacks=NULL;
        mStream=strm;
    }
    virtual void operator()(const Stream::ConnectionCallback &connectionCallback,
                            const Stream::SubstreamCallback &substreamCallback,
                            const Stream::BytesReceivedCallback &bytesReceivedCallback);

};
enum TCPStreamControlCodes {
    TCPStreamCloseStream=1,
    TCPStreamAckCloseStream=2
};
static const int TcpSstHeaderSize=24;
void copyHeader(void * destination, const UUID&key, unsigned int num) {
    std::memcpy(destination,SIRIKATA_TCP_STREAM_HEADER,SIRIKATA_TCP_STREAM_HEADER_LENGTH);
    ((char*)destination)[SIRIKATA_TCP_STREAM_HEADER_LENGTH]='0'+(num/10)%10;
    ((char*)destination)[SIRIKATA_TCP_STREAM_HEADER_LENGTH+1]='0'+(num%10);
    std::memcpy(((char*)destination)+SIRIKATA_TCP_STREAM_HEADER_LENGTH+2,
                key.getArray().begin(),
                UUID::static_size);
}
class ASIOSocketWrapper;

void triggerMultiplexedConnectionError(MultiplexedSocket*,ASIOSocketWrapper*,const boost::system::error_code &error);

class ASIOSocketWrapper {
    TCPSocket*mSocket;
    /**
     * The status of sending threads: odd number indicates asynchronous send... number >0  indicates other waiting threads number&ASYNCHRONOUS_SEND_FLAG indicates a thread is
     * currently sending data off. number&QUEUE_CHECK_FLAG means queue is currently being swapped away and items added to the queue may not ever be processed if the queue is empty
     * unless a thread takes up the torch and does it
     */
    AtomicValue<uint32> mSendingStatus;
    ThreadSafeQueue<Chunk*>mSendQueue;
    static const uint32 ASYNCHRONOUS_SEND_FLAG=(1<<29);
    static const uint32 QUEUE_CHECK_FLAG=(1<<30);
    static const size_t PACKET_BUFFER_SIZE=1400;
    uint8 mBuffer[PACKET_BUFFER_SIZE];
    friend class MultiplexedSocket;

/**
 * This function sets the QUEUE_CHECK_FLAG and checks the sendQueue for additional packets to send out.
 * If nothing is in the queue then it unsets the ASYNCHRONOUS_SEND_FLAG and QUEUE_CHECK_FLAGS
 * If something is present in the queue it calls sendToWire with the queue
 */
    void finishAsyncSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket) {
        mSendingStatus+=QUEUE_CHECK_FLAG;
        std::deque<Chunk*>toSend;
        mSendQueue.swap(toSend);
        std::size_t num_packets=toSend.size();
        if (num_packets==0) {
            mSendingStatus-=(ASYNCHRONOUS_SEND_FLAG+QUEUE_CHECK_FLAG);
        }else {
            mSendingStatus-=QUEUE_CHECK_FLAG;
            if (num_packets==1)
                sendToWire(parentMultiSocket,toSend.front());
            else
                sendToWire(parentMultiSocket,toSend);
        }
    }
    /**
     * The callback for when a single Chunk was sent.
     * If the whole Chunk was not sent then the rest of the Chunk is passed back to sendToWire
     * If the whole Chunk was shipped off, the finishAsyncSend function is called
     */
    void sendLargeChunkItem(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk *toSend, size_t originalOffset, const boost::system::error_code &error, std::size_t bytes_sent) {
        if (error)  {
            triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
            std::cerr<<"Socket disconnected...waiting for recv to trigger error condition\n";
        }else if (bytes_sent+originalOffset!=toSend->size()) {
            sendToWire(parentMultiSocket,toSend,originalOffset+bytes_sent);
        }else {
            finishAsyncSend(parentMultiSocket);
        }
    }

    /**
     * The callback for when a single large Chunk at the front of a chunk deque was sent.
     * If the whole large Chunk was not sent then the rest of the Chunk is passed back to sendToWire
     * If the whole Chunk was shipped off, the sendToWire function is called with the rest of the queue unless it is empty in which case the finishAsyncSend is called
     */
    void sendLargeDequeItem(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, std::deque<Chunk*> toSend, size_t originalOffset, const boost::system::error_code &error, std::size_t bytes_sent) {
        if (error )   {
            triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
            std::cerr<<"Socket disconnected...waiting for recv to trigger error condition\n";
        } else if (bytes_sent+originalOffset!=toSend.front()->size()) {
            sendToWire(parentMultiSocket,toSend,originalOffset+bytes_sent);
        }else if (toSend.size()<2) {
            finishAsyncSend(parentMultiSocket);
        }else {
            delete toSend.front();
            toSend.pop_front();
            if (toSend.size()==1) {
                sendToWire(parentMultiSocket,toSend.front());
            }else {
                sendToWire(parentMultiSocket,toSend);
            }
        }
    }
    /**
     * The callback for when a static buffer was shipped to the network.
     * If the whole static buffer was not sent then the rest of the static buffer is passed back to async_send and the currentBuffer is offset by the necessary ammt to finish off
     * sending it. This could be slightly inefficient if there are tons of other packets in the deque toSend but generally the buffer size should be chosen so that it can be sent
     * in one go.
     * If the whole buffer was shipped off, the sendToWire function is called with the rest of the queue unless it is empty
     */
    void sendStaticBuffer(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*>&toSend, uint8* currentBuffer, size_t bufferSize, size_t lastChunkOffset,  const boost::system::error_code &error, std::size_t bytes_sent) {
        if ( error ) {
            triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
            std::cerr<<"Socket disconnected...waiting for recv to trigger error condition\n";
        }else if (bytes_sent!=bufferSize) {
            mSocket->async_send(boost::asio::buffer(currentBuffer+bytes_sent,bufferSize-bytes_sent),
                                boost::bind(&ASIOSocketWrapper::sendStaticBuffer,
                                            this,
                                            parentMultiSocket,
                                            toSend,
                                            currentBuffer+bytes_sent,
                                            bufferSize-bytes_sent,
                                            lastChunkOffset,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));

        }else {
            if (!toSend.empty()) {
                if (toSend.size()==1) {
                    sendToWire(parentMultiSocket,toSend.front(),lastChunkOffset);
                }else {
                    sendToWire(parentMultiSocket,toSend,lastChunkOffset);
                }
            }else {
                finishAsyncSend(parentMultiSocket);
            }
        }
    }
/**
 * When there's a single packet to be sent to the network, mSocket->async_send is simply called upon the Chunk to be sent
 */
    void sendToWire(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk *toSend, size_t bytesSent=0) {
            mSocket->async_send(boost::asio::buffer(&*toSend->begin()+bytesSent,toSend->size()-bytesSent),
                                boost::bind(&ASIOSocketWrapper::sendLargeChunkItem,
                                            this,
                                            parentMultiSocket,
                                            toSend,
                                            bytesSent,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }
/**
 *  This function sends a while queue of packets to the network
 * The function first checks to see if the front packet is too large to fit in the packet-sized buffer.
 * If it's too large it sends the packet individually much like a chunk would be sent individually and it is left on the queue for data storage purposes
 * If the packet is not too large it and all subsequent packets that can fit are jammed into the packet sized mBuffer
 *  and then those packets are deleted from the queue and shipped to the network partial packets are left on the queue in that case
 */
    void sendToWire(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*>&const_toSend, size_t bytesSent=0){
        if (const_toSend.front()->size()-bytesSent>PACKET_BUFFER_SIZE||const_toSend.size()==1) {
            mSocket->async_send(boost::asio::buffer(&*const_toSend.front()->begin()+bytesSent,const_toSend.front()->size()-bytesSent),
                                boost::bind(&ASIOSocketWrapper::sendLargeDequeItem,
                                            this,
                                            parentMultiSocket,
                                            const_toSend,
                                            bytesSent,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        }else if (const_toSend.front()->size()){
            std::deque<Chunk*> toSend=const_toSend;
            size_t bufferLocation=toSend.front()->size()-bytesSent;
            std::memcpy(mBuffer,&*toSend.front()->begin()+bytesSent,toSend.front()->size()-bytesSent);
            delete toSend.front();
            toSend.pop_front();
            bytesSent=0;
            while (bufferLocation<PACKET_BUFFER_SIZE&&toSend.size()) {
                if (toSend.front()->size()>PACKET_BUFFER_SIZE-bufferLocation) {
                    bytesSent=PACKET_BUFFER_SIZE-bufferLocation;
                    std::memcpy(mBuffer+bufferLocation,&*toSend.front()->begin(),bytesSent);
                    bufferLocation=PACKET_BUFFER_SIZE;
                }else {
                    std::memcpy(mBuffer+bufferLocation,&*toSend.front()->begin(),toSend.front()->size());
                    bufferLocation+=toSend.front()->size();
                    delete toSend.front();
                    toSend.pop_front();
                }
            }
            mSocket->async_send(boost::asio::buffer(mBuffer,bufferLocation),
                                boost::bind(&ASIOSocketWrapper::sendStaticBuffer,
                                            this,
                                            parentMultiSocket,
                                            toSend,
                                            mBuffer,
                                            bufferLocation,
                                            bytesSent,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        }
    }
/**
 * If another thread claimed to be sending data asynchronously
 * This function checks to see if the send is still proceeding after the queue push
 * This thread waits until all queue checks are done, then it sees if someone took up the torch of sending the data
 * If no data is being sent, this thread takes up the torch and sets the queue check flag and sees if data is on the queue
 * If no data is on the queue at this point, since it pushed its own data to the queue it assumes someone else successfully sent it off
 * If data is on the queue, it goes ahead and sends the data to the wire by using the appropriate overload of the sendToWire function
 * it then unsets the QUEUE_CHECK_FLAG and if data isn't being set also the ASYNCHRONOUS_SEND_FLAG
 */
    void retryQueuedSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, uint32 current_status) {
        bool queue_check=(current_status&QUEUE_CHECK_FLAG)!=0;
        bool sending_packet=(current_status&ASYNCHRONOUS_SEND_FLAG)!=0;
        while (sending_packet==false||queue_check) {
            if (sending_packet==false) {
                assert(queue_check==false);//no one should check the queue without being willing to send
                current_status=++mSendingStatus;
                if (current_status==1) {
                    current_status+=(QUEUE_CHECK_FLAG+ASYNCHRONOUS_SEND_FLAG-1);
                    std::deque<Chunk*>toSend;
                    mSendQueue.swap(toSend);
                    if (toSend.empty()) {//the chunk that we put on the queue must have been sent by someone else
                        current_status-=(QUEUE_CHECK_FLAG+ASYNCHRONOUS_SEND_FLAG);
                        return;
                    }else {//the chunk may be on this queue, but we should promise folks to send it
                        current_status-=QUEUE_CHECK_FLAG;
                        if (toSend.size()==1) {
                            sendToWire(parentMultiSocket,toSend.front());
                        }else {
                            sendToWire(parentMultiSocket,toSend);
                        }
                        return;
                    }
                }
            }else if (queue_check) {
                current_status=mSendingStatus.read();
                queue_check=(current_status&QUEUE_CHECK_FLAG)!=0;
                sending_packet=(current_status&ASYNCHRONOUS_SEND_FLAG)!=0;
            }
        }
    }
public:
    ASIOSocketWrapper(TCPSocket* socket) :mSocket(socket){
    }
    ASIOSocketWrapper(const ASIOSocketWrapper& socket) :mSocket(socket.mSocket){
    }
    ASIOSocketWrapper&operator=(const ASIOSocketWrapper& socket){
        mSocket=socket.mSocket;
        return *this;
    }
    ASIOSocketWrapper() :mSocket(NULL){
    }
    TCPSocket&getSocket() {return *mSocket;}
    const TCPSocket&getSocket()const {return *mSocket;}

    /**
     * Sends the exact bytes contained within the typedeffed vector
     * \param chunk is the exact bytes to put on the network (including streamID and framing data)
     */
    void rawSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk * chunk) {
        uint32 current_status=++mSendingStatus;
        //if someone else is currently sending a packet
        if (current_status&ASYNCHRONOUS_SEND_FLAG) {
            //push the packet on the queue
            mSendQueue.push(chunk);
            current_status=--mSendingStatus;
            //may have missed the send
            retryQueuedSend(parentMultiSocket,current_status);
        }else if (current_status==1) {
            current_status+=(ASYNCHRONOUS_SEND_FLAG-1);//committed to be the sender thread
            sendToWire(parentMultiSocket, chunk);
        }
    }
/**
 *  Sends a streamID #0 packet with further control data on it. To start with only stream disconnect and the ack thereof are allowed
 */
    void sendControlPacket(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, TCPStreamControlCodes code,const Stream::StreamID&sid) {
        const unsigned int max_size=16;
        uint8 dataStream[max_size];
        unsigned int size=max_size;
        Stream::StreamID controlStream;//control packet
        controlStream.serialize(dataStream,size);
        assert(size<max_size);
        dataStream[size++]=code;
        unsigned int cur=size;
        size=max_size-size;
        sid.serialize(&dataStream[cur],size);
        assert(size+cur<=max_size);
        rawSend(parentMultiSocket,new Chunk(dataStream,dataStream+size+cur));
    }
/**
 * Sends 24 byte header that indicates version of SST, a unique ID and how many TCP connections should be established
 */
    void sendProtocolHeader(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const UUID&value, unsigned int numConnections) {
        UUID return_value=UUID::random();

        Chunk *headerData=new Chunk(TcpSstHeaderSize);
        copyHeader(&*headerData->begin(),value,numConnections);
        rawSend(parentMultiSocket,headerData);
    }
};
const uint32 ASIOSocketWrapper::ASYNCHRONOUS_SEND_FLAG;
const uint32 ASIOSocketWrapper::QUEUE_CHECK_FLAG;
const size_t ASIOSocketWrapper::PACKET_BUFFER_SIZE;

using namespace boost::asio::ip;
class RawRequest {
public:
    bool unordered;
    bool unreliable;
    Stream::StreamID originStream;
    Chunk * data;
};
class TCPReadBuffer {
    static const unsigned int mBufferLength=1440;
    static const unsigned int mLowWaterMark=256;
    uint8 mBuffer[mBufferLength];
    unsigned int mBufferPos;
    unsigned int mWhichBuffer;
    Chunk mNewChunk;
    Stream::StreamID mNewChunkID;
    std::tr1::weak_ptr<MultiplexedSocket> mParentSocket;
public:
    TCPReadBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket,unsigned int whichSocket):mParentSocket(parentSocket){
        mBufferPos=0;
        mWhichBuffer=whichSocket;
        readIntoFixedBuffer(parentSocket);
    }
    void processError(MultiplexedSocket*parentSocket, const boost::system::error_code &error);
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
        while (mBufferPos-chunkPos>4) {
            uint32 packetLength;
            std::memcpy(&packetLength,mBuffer+mBufferPos,4);
            packetLength=ntohl(packetLength);
            if (mBufferPos-chunkPos<packetLength+4) {
                if (mBufferPos-chunkPos<mLowWaterMark) {
                    break;//go directly to memmov code and move remnants to beginning of buffer to read a large portion at a time
                }else {
                    mBufferPos-=chunkPos;
                    mBufferPos-=4;
                    mNewChunkID = processPartialChunk(mBuffer+chunkPos+4,packetLength,mBufferPos,mNewChunk);
                    assert(mNewChunk.size()==0);
                    readIntoChunk(thus);
                    return;
                }
            }else {
                uint32 chunkLength=packetLength;
                Chunk resultChunk;
                Stream::StreamID resultID=processPartialChunk(mBuffer+chunkPos+4,packetLength,chunkLength,resultChunk);
                processFullChunk(thus,mWhichBuffer,resultID,resultChunk);
                chunkPos+=4+packetLength;
            }
        }
        if (chunkPos!=0&&mBufferPos!=chunkPos) {//move partial bytes to beginning
            std::memmove(mBuffer,mBuffer+chunkPos,mBufferPos-chunkPos);
        }
        mBufferPos-=chunkPos;
        readIntoFixedBuffer(thus);
    }
    void asioReadIntoChunk(const boost::system::error_code&error,std::size_t bytes_read){
        mBufferPos+=bytes_read;
        try {
            std::tr1::shared_ptr<MultiplexedSocket> thus(mParentSocket.lock());
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
        }catch(std::tr1::bad_weak_ptr&) {
            delete this;
        }
    }
    void operator()(const boost::system::error_code&error,std::size_t bytes_read){
        mBufferPos+=bytes_read;
        try {
            std::tr1::shared_ptr<MultiplexedSocket> thus(mParentSocket.lock());
            if (error){
                processError(&*thus,error);
            }else {
                translateBuffer(thus);
            }
        }catch (std::tr1::bad_weak_ptr&) {
            delete this;// the socket is deleted
        }
    }
};
class MultiplexedSocket:public SelfWeakPtr<MultiplexedSocket> {
    boost::asio::ip::tcp::resolver mResolver;
    std::vector<ASIOSocketWrapper> mSockets;
/// these items are synced together take the lock, check for preconnection,,, if connected, don't take lock...otherwise take lock and push data onto the new requests queue
    static boost::mutex sConnectingMutex;
    std::deque<RawRequest> mNewRequests;
    typedef std::tr1::unordered_map<Stream::StreamID,TCPStream::Callbacks*,Stream::StreamID::Hasher> CallbackMap;
	///Workaround for VC8 bug that does not define std::pair<Stream::StreamID,Callbacks*>::operator=
    class StreamIDCallbackPair{
    public:
        Stream::StreamID mID;
        TCPStream::Callbacks* mCallback;
        StreamIDCallbackPair(Stream::StreamID id,TCPStream::Callbacks* cb):mID(id) {
            mCallback=cb;
        }
        std::pair<Stream::StreamID,TCPStream::Callbacks*> pair() const{
           return std::pair<Stream::StreamID,TCPStream::Callbacks*>(mID,mCallback);
        }
    };
    std::deque<StreamIDCallbackPair> mCallbackRegistration;
    volatile enum SocketConnectionPhase{
        PRECONNECTION,
        CONNECTED,
        DISCONNECTED
    }mSocketConnectionPhase;
    ///Copies items from CallbackRegistration to mCallbacks Assumes sConnectingMutex is taken
    void ioReactorThreadCommitCallback(StreamIDCallbackPair& newcallback) {
            if (newcallback.mCallback==NULL) {
                CallbackMap::iterator where=mCallbacks.find(newcallback.mID);
                if (where!=mCallbacks.end()) {
                    delete where->second;
                    mCallbacks.erase(where);
                }else {
                    assert("ERROR in finding callback to erase for stream ID"&&false);
                }
            }else {
                mCallbacks.insert(newcallback.pair());
            }
    }
    bool CommitCallbacks(std::deque<StreamIDCallbackPair> &registration, SocketConnectionPhase status, bool setConnectedStatus=false) {
        bool statusChanged;
        if (setConnectedStatus||!mCallbackRegistration.empty()) {
			boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
            statusChanged=(status!=mSocketConnectionPhase);
            if (setConnectedStatus) {
                mSocketConnectionPhase=status;
            }
            mCallbackRegistration.swap(registration);
            bool other_registrations=registration.empty();
            assert(other_registrations==false);
        }
        while (!registration.empty()) {
            ioReactorThreadCommitCallback(registration.front());
            registration.pop_front();
        }
        return statusChanged;
    }
    size_t leastBusyStream() {
        ///FIXME look at bytes to be sent or recently used, etc
        return rand()%mSockets.size();
    }
    float dropChance(const Chunk*data,size_t whichStream) {
        return .25;
    }
    static void sendBytesNow(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data) {
        static Stream::StreamID::Hasher hasher;
        size_t whichStream=data.unordered?thus->leastBusyStream():hasher(data.originStream)%thus->mSockets.size();
        if (data.unreliable==false||rand()/(float)RAND_MAX>thus->dropChance(data.data,whichStream)) {
            thus->mSockets[whichStream].rawSend(thus,data.data);
        }
    }
public:
    IOService&getASIOService(){return mResolver.io_service();}
    static void closeStream(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const Stream::StreamID&sid) {
        for (std::vector<ASIOSocketWrapper>::iterator i=thus->mSockets.begin(),ie=thus->mSockets.end();i!=ie;++i) {
            i->sendControlPacket(thus,TCPStreamCloseStream,sid);
        }
    }
    static void sendBytes(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const RawRequest&data) {
        if (thus->mSocketConnectionPhase==CONNECTED) {
            sendBytesNow(thus,data);
        }else {
            bool lockCheckConnected=false;
            {
                boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
                if (thus->mSocketConnectionPhase==CONNECTED) {
                    lockCheckConnected=true;
                }else if(thus->mSocketConnectionPhase==DISCONNECTED) {
                    //retval=false;
                    //FIXME is this the correct thing to do?
                    thus->mNewRequests.push_back(data);
                }else {
                    thus->mNewRequests.push_back(data);
                }
            }
            if (lockCheckConnected) {
                sendBytesNow(thus,data);
            }
        }
    }
    /**
     * Adds callbacks onto the queue of callbacks-to-be-added
     */
    void addCallbacks(const Stream::StreamID&sid, TCPStream::Callbacks* cb) {
        boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
        mCallbackRegistration.push_back(StreamIDCallbackPair(sid,cb));
    }
    AtomicValue<uint32> mHighestStreamID;
    ///a map from StreamID to count of number of acked close requests--to avoid any unordered packets coming in
    std::tr1::unordered_map<Stream::StreamID,unsigned int,Stream::StreamID::Hasher>mAckedClosingStreams;
#define ThreadSafeStack ThreadSafeQueue //FIXME this can be way more efficient
    ///actually free stream IDs
    ThreadSafeStack<Stream::StreamID>mFreeStreamIDs;
#undef ThreadSafeStack
    CallbackMap mCallbacks;
///a pair of the number sockets - num positive checks (or -n for n sockets of which at least 1 failed) and an example header that passes if the first int is >1
    typedef std::pair<std::pair<int,UUID>,Array<uint8,TcpSstHeaderSize> > HeaderCheck;
    Stream::StreamID getNewID() {
        if (!mFreeStreamIDs.probablyEmpty()) {
            Stream::StreamID retval;
            if (mFreeStreamIDs.pop(retval))
                return retval;
        }
        unsigned int retval=mHighestStreamID+=2;
        assert(retval>1);
        return Stream::StreamID(retval);
    }
    MultiplexedSocket(IOService*io):mResolver(*io),mHighestStreamID(1) {
        mSocketConnectionPhase=PRECONNECTION;
    }
    MultiplexedSocket(const UUID&uuid,const std::vector<TCPSocket*>&sockets)
        : mResolver(sockets[0]->io_service()),
          mHighestStreamID(0) {
        mSocketConnectionPhase=PRECONNECTION;
        for (unsigned int i=0;i<(unsigned int)sockets.size();++i) {
            mSockets.push_back(ASIOSocketWrapper(sockets[i]));
        }
    }
    static void sendAllProtocolHeaders(const std::tr1::shared_ptr<MultiplexedSocket>&thus,const UUID&syncedUUID) {
        unsigned int numSockets=(unsigned int)thus->mSockets.size();
        for (std::vector<ASIOSocketWrapper>::iterator i=thus->mSockets.begin(),ie=thus->mSockets.end();i!=ie;++i) {
            i->sendProtocolHeader(thus,syncedUUID,numSockets);
        }
        boost::lock_guard<boost::mutex> connectingMutex(sConnectingMutex);
        thus->mSocketConnectionPhase=CONNECTED;
        for (unsigned int i=0,ie=thus->mSockets.size();i!=ie;++i) {
            new TCPReadBuffer(thus,i);
        }
    }
    ///erase all sockets and callbacks since the refcount is now zero;
    ~MultiplexedSocket() {
        for (unsigned int i=0;i<(unsigned int)mSockets.size();++i){
            delete mSockets[i].mSocket;
        }
        boost::lock_guard<boost::mutex> connecting_mutex(sConnectingMutex);
        while (!mCallbackRegistration.empty()){
            delete mCallbackRegistration.front().mCallback;
            mCallbackRegistration.pop_front();
        }
        while (!mNewRequests.empty()) {
            delete mNewRequests.front().data;
            mNewRequests.pop_front();
        }
        while(!mCallbacks.empty()) {
            delete mCallbacks.begin()->second;
            mCallbacks.erase(mCallbacks.begin());
        }
    }
    void receiveFullChunk(unsigned int whichSocket, Stream::StreamID id,const Chunk&newChunk){
        if (id==Stream::StreamID()) {//control packet
            if(newChunk.size()) {
                unsigned int controlCode=*newChunk.begin();
                switch (controlCode) {
                  case TCPStreamCloseStream:
                  case TCPStreamAckCloseStream:
                    if (newChunk.size()>1) {
                        unsigned int avail_len=newChunk.size()-1;
                        id.unserialize((const uint8*)&(newChunk[1]),avail_len);
                        if (avail_len>=newChunk.size()-1) {
                            std::cerr<<"Control Chunk too short\n";
                        }
                    }
                    if (id!=Stream::StreamID()) {
                        std::tr1::unordered_map<Stream::StreamID,unsigned int>::iterator where=mAckedClosingStreams.find(id);
                        if (where!=mAckedClosingStreams.end()){
                            where->second++;
                            if (where->second==mSockets.size()*2) {
                                mAckedClosingStreams.erase(where);
                                if (id.odd()==((mHighestStreamID.read()&1)?true:false)) {
                                    mFreeStreamIDs.push(id);
                                }
                            }
                        }else{
                            if (mSockets.size()*2/*impossible*/==1) {
                                if (id.odd()==((mHighestStreamID.read()&1)?true:false)) {
                                    mFreeStreamIDs.push(id);
                                }
                            }else {
                                mAckedClosingStreams[id]=1;
                            }
                            if (controlCode==TCPStreamCloseStream){
                                std::deque<StreamIDCallbackPair> registrations;
                                CommitCallbacks(registrations,CONNECTED,false);
                                CallbackMap::iterator where=mCallbacks.find(id);
                                if (where!=mCallbacks.end()) {
                                    where->second->mConnectionCallback(Stream::Disconnected,"Remote Host Disconnected");
                                }
                            }
                        }
                    }
                    break;
                  default:
                    break;
                }
                switch (controlCode) {
                  case TCPStreamCloseStream:
                    if (id!=Stream::StreamID())
                        mSockets[whichSocket].sendControlPacket(getSharedPtr(),TCPStreamAckCloseStream,id);
                    break;
                  case TCPStreamAckCloseStream:
                    break;
                  default:
                    break;
                }
            }
        }else {
            std::deque<StreamIDCallbackPair> registrations;
            CommitCallbacks(registrations,CONNECTED,false);
            CallbackMap::iterator where=mCallbacks.find(id);
            if (where!=mCallbacks.end()) {
                where->second->mBytesReceivedCallback(newChunk);
            }else {
                //new substream
                //FIXME dont know which callback to call for substream creation
                where=mCallbacks.find(Stream::StreamID(1));
                if (where==mCallbacks.end())
                    where=mCallbacks.find(Stream::StreamID(2));
                if (where==mCallbacks.end()&&!mCallbacks.empty()) {
                    where=mCallbacks.begin();
                }
                if (where!=mCallbacks.end()) {
                    TCPStream*newStream=new TCPStream(getSharedPtr());
                    newStream->mID=id;
                    TCPSetCallbacks setCallbackFunctor(this,newStream);
                    where->second->mSubstreamCallback(newStream,setCallbackFunctor);
                    if (setCallbackFunctor.mCallbacks != NULL) {
                        setCallbackFunctor.mCallbacks->mBytesReceivedCallback(newChunk);
                    }else {
                        closeStream(getSharedPtr(),id);
                    }
                }
            }
        }
    }
    /**
     * Calls the connected callback with the succeess or failure status. Sets status while holding the sConnectingMutex lock so that after that point no more Connected responses
     * will be sent out. Then inserts the registrations into the mCallbacks map during the ioReactor thread.
     */
    void connectionFailureOrSuccessCallback(SocketConnectionPhase status, const std::string&error_code=std::string()) {
        Stream::ConnectionStatus stat=(status==CONNECTED?Stream::Connected:Stream::Disconnected);
        std::deque<StreamIDCallbackPair> registrations;
        bool actuallyDoSend=CommitCallbacks(registrations,status,true);
        if (actuallyDoSend) {
            for (CallbackMap::iterator i=mCallbacks.begin(),ie=mCallbacks.end();i!=ie;++i) {
                i->second->mConnectionCallback(stat,error_code);
            }
        }else {
            std::cerr<< "Did not call callbacks because callback message already sent for "<<error_code<<'\n';
        }
    }
   /**
    * The connection failed before any sockets were established (or as a helper function after they have been cleaned)
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(const std::string& error) {

        connectionFailureOrSuccessCallback(DISCONNECTED,error);
    }
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(unsigned int whichSocket, const std::string& error) {
        connectionFailedCallback(error);
        //FIXME do something with the socket specifically that failed.
    }
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(unsigned int whichSocket, const boost::system::error_code& error) {
        connectionFailedCallback(whichSocket,error.message());
    }
   /**
    * The a particular socket's connection failed
    * This function will call all substreams disconnected methods
    */
    void connectionFailedCallback(const ASIOSocketWrapper* whichSocket, const boost::system::error_code &error) {
        unsigned int which=0;
        for (std::vector<ASIOSocketWrapper>::iterator i=mSockets.begin(),ie=mSockets.end();i!=ie;++i,++which) {
            if (&*i==whichSocket)
                break;
        }
        connectionFailedCallback(which==mSockets.size()?0:which,error.message());
    }
   /**
    * The a particular established a connection:
    * This function will call all substreams connected methods
    */
    void connectedCallback() {
        connectionFailureOrSuccessCallback(CONNECTED);
    }
   /**
    * This function checks a particular sockets initial handshake header.
    * If this is the first header read, it will save it for comparison
    * If this is the nth header read and everythign is successful it will decrement the first header check integer
    * If anything goes wrong and it is the first time, it will decrement the first header check integer below zero to indiate error and call connectionFailed
    * If anything goes wrong and the first header check integer is already below zero it will decline to take action
    * The buffer passed in will be deleted by this function
    */
    void checkHeader(unsigned int whichSocket,const std::tr1::shared_ptr<HeaderCheck>&headerCheck, Array<uint8,TcpSstHeaderSize>* buffer, const boost::system::error_code &error, std::size_t bytes_received) {

        if (headerCheck->first.first==(int)mSockets.size()) {
            headerCheck->second=*buffer;
            headerCheck->first.first--;
        }else if (headerCheck->first.first>=1) {
            if (headerCheck->second!=*buffer) {
                connectionFailedCallback(whichSocket,"Bad header comparison "
                                         +std::string((char*)buffer->begin(),TcpSstHeaderSize)
                                         +" does not match "
                                         +std::string((char*)headerCheck->second.begin(),TcpSstHeaderSize));
                headerCheck->first.first-=mSockets.size();
                headerCheck->first.first-=1;
            }else {
                headerCheck->first.first--;
                if (headerCheck->first.first==0) {
                    connectedCallback();
                }
                new TCPReadBuffer(getSharedPtr(),whichSocket);
            }
        }else {
            headerCheck->first.first-=1;
        }
        delete buffer;

    }
   /**
    * This function checks if a particular sockets has connected to its destination IP address
    * If everything is successful it will decrement the first header check integer
    * If the last resolver fails and it is the first time, it will decrement the first header check integer below zero to indiate error and call connectionFailed
    * If anything goes wrong and the first header check integer is already below zero it will decline to take action
    * The buffer passed in will be deleted by this function
    */

    void connectToIPAddress(unsigned int whichSocket,const std::tr1::shared_ptr<HeaderCheck>&headerCheck,tcp::resolver::iterator it,const boost::system::error_code &error) {
        if (error) {
            if (it == tcp::resolver::iterator()) {
                if (headerCheck->first.first>=1) {
                    headerCheck->first.first-=mSockets.size();
                    headerCheck->first.first-=1;
                    connectionFailedCallback(whichSocket,error);
                }else headerCheck->first.first-=1;
            }else {
                //need to use boost::bind instead of TR1::bind to remain compatible with boost::asio::placeholders
                mSockets[whichSocket].mSocket->async_connect(*it,
                                                             boost::bind(&MultiplexedSocket::connectToIPAddress,
                                                                         getSharedPtr(),
                                                                         whichSocket,
                                                                         headerCheck,
                                                                         ++it,
                                                                         boost::asio::placeholders::error));
            }
        }else {
            mSockets[whichSocket].mSocket->set_option(tcp::no_delay(true));
            mSockets[whichSocket].sendProtocolHeader(getSharedPtr(),headerCheck->first.second,headerCheck->first.first);
            Array<uint8,TcpSstHeaderSize> *header=new Array<uint8,TcpSstHeaderSize>;
            //need to use boost::bind instead of tr1::bind to remain compatible with boost::asio::placeholders
            boost::asio::async_read(*mSockets[whichSocket].mSocket,
                                    boost::asio::buffer(header->begin(),TcpSstHeaderSize),
                                    boost::asio::transfer_at_least(TcpSstHeaderSize),
                                    boost::bind(&MultiplexedSocket::checkHeader,
                                                getSharedPtr(),
                                                whichSocket,
                                                headerCheck,
                                                header,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
    }
    void handleResolve(const std::tr1::shared_ptr<HeaderCheck>&headerCheck,const boost::system::error_code &error, tcp::resolver::iterator it) {
        if (error) {
            connectionFailedCallback(error.message());
        }else {
            for (unsigned int whichSocket=0;whichSocket<(unsigned int)mSockets.size();++whichSocket) {
                mSockets[whichSocket].mSocket=new TCPSocket(getASIOService());
                connectToIPAddress(whichSocket,headerCheck,it,boost::asio::error::host_not_found);
            }
        }

    }
    void connect(const Address&address, unsigned int numSockets){
        mSocketConnectionPhase=PRECONNECTION;
        mSockets.resize(numSockets);
        std::tr1::shared_ptr<HeaderCheck> headerCheck(new HeaderCheck(std::pair<int,UUID>(numSockets,UUID::random()),Array<uint8,TcpSstHeaderSize>()));

        tcp::resolver::query query(tcp::v4(), address.getHostName(), atoi(address.getService().c_str()));
        mResolver.async_resolve(query,
                                boost::bind(&MultiplexedSocket::handleResolve, getSharedPtr(),
                                            headerCheck,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::iterator));

    }
    TCPSocket&getSocket(unsigned int whichSocket){
        return mSockets[whichSocket].getSocket();
    }
    const TCPSocket&getSocket(unsigned int whichSocket)const{
        return mSockets[whichSocket].getSocket();
    }
};
boost::mutex MultiplexedSocket::sConnectingMutex;
void TCPSetCallbacks::operator()(const Stream::ConnectionCallback &connectionCallback,
                                         const Stream::SubstreamCallback &substreamCallback,
                                         const Stream::BytesReceivedCallback &bytesReceivedCallback){
    mCallbacks=new TCPStream::Callbacks(connectionCallback,
                                        substreamCallback,
                                        bytesReceivedCallback);
    mMultiSocket->addCallbacks(mStream->getID(),mCallbacks);
}

void TCPReadBuffer::processError(MultiplexedSocket*parentSocket, const boost::system::error_code &error){
    parentSocket->connectionFailedCallback(mWhichBuffer,error);
}
void TCPReadBuffer::processFullChunk(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket, unsigned int whichSocket, const Stream::StreamID&id, const Chunk&newChunk){
    parentSocket->receiveFullChunk(whichSocket,id,newChunk);
}
void TCPReadBuffer::readIntoFixedBuffer(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket){
    parentSocket
        ->getSocket(mWhichBuffer)
        .async_receive(boost::asio::buffer(mBuffer,mBufferLength),
                       *this);
}
void TCPReadBuffer::readIntoChunk(const std::tr1::shared_ptr<MultiplexedSocket> &parentSocket){
    assert(mNewChunk.size()>0);//otherwise should have been filtered out by caller
    parentSocket
        ->getSocket(mWhichBuffer)
        .async_receive(boost::asio::buffer(&*mNewChunk.begin(),mNewChunk.size()),
                       boost::bind(&TCPReadBuffer::asioReadIntoChunk,
                                   this,
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred));
}
void triggerMultiplexedConnectionError(MultiplexedSocket*socket,ASIOSocketWrapper*wrapper,const boost::system::error_code &error){
    socket->connectionFailedCallback(wrapper,error);
}
TCPStream::TCPStream(const std::tr1::shared_ptr<MultiplexedSocket>&shared_socket):mSocket(shared_socket),mSendStatus(0) {
}
class TCPStreamBuilder{
public:
    class IncompleteStreamState {
    public:
        int mNumSockets;
        std::vector<TCPSocket*>mSockets;
    };
    typedef std::map<UUID,IncompleteStreamState> IncompleteStreamMap;
    static std::deque<UUID> sStaleUUIDs;
    static IncompleteStreamMap sIncompleteStreams;
    unsigned char buffer[TcpSstHeaderSize];
    unsigned int mOffset;
    TCPSocket *mSocket;
    Stream::SubstreamCallback mCallback;
    TCPStreamBuilder(TCPSocket *socket, const Stream::SubstreamCallback& cb):mSocket(socket),mCallback(cb){
        mOffset=0;
    }
    ///gets called when a complete 24 byte header is actually received: uses the UUID within to match up appropriate sockets
    void operator() (const boost::system::error_code &error,
                     std::size_t bytes_transferred) {
        //FIXME: cleanup stale UUIDs
        mOffset+=(unsigned int)bytes_transferred;
        assert ((unsigned int)mOffset<=(unsigned int)TcpSstHeaderSize);
        if (error || std::memcmp(buffer,SIRIKATA_TCP_STREAM_HEADER,SIRIKATA_TCP_STREAM_HEADER_LENGTH)!=0) {
            std::cerr<< "Connection received with incomprehensible header";
            delete this;
        }else {
            UUID context=UUID(&buffer[TcpSstHeaderSize-16],16);
            IncompleteStreamMap::iterator where=sIncompleteStreams.find(context);
            unsigned int numConnections=((buffer[SIRIKATA_TCP_STREAM_HEADER_LENGTH]-'0')%10)*10+((buffer[SIRIKATA_TCP_STREAM_HEADER_LENGTH+1]-'0')%10);
            if (numConnections>99) numConnections=99;//FIXME: some option in options
            if (where==sIncompleteStreams.end()){
                sIncompleteStreams[context].mNumSockets=numConnections;
                where=sIncompleteStreams.find(context);
                assert(where!=sIncompleteStreams.end());
            }
            if ((int)numConnections!=where->second.mNumSockets) {
                std::cerr<< "Single client disagrees on number of connections to establish";
                sIncompleteStreams.erase(where);
            }else {
                where->second.mSockets.push_back(mSocket);
                if (numConnections==(unsigned int)where->second.mSockets.size()) {
                    std::tr1::shared_ptr<MultiplexedSocket> shared_socket(MultiplexedSocket::construct(context,where->second.mSockets));
                    MultiplexedSocket::sendAllProtocolHeaders(shared_socket,UUID::random());
                    sIncompleteStreams.erase(where);

                    TCPStream * strm=new TCPStream(shared_socket);
                    Stream::StreamID newID=strm->mID=Stream::StreamID(1);
                    TCPSetCallbacks setCallbackFunctor(&*shared_socket,strm);
                    mCallback(strm,setCallbackFunctor);
                    if (setCallbackFunctor.mCallbacks==NULL) {
                        std::cerr<<"Forgot to set listener on socket\n";
                        shared_socket->closeStream(shared_socket,newID);
                    }
                }else{
                    sStaleUUIDs.push_back(context);
                }
            }
            delete this;
        }
    }
};
void TCPStream::send(const Chunk&data, Stream::Reliability reliability) {
    RawRequest toBeSent;
    switch(reliability) {
      case Unreliable:
        toBeSent.unordered=true;
        toBeSent.unreliable=true;
        break;
      case Reliable:
        toBeSent.unordered=false;
        toBeSent.unreliable=false;
        break;
      case ReliableUnordered:
        toBeSent.unordered=true;
        toBeSent.unreliable=false;
        break;
    }
    toBeSent.originStream=getID();
    uint8 serializedStreamId[8];
    unsigned int streamIdLength=8;
    bool success=toBeSent.originStream.serialize(serializedStreamId,streamIdLength);
    assert(success);
    size_t totalSize=data.size();
    totalSize+=streamIdLength;
    toBeSent.data=new Chunk(totalSize+4);
    uint8 length[4]={(totalSize/256/256/256)%256,
                     (totalSize/256/256)%256,
                     (totalSize/256)%256,
                     (totalSize)%256};
    uint8 *outputBuffer=&(*toBeSent.data)[0];
    std::memcpy(outputBuffer,length,4);
    std::memcpy(outputBuffer+4,serializedStreamId,streamIdLength);
    if (data.size())
        std::memcpy(&outputBuffer[4+streamIdLength],
                    &data[0],
                    data.size());
    unsigned int sendStatus=++mSendStatus;
    if ((sendStatus&SendStatusClosing)==0) {
        MultiplexedSocket::sendBytes(mSocket,toBeSent);
    }
    --mSendStatus;
}
void TCPStream::close() {
    mSendStatus+=SendStatusClosing;
    while (mSendStatus.read()!=SendStatusClosing)
        ;
    mSocket->addCallbacks(getID(),NULL);
    MultiplexedSocket::closeStream(mSocket,getID());
}
TCPStreamBuilder::IncompleteStreamMap TCPStreamBuilder::sIncompleteStreams;
std::deque<UUID> TCPStreamBuilder::sStaleUUIDs;
TCPStream::TCPStream(IOService&io):mSocket(MultiplexedSocket::construct(&io)),mSendStatus(0) {
}
void TCPStream::connect(const Address&addy,
                        const ConnectionCallback &connectionCallback,
                        const SubstreamCallback &substreamCallback,
                        const BytesReceivedCallback&bytesReceivedCallback) {
    mSendStatus=0;
    mID=StreamID(1);
    mSocket->addCallbacks(getID(),new Callbacks(connectionCallback,
                                                substreamCallback,
                                                bytesReceivedCallback));
    mSocket->connect(addy,3);
}
Stream* TCPStream::factory() {
    return new TCPStream(mSocket->getASIOService());
}
bool TCPStream::cloneFrom(Stream*otherStream,
                          const ConnectionCallback &connectionCallback,
                          const SubstreamCallback &substreamCallback,
                          const BytesReceivedCallback&bytesReceivedCallback) {
    TCPStream * toBeCloned=dynamic_cast<TCPStream*>(otherStream);
    if (NULL==toBeCloned)
        return false;
    mSocket=toBeCloned->mSocket;
    StreamID newID=mSocket->getNewID();
    mID=newID;
    mSocket->addCallbacks(newID,new Callbacks(connectionCallback,
                                              substreamCallback,
                                              bytesReceivedCallback));
    return true;
}

void beginNewStream(TCPSocket * socket, const Stream::SubstreamCallback& cb) {
    TCPStreamBuilder * tsb=new TCPStreamBuilder(socket,cb);
    boost::asio::async_read(*socket,
                            boost::asio::buffer(&tsb->buffer[0],TcpSstHeaderSize),
                            boost::asio::transfer_at_least(TcpSstHeaderSize),
                            *tsb);
}

}  }
