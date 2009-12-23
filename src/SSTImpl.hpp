#ifndef SST_IMPL_HPP
#define SST_IMPL_HPP

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>

#include "oh/ObjectHostContext.hpp"
#include "oh/ObjectHost.hpp"
#include "Utility.hpp"

#include <sirikata/util/SerializationCheck.hpp>



namespace CBR {
class ServerIDMap;



template <typename EndObjectType>
class EndPoint {
public:
  EndObjectType endPoint;
  uint16 port;

  EndPoint() {

  }

  EndPoint(EndObjectType endPoint, uint16 port) {
    this->endPoint = endPoint;
    this->port = port;
  }

  bool operator< (const EndPoint &ep) const{   
    if (endPoint != ep.endPoint) {
      return endPoint < ep.endPoint;
    }

    return this->port < ep.port ;
  }

};




template <typename EndPointType>
class BaseDatagramLayer
{

public:
  BaseDatagramLayer(const ObjectHostContext* ctx) : mContext(ctx) {}

  void send2(EndPoint<EndPointType> src, EndPoint<EndPointType> dest, String dataStr) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    mContext->objectHost->send(
	    src.port,src.endPoint,
            dest.port, dest.endPoint,
            dataStr
        );
  }
  
  /* This function will have to be re-implemented to support sending using
     other kinds of packets. For now, I'll implement only for ODP. */
  void send(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
    mContext->mainStrand->post(
			       std::tr1::bind(
				     &(BaseDatagramLayer<EndPointType>::send2),this,
				     *src, *dest, String((char*)data, len))
			      );
  }

  /* This function will also have to be re-implemented to support receiving
     using other kinds of packets. For now, I'll implement only for ODP. */
  void handleReceive(EndPoint<EndPointType> endPoint, void* data, int len);

private: 
  const ObjectHostContext* mContext;

  Sirikata::SerializationCheck mSerialization;

};

template <class EndPointType>
class Connection;

template <class EndPointType>
class Stream;

typedef std::tr1::function< void(boost::shared_ptr< Connection<UUID> > ) > ConnectionReturnCallbackFunction;

typedef std::tr1::function< void(boost::shared_ptr< Stream<UUID> >) >  StreamReturnCallbackFunction;
typedef std::tr1::function< void (int, void*) >  DatagramSendDoneCallback;

typedef std::tr1::function<void (uint8*, int) >  ReadDatagramCallback;

typedef std::tr1::function<void (uint8*, int) > ReadCallback;

typedef UUID USID;

typedef uint16 LSID;


class ChannelSegment{
public:

  const void* mBuffer;
  uint16 mBufferLength;
  uint64 mChannelSequenceNumber;
  uint64 mAckSequenceNumber;

  Time mTransmitTime;
  Time mAckTime;

  ChannelSegment(const void* data, int len, uint64 channelSeqNum, uint64 ackSequenceNum) : 
                                              mBuffer(data), mBufferLength(len),
					      mChannelSequenceNumber(channelSeqNum), 
					      mAckSequenceNumber(ackSequenceNum),
					      mTransmitTime(Time::null()), mAckTime(Time::null())
  {
  }

  ChannelSegment(const void* data, int len, uint64 channelSeqNum, uint64 ackSeqNum,
		 Time& transmitTime)
    : mBuffer(data), mBufferLength(len), mChannelSequenceNumber(channelSeqNum),
      mAckSequenceNumber(ackSeqNum),
      mTransmitTime(transmitTime), mAckTime(Time::null())
  {
  }

  void setAckTime(Time& ackTime) {
    mAckTime = ackTime;
  }

  
};

template <class EndPointType>
class Connection {
private:
    typedef std::map<EndPoint<EndPointType>, boost::shared_ptr<Connection> >  ConnectionMap;
    typedef std::map<EndPoint<EndPointType>, ConnectionReturnCallbackFunction>
                                                        ConnectionReturnCallbackMap;

    static ConnectionMap mConnectionMap;
    static ConnectionReturnCallbackMap mConnectionReturnCallbackMap;
    
    ObjectHostContext* mContext;
    EndPoint<EndPointType> mLocalEndPoint;
    EndPoint<EndPointType> mRemoteEndPoint;

    BaseDatagramLayer<UUID> mDatagramLayer;

    int mState;
    uint8 mRemoteChannelID;
    uint8 mLocalChannelID;

    uint64 mTransmitSequenceNumber;
    uint64 mAckSequenceNumber;   //the last transmit sequence number received from the other side

   
    std::map<LSID, boost::shared_ptr< Stream<EndPointType> > > mOutgoingSubstreamMap;
    std::map<LSID, boost::shared_ptr< Stream<EndPointType> > > mIncomingSubstreamMap;

    uint16 mNumStreams;

    std::deque< boost::shared_ptr<ChannelSegment> > mQueuedSegments;
  
    std::deque< boost::shared_ptr<ChannelSegment> > mOutstandingSegments;

