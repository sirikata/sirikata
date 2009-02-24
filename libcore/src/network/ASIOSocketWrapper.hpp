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


namespace Sirikata { namespace Network {
class ASIOSocketWrapper;

void triggerMultiplexedConnectionError(MultiplexedSocket*,ASIOSocketWrapper*,const boost::system::error_code &error);

class ASIOSocketWrapper {
#define ASIOSocketWrapperBuffer(pointer,size) boost::asio::buffer(pointer,(size))
    TCPSocket*mSocket;
    /**
     * The status of sending threads: odd number indicates asynchronous send... number >0  indicates other waiting threads number&ASYNCHRONOUS_SEND_FLAG indicates a thread is
     * currently sending data off. number&QUEUE_CHECK_FLAG means queue is currently being swapped away and items added to the queue may not ever be processed if the queue is empty
     * unless a thread takes up the torch and does it
     */
    AtomicValue<uint32> mSendingStatus;
    ThreadSafeQueue<Chunk*>mSendQueue;
	enum {
		ASYNCHRONOUS_SEND_FLAG=(1<<29),
		QUEUE_CHECK_FLAG=(1<<30),
		PACKET_BUFFER_SIZE=1400
	};
    uint8 mBuffer[PACKET_BUFFER_SIZE];
    std::vector<uint8> mPacketLogger;
    typedef boost::system::error_code ErrorCode;
/**
 * This function sets the QUEUE_CHECK_FLAG and checks the sendQueue for additional packets to send out.
 * If nothing is in the queue then it unsets the ASYNCHRONOUS_SEND_FLAG and QUEUE_CHECK_FLAGS
 * If something is present in the queue it calls sendToWire with the queue
 */
    void finishAsyncSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket) {
        assert(mSendingStatus.read()&ASYNCHRONOUS_SEND_FLAG);
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
    void sendLargeChunkItem(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk *toSend, size_t originalOffset, const ErrorCode &error, std::size_t bytes_sent) {
#ifdef TCPSSTLOG
        if (!error) {
            char TESTs[1024];
            sprintf(TESTs,"%d.sack",(int)(size_t)this);
            FILE * fp=fopen(TESTs,"a");
            fwrite(&*toSend->begin()+originalOffset,bytes_sent,1,fp);
            fclose(fp);
        }
#endif
        if (!error) {
            //mPacketLogger.insert(mPacketLogger.end(),test.begin(),test.end());
        }
        if (error)  {
            triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
            std::cerr<<"Socket disconnected...waiting for recv to trigger error condition\n";
        }else if (bytes_sent+originalOffset!=toSend->size()) {
            sendToWire(parentMultiSocket,toSend,originalOffset+bytes_sent);
        }else {
            delete toSend;
            finishAsyncSend(parentMultiSocket);
        }
    }

    /**
     * The callback for when a single large Chunk at the front of a chunk deque was sent.
     * If the whole large Chunk was not sent then the rest of the Chunk is passed back to sendToWire
     * If the whole Chunk was shipped off, the sendToWire function is called with the rest of the queue unless it is empty in which case the finishAsyncSend is called
     */
    void sendLargeDequeItem(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*> &const_toSend, size_t originalOffset, const ErrorCode &error, std::size_t bytes_sent) {
#ifdef TCPSSTLOG
        if (!error) {
            char TESTs[1024];
            sprintf(TESTs,"%d.sack",(int)(size_t)this);
            FILE * fp=fopen(TESTs,"a");
            fwrite(&*const_toSend.front()->begin()+originalOffset,bytes_sent,1,fp);
            fclose(fp);
        }
#endif
        if (!error) {
            //mPacketLogger.insert(mPacketLogger.end(),*const_toSend.front()->begin()+originalOffset,*const_toSend.front()->begin()+originalOffset+bytes_sent);
        }
        if (error )   {
            triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
            std::cerr<<"Socket disconnected...waiting for recv to trigger error condition\n";
        } else if (bytes_sent+originalOffset!=const_toSend.front()->size()) {
            sendToWire(parentMultiSocket,const_toSend,originalOffset+bytes_sent);
        }else if (const_toSend.size()<2) {
            delete const_toSend.front();
            finishAsyncSend(parentMultiSocket);
        }else {
            std::deque<Chunk*> toSend=const_toSend;
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
    void sendStaticBuffer(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const std::deque<Chunk*>&toSend, uint8* currentBuffer, size_t bufferSize, size_t lastChunkOffset,  const ErrorCode &error, std::size_t bytes_sent) {
#ifdef TCPSSTLOG
            char TESTs[1024];
            sprintf(TESTs,"%d.sack",(int)(size_t)this);
            FILE * fp=fopen(TESTs,"a");
            fwrite(currentBuffer,bytes_sent,1,fp);
            fclose(fp);
#endif
        if (!error) {
            //mPacketLogger.insert(mPacketLogger.end(),currentBuffer,currentBuffer+bytes_sent);
        }
        if ( error ) {
            triggerMultiplexedConnectionError(&*parentMultiSocket,this,error);
            std::cerr<<"Socket disconnected...waiting for recv to trigger error condition\n";
        }else if (bytes_sent!=bufferSize) {
            mSocket->async_send(ASIOSocketWrapperBuffer(currentBuffer+bytes_sent,bufferSize-bytes_sent),
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

            mSocket->async_send(ASIOSocketWrapperBuffer(&*toSend->begin()+bytesSent,toSend->size()-bytesSent),
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
            mSocket->async_send(ASIOSocketWrapperBuffer(&*const_toSend.front()->begin()+bytesSent,const_toSend.front()->size()-bytesSent),
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
            mSocket->async_send(ASIOSocketWrapperBuffer(mBuffer,bufferLocation),
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
   while(queue_check){
                     static bool epic_fail=true;
                     if (epic_fail) {
                         epic_fail=false;
                         std::cerr<<"FAILURE of queue check "<< current_status<<'\n';
                     }
                 }

                assert(queue_check==false);//no one should check the queue without being willing to send
                current_status=++mSendingStatus;
                if (current_status==1) {
                    mSendingStatus+=(QUEUE_CHECK_FLAG+ASYNCHRONOUS_SEND_FLAG-1);
                    std::deque<Chunk*>toSend;
                    mSendQueue.swap(toSend);
                    if (toSend.empty()) {//the chunk that we put on the queue must have been sent by someone else
                        mSendingStatus-=(QUEUE_CHECK_FLAG+ASYNCHRONOUS_SEND_FLAG);
                        return;
                    }else {//the chunk may be on this queue, but we should promise folks to send it
                        assert(mSendingStatus.read()&ASYNCHRONOUS_SEND_FLAG);
                        mSendingStatus-=QUEUE_CHECK_FLAG;
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

    ASIOSocketWrapper(TCPSocket* socket) :mSocket(socket),mSendingStatus(0){
        //mPacketLogger.reserve(268435456);
    }

    ASIOSocketWrapper(const ASIOSocketWrapper& socket) :mSocket(socket.mSocket),mSendingStatus(0){
        //mPacketLogger.reserve(268435456);
    }

    ASIOSocketWrapper&operator=(const ASIOSocketWrapper& socket){
        mSocket=socket.mSocket;
        return *this;
    }

    ASIOSocketWrapper() :mSocket(NULL),mSendingStatus(0){
    }

    TCPSocket&getSocket() {return *mSocket;}

    const TCPSocket&getSocket()const {return *mSocket;}

    void shutdownAndClose();

    void createSocket(IOService&io);

    void destroySocket();
    /**
     * Sends the exact bytes contained within the typedeffed vector
     * \param chunk is the exact bytes to put on the network (including streamID and framing data)
     */
    void rawSend(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, Chunk * chunk);

    static Chunk*constructControlPacket(TCPStream::TCPStreamControlCodes code,const Stream::StreamID&sid);
/**
 *  Sends a streamID #0 packet with further control data on it. To start with only stream disconnect and the ack thereof are allowed
 */
    void sendControlPacket(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, TCPStream::TCPStreamControlCodes code,const Stream::StreamID&sid) {
        rawSend(parentMultiSocket,constructControlPacket(code,sid));
    }
/**
 * Sends 24 byte header that indicates version of SST, a unique ID and how many TCP connections should be established
 */
    void sendProtocolHeader(const std::tr1::shared_ptr<MultiplexedSocket>&parentMultiSocket, const UUID&value, unsigned int numConnections);
#undef ASIOSocketWrapperBuffer
};
} }