    uint16 mCwnd;
   
    uint64 mRTO; // RTO in microseconds
    bool mFirstRTO;

    bool mLooping;
    Thread* mThread;

    boost::mutex mMutex;

    Connection(const ObjectHostContext* ctx, EndPoint<EndPointType> localEndPoint, 
             EndPoint<EndPointType> remoteEndPoint)
    : mLocalEndPoint(localEndPoint), mRemoteEndPoint(remoteEndPoint),
      mDatagramLayer(ctx), mState(CONNECTION_DISCONNECTED),
      mRemoteChannelID(0), mLocalChannelID(1), mTransmitSequenceNumber(1), 
      mAckSequenceNumber(1), mNumStreams(0), mCwnd(1), mRTO(20000), mFirstRTO(true),
      mLooping(true)
   {
     mThread = new Thread(boost::bind(&(Connection<EndPointType>::dataSendingLoop), this));
   }

public:

  ~Connection() {
    mLooping = false;
    mThread->join();

    delete mThread;

    printf("Connection getting destroyed\n");
    fflush(stdout);
  }

  
  enum ConnectionStates {
       CONNECTION_DISCONNECTED = 1,      // no network connectivity for this connection. 
                              // It has either never been connected or has 
                              // been fully disconnected.
       CONNECTION_PENDING_CONNECT = 2,   // this connection is in the process of setting
                              // up a connection. The connection setup will be
                              // complete (or fail with an error) when the 
                              // application-specified callback is invoked.
       CONNECTION_PENDING_RECEIVE_CONNECT = 3,// connection received an initial 
                                              // channel negotiation request, but the 
                                              // negotiation has not completed yet.

       CONNECTION_CONNECTED=4,           // The connection is connected to a remote end 
                              // point.
       CONNECTION_PENDING_DISCONNECT=5,  // The connection is in the process of 
                              // disconnecting from the remote end point.

  };


  void dataSendingLoop() {
    

    
    // should start from ssthresh, the slow start lower threshold, but starting
    // from 1 for now. Still need to implement slow start.
    while (mLooping) {
      uint16 numSegmentsSent = 0;

      {
	boost::mutex::scoped_lock l(mMutex);

	if (mQueuedSegments.empty() ) continue;
	
	printf("%s has window size = %d, queued_segments.size=%d\n", mLocalEndPoint.endPoint.readableHexData().c_str(), mCwnd, (int)mQueuedSegments.size());
	
	for (int i = 0; i < mCwnd; i++) {
	  if ( !mQueuedSegments.empty() ) {
	    boost::shared_ptr<ChannelSegment> segment = mQueuedSegments.front();
	    
	    CBR::Protocol::SST::SSTChannelHeader sstMsg;
	    sstMsg.set_channel_id( mRemoteChannelID );
	    sstMsg.set_transmit_sequence_number(segment->mChannelSequenceNumber);
	    sstMsg.set_ack_count(1);
	    sstMsg.set_ack_sequence_number(segment->mAckSequenceNumber);
	    
	    sstMsg.set_payload(segment->mBuffer, segment->mBufferLength);
	    
	    std::string buffer = serializePBJMessage(sstMsg);
	    mDatagramLayer.send(&mLocalEndPoint, &mRemoteEndPoint, (void*) buffer.data(),
				buffer.size());
	    
	    printf("%s sending packet from data sending loop\n", mLocalEndPoint.endPoint.readableHexData().c_str());
	    
	    segment->mTransmitTime = Timer::now();
	    mOutstandingSegments.push_back(segment);
	    
	    mQueuedSegments.pop_front();
	    
	    numSegmentsSent++;
	  }
	  else {
	    break;
	  }
	}	
      }

      boost::this_thread::sleep( boost::posix_time::microseconds(mRTO*2) );
      
      printf("%s mOutstandingSegments.size()=%d\n", mLocalEndPoint.endPoint.toString().c_str() , mOutstandingSegments.size() );

      bool all_sent_packets_acked = true;
      bool no_packets_acked = true;
      for (uint i=0; i < mOutstandingSegments.size(); i++) {
	boost::shared_ptr<ChannelSegment> segment = mOutstandingSegments[i];
	
	if (mOutstandingSegments[i]->mAckTime == Time::null()) {
	  all_sent_packets_acked = false;
	}
	else {
          no_packets_acked = false;
	  if (mFirstRTO ) {
	    mRTO = ((segment->mAckTime - segment->mTransmitTime).toMicroseconds());
            mFirstRTO = false;
	  }
	  else {
	    mRTO = 0.8 * mRTO + 0.2 * (segment->mAckTime - segment->mTransmitTime).toMicroseconds();
	  }
	}
      }
   

      printf("mRTO=%d\n", (int) mRTO);

      if (numSegmentsSent >= mCwnd) {
        if (all_sent_packets_acked) {
          mCwnd += 1;
        }
	else {
	  mCwnd /= 2;
	}
      }
      else {
        if (all_sent_packets_acked) {
	  mCwnd = (mCwnd + numSegmentsSent) / 2;
	}
	else {
	  mCwnd /= 2;
	}
      }

      if (mCwnd < 1) {
        mCwnd = 1;
      }


      mOutstandingSegments.clear();
    }



  /* 
     Need to keep track of which packets got acked at the Channel layer.
     Channel is not going to do any retransmissions. 
     
     Solution: Keep two lists: a) The packets in the queue waiting to be sent.
                               b) The packets that have been sent. For each
			          packet, also store TransmitTime and AckTime.

	       On getting ack, go to list (b), and mark the packet as acked.
	       To check if all packets acked, just check all packets in list
	       (b). For RTO, update RTO over all the packets that 
	       were acked.

  */
  }

  /* Creates a connection for the application to a remote 
     endpoint. The EndPoint argument specifies the location of the remote
     endpoint. It is templatized to enable it to refer to either IP 
     addresses and ports, or object identifiers. The 
     ConnectionReturnCallbackFunction returns a reference-counted, shared-
     pointer of the Connection that was created. The constructor may or 
     may not actually synchronize with the remote endpoint. Instead the 
     synchronization may be done when the first stream is created.

     @EndPoint A templatized argument specifying the remote end-point to 
               which this connection is connected.

     @ConnectionReturnCallbackFunction A callback function which will be
               called once the connection is created and will provide  a
               reference-counted, shared-pointer to the  connection. 
               ConnectionReturnCallbackFunction should have the signature
               void (boost::shared_ptr<Connection>). If the boost::shared_ptr argument
               is NULL, the connection setup failed.

     @return false if it's not possible to create this connection, e.g. if another connection
     is already using the same local endpoint; true otherwise.
  */
  static bool createConnection(const ObjectHostContext* ctx, 
			       EndPoint <EndPointType> localEndPoint, 
			       EndPoint <EndPointType> remoteEndPoint, 
                               ConnectionReturnCallbackFunction cb) 
  {
    if (mConnectionMap.find(localEndPoint) != mConnectionMap.end()) {
      return false;
    }
    
    boost::shared_ptr<Connection>  conn =  boost::shared_ptr<Connection>(
						           new Connection(ctx, localEndPoint, remoteEndPoint));
    mConnectionMap[localEndPoint] = conn;

    conn->setState(CONNECTION_PENDING_CONNECT);    

    uint8* payload = new uint8[1];
    payload[0] = 1; //getAvailableChannel(); */    
    
    conn->setLocalChannelID(1);
    conn->sendData(payload, 1);

    delete payload;

    mConnectionReturnCallbackMap[remoteEndPoint] = cb;

    return true;
  }

  /* Creates a stream on top of this connection. The function also queues 
     up any initial data that needs to be sent on the stream. The function
     does not return a stream immediately since stream  creation might
     take some time and yet fail in the end. So the function returns without
     synchronizing with the remote host. Instead the callback function 
     provides a reference-counted,  shared-pointer to the stream.
     If this connection hasn't synchronized with the remote endpoint yet, 
     this function will also take care of doing that.
 
     @data A pointer to the initial data buffer that needs to be sent on
           this stream. Having this pointer removes the need for the 
           application to enqueue data until the stream is actually
           created.
    @port The length of the data buffer.
    @StreamReturnCallbackFunction A callback function which will be 
                                 called once the stream is created and
                                 the initial data queued up (or actually 
                                 sent?). The function will provide a 
                                 reference counted, shared pointer to the
                                 connection. StreamReturnCallbackFunction
                                 should have the signature void (boost::shared_ptr<Stream>).
                                 
  */
  virtual void stream(StreamReturnCallbackFunction cb, void* initial_data, int length) {
    USID usid = createNewUSID();
    LSID lsid = ++mNumStreams;
    boost::shared_ptr<Stream<EndPointType> > stream = 
      boost::shared_ptr<Stream<EndPointType> > 
      ( new Stream<EndPointType>(NULL, this, usid, lsid, initial_data, length, false, 0, cb) );

    mOutgoingSubstreamMap[lsid]=stream;
  }


  /* Sends the specified data buffer using best-effort datagrams on the 
     underlying connection. This may be done using an ephemeral stream 
     on top of the underlying connection or some other mechanism (e.g.
     datagram packets sent directly on the underlying  connection). 

     @param data the buffer to send
     @param length the length of the buffer
     @param DatagramSendDoneCallback a callback of type 
                                     void (int errCode, void*)
                                     which is called when sending
                                     the datagram failed or succeeded.
                                     'errCode' contains SUCCESS or FAILURE
                                     while the 'void*' argument is a pointer
                                     to the buffer that couldn't be sent.
  */
  virtual void datagram( void* data, int length, DatagramSendDoneCallback cb) {
    

    if (cb != NULL) {
      //invoke the callback function
    }
  }

  /*
    Register a callback which will be called when there is a datagram
    available to be read.

    @param ReadDatagramCallback a function of type "void (uint8*, int)"  
           which will be called when a datagram is available. The 
           "uint8*" field will be filled up with the received datagram,
           while the 'int' field will contain its size.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadDatagramCallback( ReadDatagramCallback cb) {

    return true;
  }
 

  /*
    Register a callback which will be called when there is a new 
    datagram available to be read. In other words, datagrams we have
    seen previously will not trigger this callback.

    @param ReadDatagramCallback a function of type "void (uint8*, int)"  
           which will be called when a datagram is available. The 
           "uint8*" field will be filled up with the received datagram,
           while the 'int' field will contain its size.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadOrderedDatagramCallback( ReadDatagramCallback cb )  {

     return true;
  }

  /*  Closes the connection.
 
      @param force if true, the connection is closed forcibly and 
             immediately. Otherwise, the connection is closed 
             gracefully and all outstanding packets are sent and
             acknowledged. Note that even in the latter case,
             the function returns without synchronizing with the 
             remote end point.
  */
  virtual void close(bool force) {

  }

  
  /*
    Returns the local endpoint to which this connection is bound.

    @return the local endpoint.
  */
  virtual EndPoint <EndPointType> localEndPoint()  {
    
    return mLocalEndPoint;
  }

  /*
    Returns the remote endpoint to which this connection is connected.

    @return the remote endpoint.
  */
  virtual EndPoint <EndPointType> remoteEndPoint() {
    return mRemoteEndPoint;
  }

  static void handleReceive(const ObjectHostContext* ctx, EndPoint<EndPointType> remoteEndPoint, 
                            EndPoint<EndPointType> localEndPoint, void* recv_buffer, int len) 
  {
    char* data = (char*) recv_buffer;
    std::string str = std::string(data, len);

    CBR::Protocol::SST::SSTChannelHeader* received_msg = new CBR::Protocol::SST::SSTChannelHeader();
    bool parsed = parsePBJMessage(received_msg, str);

    uint8 channelID = received_msg->channel_id();

    if (mConnectionMap.find(localEndPoint) != mConnectionMap.end()) {
      if (channelID == 0) {
	/*Someone's already connected at this port. Either don't reply or 
	  send back a request rejected message. */

	std::cout << "Someone's already connected at this port on object " << localEndPoint.endPoint.toString() << "\n";
	return;
      }
      boost::shared_ptr<Connection<EndPointType> > conn = mConnectionMap[localEndPoint];

      conn->receiveMessage(data, len);
    }
    else if (channelID == 0) {
      /* it's a new channel request negotiation protocol
	 packet ; allocate a new channel.*/
      
      std::cout << "Received a new channel request\n";
            
      uint8* received_payload = (uint8*) received_msg->payload().data();
      
      uint8* payload = new uint8[1];
      payload[0] = 2; //getAvailableChannel();      

      boost::shared_ptr<Connection>  conn =  
                   boost::shared_ptr<Connection>(
				    new Connection(ctx, localEndPoint, remoteEndPoint));
      mConnectionMap[localEndPoint] = conn;

      conn->setLocalChannelID(2);      
      conn->setRemoteChannelID(received_payload[0]);
      conn->setState(CONNECTION_PENDING_RECEIVE_CONNECT);

      conn->sendData(payload, 1);

      delete payload;
    }

    delete received_msg;

    //receivingObj->handleReceive(remoteEndPoint.endPoint, remoteEndPoint.port, 
    //                            localEndPoint.endPoint,  buffer, len);
  }

  uint64 sendData(const void* data2, uint32 length) {
    /* We probably need a lock here on the mTransmitSequenceNumber.  */

    boost::mutex::scoped_lock l(mMutex);

    uint8* data = new uint8[length];
    memcpy(data, (uint8*) data2, length);
    
    CBR::Protocol::SST::SSTStreamHeader* stream_msg =
                       new CBR::Protocol::SST::SSTStreamHeader();

    std::string str = std::string( (char*)data, length);

    bool parsed = parsePBJMessage(stream_msg, str);
 
    if ( stream_msg->type() !=  stream_msg->ACK) { 
      mQueuedSegments.push_back( boost::shared_ptr<ChannelSegment>( 
                    new ChannelSegment(data, length, mTransmitSequenceNumber, mAckSequenceNumber) ) );
    }
    else {

      CBR::Protocol::SST::SSTChannelHeader sstMsg;
      sstMsg.set_channel_id( mRemoteChannelID );
      sstMsg.set_transmit_sequence_number(mTransmitSequenceNumber);
      sstMsg.set_ack_count(1);
      sstMsg.set_ack_sequence_number(mAckSequenceNumber);

      sstMsg.set_payload(data, length);

      std::string buffer = serializePBJMessage(sstMsg);
      mDatagramLayer.send(&mLocalEndPoint, &mRemoteEndPoint, (void*) buffer.data(),
                        buffer.size());
    }    

    mTransmitSequenceNumber++;

    return mTransmitSequenceNumber-1;
  }
  
private:
  void setState(int state) {
    this->mState = state;
  }

  void setLocalChannelID(int channelID) {
    this->mLocalChannelID = channelID;
  }

  void setRemoteChannelID(int channelID) {
    this->mRemoteChannelID = channelID;
  }

  USID createNewUSID() {
    uint8 raw_uuid[UUID::static_size];
    for(uint32 ui = 0; ui < UUID::static_size; ui++)
        raw_uuid[ui] = (uint8)rand() % 256;
    UUID id(raw_uuid, UUID::static_size);
    return id;
  }

  void receiveMessage(void* recv_buff, int len) {
    uint8* data = (uint8*) recv_buff;
    std::string str = std::string((char*) data, len);
    
    CBR::Protocol::SST::SSTChannelHeader* received_msg = 
                       new CBR::Protocol::SST::SSTChannelHeader();
    bool parsed = parsePBJMessage(received_msg, str);

    mAckSequenceNumber = received_msg->transmit_sequence_number();

    uint64 receivedAckNum = received_msg->ack_sequence_number();

    if (mOutstandingSegments.size() > 0){
      printf("%s mOutstandingSegments[0]->mChannelSequenceNumber=%d, receivedAckNum=%d\n", mLocalEndPoint.endPoint.toString().c_str(), (int)(mOutstandingSegments[0]->mChannelSequenceNumber), (int)(receivedAckNum) );
    }

    for (uint i = 0; i < mOutstandingSegments.size(); i++) {
      if (mOutstandingSegments[i]->mChannelSequenceNumber == receivedAckNum) {
        printf("Packet acked at %s\n", mLocalEndPoint.endPoint.toString().c_str());
	mOutstandingSegments[i]->mAckTime = Timer::now();
      }
    }

    if (mState == CONNECTION_PENDING_CONNECT) {
      mState = CONNECTION_CONNECTED;

      uint8* received_payload = (uint8*) received_msg->payload().data();

      this->setRemoteChannelID(received_payload[0]);
               
      sendData( received_payload, 0 );

      if (mConnectionReturnCallbackMap.find(mRemoteEndPoint) != mConnectionReturnCallbackMap.end()) 
      {
	if (mConnectionMap.find(mLocalEndPoint) != mConnectionMap.end()) {
	  boost::shared_ptr<Connection> conn = mConnectionMap[mLocalEndPoint];	

	  mConnectionReturnCallbackMap[mRemoteEndPoint] (conn);
	}
      }

      mConnectionReturnCallbackMap.erase(mRemoteEndPoint);
    }
    else if (mState == CONNECTION_PENDING_RECEIVE_CONNECT) {
      mState = CONNECTION_CONNECTED;      

      std::cout << "Receiving side connected!!!\n";
    }
    else if (mState == CONNECTION_CONNECTED) {
      parsePacket(recv_buff, len);
    }

    delete received_msg;
  }

  void parsePacket(void* recv_buff, uint32 len) {
    uint8* data = (uint8*) recv_buff;
    std::string str = std::string((char*) data, len);
    
    CBR::Protocol::SST::SSTChannelHeader* received_channel_msg = 
                       new CBR::Protocol::SST::SSTChannelHeader();
    bool parsed = parsePBJMessage(received_channel_msg, str);

    CBR::Protocol::SST::SSTStreamHeader* received_stream_msg = 
                       new CBR::Protocol::SST::SSTStreamHeader();
    parsed = parsePBJMessage(received_stream_msg, received_channel_msg->payload());

    LSID incomingLsid = received_stream_msg->lsid();
    if (received_stream_msg->type() == received_stream_msg->INIT) {
      std::cout << "INIT packet received\n";      
      
      if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
	uint8 buf[1];
	//create a new stream
	USID usid = createNewUSID();
	LSID newLSID = ++mNumStreams;
	boost::shared_ptr<Stream<EndPointType> > stream = 
	  boost::shared_ptr<Stream<EndPointType> > 
	      (new Stream<EndPointType>(NULL, this, usid, newLSID,
				        buf, 0, true, incomingLsid, NULL));
	mOutgoingSubstreamMap[newLSID] = stream;
        mIncomingSubstreamMap[incomingLsid] = stream;
      }
    }
    else if (received_stream_msg->type() == received_stream_msg->REPLY) {
      
      if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
        LSID initiatingLSID = received_stream_msg->rsid();

        if (mOutgoingSubstreamMap.find(initiatingLSID) != mOutgoingSubstreamMap.end()) {
	  boost::shared_ptr< Stream<EndPointType> > stream = mOutgoingSubstreamMap[initiatingLSID];
          mIncomingSubstreamMap[incomingLsid] = stream;
          std::cout << "REPLY packet received\n";

	  if (stream->mStreamReturnCallback != NULL){
	    stream->mStreamReturnCallback(stream);
	  }
        }
        else {
          std::cout << "Received reply packet for unknown stream\n";
        }
      }
    }
    else if (received_stream_msg->type() == received_stream_msg->DATA) {
      
      printf("DATA received\n");

      if (mIncomingSubstreamMap.find(incomingLsid) != mIncomingSubstreamMap.end()) {
	 boost::shared_ptr< Stream<EndPointType> > stream_ptr = 
	                                        mIncomingSubstreamMap[incomingLsid];
	 stream_ptr->receiveData( received_stream_msg->payload().data(),
				  received_stream_msg->bsn(),
				  received_stream_msg->payload().size()				  
				  );
      }
    }
    else if (received_stream_msg->type() == received_stream_msg->ACK) {
      
      printf("ACK received : offset = %d\n", (int)received_channel_msg->ack_sequence_number() );

      if (mIncomingSubstreamMap.find(incomingLsid) != mIncomingSubstreamMap.end()) {
	 boost::shared_ptr< Stream<EndPointType> > stream_ptr = 
	                                        mIncomingSubstreamMap[incomingLsid];
	 stream_ptr->receiveData( received_stream_msg->payload().data(),
				  received_channel_msg->ack_sequence_number(),
				  received_stream_msg->payload().size()				  
				  );
      }
    }
  }

};


class StreamBuffer{
public:

  const void* mBuffer;
  uint16 mBufferLength;
  uint32 mOffset;

  StreamBuffer(const void* data, int len, int offset){
    mBuffer = data;
    mBufferLength = len;
    mOffset = offset;
  }
};

template <class EndPointType>
class Stream {
public:

  StreamReturnCallbackFunction mStreamReturnCallback;
  
   enum StreamStates {
       DISCONNECTED = 1,       
       CONNECTED=2,
       PENDING_DISCONNECT=3,
     };

  Stream(Stream<EndPointType>* parent, Connection<EndPointType>* conn,
	 USID usid, LSID lsid, void* initial_data, uint32 length, 
	 bool remotelyInitiated, LSID remoteLSID, StreamReturnCallbackFunction cb)
  : 
    mStreamReturnCallback(cb),
    mNumBytesSent(0),
    mParentStream(parent),
    mConnection(conn),    
    mUSID(usid),
    mLSID(lsid),
    MAX_BUFFER_SIZE(500),
    MAX_QUEUE_LENGTH(1000000),
    mRetransmitTimer(mIOService)
  {
    if (remotelyInitiated) {
      sendReplyPacket(initial_data, length, remoteLSID);
    }
    else {
      sendInitPacket(initial_data, length);
    }

    mCurrentQueueLength = 0;
    
    thrd = new Thread(boost::bind(&(Stream<EndPointType>::ioServicingLoop), this));
    //boost::thread t(boost::bind(&boost::asio::io_service::run, &mIOService));
  }

  ~Stream() {
    printf("Stream getting destroyed\n");
    fflush(stdout);    

    mLooping = false;
    thrd->join();    
    
    //int* x = NULL;
    //int y=*x;    // for deliberately crashing everything :P       
  }

  void ioServicingLoop() {
    mQueuedBuffers.clear();

    Time start_time = Timer::now();
    mLooping = true;
    while (mLooping) {
      //boost::this_thread::sleep(boost::posix_time::microseconds(250));

      //this should wait for the queue to get occupied... right now it is
      //just polling...
      Time cur_time = Timer::now();
      if ( (cur_time - start_time).toMilliseconds() > 10000) {
        resendUnackedPackets();
        start_time = cur_time;
      }
      
      boost::mutex::scoped_lock l(m_mutex);

      while ( !mQueuedBuffers.empty() ) {
        printf("ioServicingLoop enqueuing packets into send queue\n");
 
	boost::shared_ptr<StreamBuffer> buffer = mQueuedBuffers.front();

	uint64 channelID = sendDataPacket(buffer->mBuffer, 
					  buffer->mBufferLength,
					  buffer->mOffset
					  );

	if ( mChannelToBufferMap.find(channelID) == mChannelToBufferMap.end() ) {
	  // printf("Adding to channel-buffer map: channelID=%d, offset=%d\n",
 	  //	 (int) channelID, (int) buffer->mOffset);
	  mChannelToBufferMap[channelID] = buffer;
	}
	
	mQueuedBuffers.pop_front();
        start_time = cur_time;

	/*mRetransmitTimer.expires_from_now(boost::posix_time::milliseconds(100));
	mRetransmitTimer.async_wait(boost::bind(&(Stream<EndPointType>::resendUnackedPackets), 
                   	            this, boost::asio::placeholders::error));*/
      }
    }
  }

  void resendUnackedPackets(/*const boost::system::error_code& error*/) {
    //printf("Called resendUnackedPackets\n");
     //if (error) {
     //  printf("timer handler had ERROR\n");
     //}
     boost::mutex::scoped_lock l(m_mutex);
     
     for(std::map<uint64,boost::shared_ptr<StreamBuffer> >::iterator it=mChannelToBufferMap.begin();
	 it != mChannelToBufferMap.end(); ++it)
     {
        mQueuedBuffers.push_back(it->second);
        printf("Resending unacked packet at offset %d\n", (int)(it->second->mOffset));	
     }

     mChannelToBufferMap.clear();
  }

  /* Writes data bytes to the stream. If not all bytes can be transmitted
     immediately, they are queued locally until ready to transmit. 
     @param data the buffer containing the bytes to be written
     @param len the length of the buffer
     @return the number of bytes written or enqueued, or -1 if an error  
             occurred
  */
  virtual int write(const uint8* data, int len) {
    boost::mutex::scoped_lock l(m_mutex);
    int count = 0;
    if (len <= MAX_BUFFER_SIZE) {
      if (mCurrentQueueLength+len > MAX_QUEUE_LENGTH) {
	return 0;
      }
      mQueuedBuffers.push_back( boost::shared_ptr<StreamBuffer>(new StreamBuffer(data, len, mNumBytesSent)) );
      mCurrentQueueLength += len;
      mNumBytesSent += len;

      return len;
    }
    else {      
      int currOffset = 0;
      while (currOffset < len) {	
	int buffLen = (len-currOffset > MAX_BUFFER_SIZE) ? 
	              MAX_BUFFER_SIZE : 
	              (len-currOffset);

	if (mCurrentQueueLength + buffLen > MAX_QUEUE_LENGTH) {
	  return currOffset;
	}

	mQueuedBuffers.push_back( boost::shared_ptr<StreamBuffer>(new StreamBuffer(data+currOffset, buffLen, mNumBytesSent)) );
	currOffset += buffLen;
	mCurrentQueueLength += buffLen;
	mNumBytesSent += buffLen;

	count++;
      }

      printf("NUM PKTS GEN = %d, currOffset=%d\n", count, currOffset);
      return currOffset;
    }

    return 0;
  }

  /* Reads data bytes from a stream. If no bytes are available to be
     read immediately, it returns without reading anything. The function
     does not block if no bytes are available to read.
     @param data the buffer into which to read. If NULL, the received data
                 is discarded.
     @param len the length of the buffer
     @return the number of bytes read into the buffer.
  */
  virtual int read(uint8* data, int len) {
    return 0;
  }

  /* Gathers data from the buffers described in 'vec',
     which is taken to be 'count' structures long, and
     writes them to the stream. As each buffer is 
     written, it moves on to the next. If not all bytes
     can be transmitted immediately, they are queued 
     locally until ready to transmit.

     The return value is a count of bytes written.

     @param vec the array containing the iovec buffers to be written
     @param count the number of iovec buffers in the array
     @return the number of bytes written or enqueued, or -1 if an error  
             occurred
  */
  virtual int writev(const struct iovec* vec, int count) {
    return 0;
  }

  /* Reads data from stream and scatters it into the buffers
     described in 'vec', which is taken to be 'count' structures
     long. As each buffer is filled, data is sent to the next.

      Note that readv is not guaranteed to fill all the buffers.
      It may stop at any point, for the same reasons read would.

     @param vec the array containing the iovec buffers to be read into
     @param count the number of iovec buffers in the array
     @return the number of bytes read, or -1 if an error  
             occurred
  */
  virtual int readv(const struct iovec* vec, int count) {
    return 0;
  }

  /*
    Returns true if there are bytes available to read from the stream.

    @return true, if there are bytes available to read from the stream;
            false otherwise.
  */
  virtual bool pollIfReadable() {
    return true;
  }
  
  /*
    Register a callback which will be called when there are bytes to be
    read from the stream. 

    @param ReadCallback a function of type "void (uint8*, int)" which will 
           be called when data is available. The "uint8*" field will be filled
           up with the received data, while the 'int' field will contain 
           the size of the data.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadCallback( ReadCallback ) {
    return true;
  }

  /* Close this stream. If the 'force' parameter is 'false',
     all outstanding data is sent and acknowledged before the stream is closed.
     Otherwise, the stream is closed immediately and outstanding data may be lost. 
     Note that in the former case, the function will still return immediately, changing
     the state of the connection PENDING_DISCONNECT without necessarily talking to the
     remote endpoint.
     @param force use false if the stream should be gracefully closed, true otherwise.
     @return  true if the stream was successfully closed. 
     
  */
  virtual bool closeStream(bool force) {
    return true;
  }

  /* 
    Sets the priority of this stream.
    As in the original SST interface, this implementation gives strict preference to 
    streams with higher priority over streams with lower priority, but it divides
    available transmit bandwidth evenly among streams with the same priority level.
    All streams have a default priority level of zero.
    @param the new priority level of the stream.
  */
  virtual void setPriority(int pri) {

  }

  /*Returns the stream's current priority level.
    @return the stream's current priority level
  */
  virtual int priority() {
    return 0;
  }

  /* Returns the top-level connection that created this stream.
     @return a pointer to the connection that created this stream.
  */
  virtual Connection<EndPointType>* connection() {
    return NULL;
  }
    
  /* Creates a child stream. The function also queues up
     any initial data that needs to be sent on the child stream. The function does not
     return a stream immediately since stream creation might take some time and
     yet fail in the end. So the function returns without synchronizing with the
     remote host. Instead the callback function provides a reference-counted,
     shared-pointer to the stream. If this connection hasn't synchronized with 
     the remote endpoint yet, this function will also take care of doing that.

     All the callbacks registered on the child stream will also be registered on the
     parent stream, but not vice versa. 
 
     @data A pointer to the initial data buffer that needs to be sent on this stream.
         Having this pointer removes the need for the application to enqueue data
         until the stream is actually created.
     @port The length of the data buffer.
     @StreamReturnCallbackFunction A callback function which will be called once the 
                                  stream is created and the initial data queued up
                                  (or actually sent?). The function will provide  a 
                                  reference counted, shared pointer to the  connection.
  */
  virtual void createChildStream(StreamReturnCallbackFunction, void* data, int length) {

  }

  void receiveData( const void* buffer, uint64 offset, uint32 len ) {
    if (len > 0) {
      sendAckPacket();
      mReceivedSegments[offset] = 1;
      printf("mReceivedSegments.size() = %d\n", (int)mReceivedSegments.size());
      printf("Received Segment at offset = %d\n", (int) offset);      
    }
    else {
      boost::mutex::scoped_lock l(m_mutex);
      
      if (mChannelToBufferMap.find(offset) != mChannelToBufferMap.end()) {
	uint64 dataOffset = mChannelToBufferMap[offset]->mOffset;
	printf("REMOVED ACKED PACKET. Offset: %d\n", (int) mChannelToBufferMap[offset]->mOffset);
	mChannelToBufferMap.erase(offset);

	std::vector <uint64> channelOffsets;
	for(std::map<uint64, boost::shared_ptr<StreamBuffer> >::iterator it = mChannelToBufferMap.begin();
	    it != mChannelToBufferMap.end(); ++it)
	{
	  if (it->second->mOffset == dataOffset) {
	    channelOffsets.push_back(it->first);
	  }
	}

	for (uint i=0; i< channelOffsets.size(); i++) {
	  mChannelToBufferMap.erase(channelOffsets[i]);
	}	
      }
    }
  }

private:
  LSID getLSID() {
    return mParentStream->getLSID();
  }

  void sendInitPacket(void* data, uint32 len) {
    printf("Sending Init packet\n");

    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.INIT); 
    sstMsg.set_flags(0);
    sstMsg.set_window(1024);

    if (mParentStream != NULL) {
      sstMsg.set_psid(mParentStream->getLSID());
    }
    else {
      sstMsg.set_psid(0);
    }
    sstMsg.set_bsn(0);    

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection->sendData( buffer.data(), buffer.size() );
  }

  void sendAckPacket() {
    printf("Sending Ack packet\n");

    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.ACK);
    sstMsg.set_flags(0);
    sstMsg.set_window(1024);

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection->sendData(  buffer.data(), buffer.size());
  }

  uint64 sendDataPacket(const void* data, uint32 len, uint32 offset) {
    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.DATA);
    sstMsg.set_flags(0);
    sstMsg.set_window(1024);
    
    sstMsg.set_bsn(offset);

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    return mConnection->sendData(  buffer.data(), buffer.size());
  }

  void sendReplyPacket(void* data, uint32 len, LSID remoteLSID) {
    printf("Sending Reply packet\n");
    
    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.REPLY); 
    sstMsg.set_flags(0);
    sstMsg.set_window(1024);
    sstMsg.set_rsid(remoteLSID);
    sstMsg.set_bsn(0);    

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection->sendData(  buffer.data(), buffer.size());
  }

  
  uint32 mNumBytesSent;

  Stream<EndPointType>* mParentStream;
  Connection<EndPointType>* mConnection;
  

  std::map<uint64, boost::shared_ptr<StreamBuffer> >  mChannelToBufferMap;

  std::deque< boost::shared_ptr<StreamBuffer> > mQueuedBuffers;
  uint32 mCurrentQueueLength;
  
  USID mUSID;
  LSID mLSID;

  uint16 MAX_BUFFER_SIZE;
  uint32 MAX_QUEUE_LENGTH;

  boost::mutex m_mutex;

  boost::asio::io_service mIOService;

  std::map<uint64, int> mReceivedSegments;

  Thread* thrd;

  bool mLooping;

  boost::asio::deadline_timer mRetransmitTimer;
  
};

}

#endif
