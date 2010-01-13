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
#include <bitset>

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

class Mutex {
public:

  Mutex() {  }

  Mutex(const Mutex& mutex) {  }

  boost::mutex& getMutex() {
    return mMutex;
  }

private:
  boost::mutex mMutex;
 
};


template <typename EndPointType>
class BaseDatagramLayer
{

public:
  BaseDatagramLayer(const ObjectHostContext* ctx) : mContext(ctx) {}

  void send(EndPoint<EndPointType> src, EndPoint<EndPointType> dest, String dataStr) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    mContext->objectHost->send(
	    src.port,src.endPoint,
            dest.port, dest.endPoint,
            dataStr
        );
  }
  
  /* This function will have to be re-implemented to support sending using
     other kinds of packets. For now, I'll implement only for ODP. */
  void asyncSend(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
    mContext->mainStrand->post(
			       std::tr1::bind(
				     &(BaseDatagramLayer<EndPointType>::send),this,
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

typedef std::tr1::function< void(int, boost::shared_ptr< Connection<UUID> > ) > ConnectionReturnCallbackFunction;

typedef std::tr1::function< void(int, boost::shared_ptr< Stream<UUID> >) >  StreamReturnCallbackFunction;
typedef std::tr1::function< void (int, void*) >  DatagramSendDoneCallback;

typedef std::tr1::function<void (uint8*, int) >  ReadDatagramCallback;

typedef std::tr1::function<void (uint8*, int) > ReadCallback;

typedef UUID USID;

typedef uint16 LSID;



class ChannelSegment{
public:

  uint8* mBuffer;
  uint16 mBufferLength;
  uint64 mChannelSequenceNumber;
  uint64 mAckSequenceNumber;

  Time mTransmitTime;
  Time mAckTime;

  ChannelSegment( void* data, int len, uint64 channelSeqNum, uint64 ackSequenceNum) : 
                                               mBufferLength(len),
					      mChannelSequenceNumber(channelSeqNum), 
					      mAckSequenceNumber(ackSequenceNum),
					      mTransmitTime(Time::null()), mAckTime(Time::null())
  {
    mBuffer = new uint8[len];
    memcpy(mBuffer, (uint8*) data, len);
  }

  

  ~ChannelSegment() {
    delete [] mBuffer;
  }

  void setAckTime(Time& ackTime) {
    mAckTime = ackTime;
  }
  
};

template <class EndPointType>
class Connection {

private:
  friend class Stream<EndPointType>;

  typedef std::map<EndPoint<EndPointType>, boost::shared_ptr<Connection> >  ConnectionMap;
  typedef std::map<EndPoint<EndPointType>, ConnectionReturnCallbackFunction>  ConnectionReturnCallbackMap;
  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction>   StreamReturnCallbackMap;  
  

  static ConnectionMap mConnectionMap;
  static std::bitset<256> mAvailableChannels;

  static ConnectionReturnCallbackMap mConnectionReturnCallbackMap;
  static StreamReturnCallbackMap  mListeningConnectionsCallbackMap;

  static Mutex mStaticMembersLock;
    
  ObjectHostContext* mContext;
  EndPoint<EndPointType> mLocalEndPoint;
  EndPoint<EndPointType> mRemoteEndPoint;

  BaseDatagramLayer<UUID> mDatagramLayer;

  int mState;
  uint8 mRemoteChannelID;
  uint8 mLocalChannelID;

  uint64 mTransmitSequenceNumber;
  uint64 mLastReceivedSequenceNumber;   //the last transmit sequence number received from the other side
   
  std::map<LSID, boost::shared_ptr< Stream<EndPointType> > > mOutgoingSubstreamMap;
  std::map<LSID, boost::shared_ptr< Stream<EndPointType> > > mIncomingSubstreamMap;

  std::vector<ReadDatagramCallback> mReadDatagramCallbacks;

  uint16 mNumStreams;

  std::deque< boost::shared_ptr<ChannelSegment> > mQueuedSegments;
  
  std::deque< boost::shared_ptr<ChannelSegment> > mOutstandingSegments;

  uint16 mCwnd;
   
  uint64 mRTO; // RTO in microseconds
  bool mFirstRTO;

  bool mLooping;
  Thread* mThread;

  boost::mutex mQueueMutex;  
  boost::condition_variable mQueueEmptyCondVar;

  uint16 MAX_DATAGRAM_SIZE;
  uint16 MAX_PAYLOAD_SIZE;
  uint32 MAX_QUEUED_SEGMENTS;
  float  CC_ALPHA;

  boost::weak_ptr<Connection<EndPointType> > weak_this;

  Connection(const ObjectHostContext* ctx, EndPoint<EndPointType> localEndPoint, 
             EndPoint<EndPointType> remoteEndPoint)
    : mLocalEndPoint(localEndPoint), mRemoteEndPoint(remoteEndPoint),
      mDatagramLayer(ctx), mState(CONNECTION_DISCONNECTED),
      mRemoteChannelID(0), mLocalChannelID(1), mTransmitSequenceNumber(1), 
      mLastReceivedSequenceNumber(1), mNumStreams(0), mCwnd(1), mRTO(20000), mFirstRTO(true),
      mLooping(true), MAX_DATAGRAM_SIZE(1000), MAX_PAYLOAD_SIZE(1300),
      MAX_QUEUED_SEGMENTS(300),
      CC_ALPHA(0.8)
  {
    mThread = new Thread(boost::bind(&(Connection<EndPointType>::sendChannelSegmentLoop), this));
  }
  
  void sendSSTChannelPacket(CBR::Protocol::SST::SSTChannelHeader& sstMsg) {
    std::string buffer = serializePBJMessage(sstMsg);
    mDatagramLayer.asyncSend(&mLocalEndPoint, &mRemoteEndPoint, (void*) buffer.data(),
				       buffer.size());
  }

  /* Returns -1 if no channel is available. Otherwise returns the lowest
     available channel. */
  static int getAvailableChannel() {
    //TODO: faster implementation.
    for (uint i=1; i<mAvailableChannels.size(); i++) {
      if (mAvailableChannels.test(i)) {	
	mAvailableChannels.set(i, 1);
	return i;
      }
    }

    return -1;
  }

  static void releaseChannel(uint8 channel) {
    assert(channel > 0);
    mAvailableChannels.set(channel, 0);
  }

  void sendChannelSegmentLoop() {
    // should start from ssthresh, the slow start lower threshold, but starting
    // from 1 for now. Still need to implement slow start.
    while (mLooping) {
      uint16 numSegmentsSent = 0;
      {
	boost::unique_lock<boost::mutex> lock(mQueueMutex);	
	
        while (mQueuedSegments.empty()){
          printf("Waiting on lock\n");
          mQueueEmptyCondVar.wait(lock);

	  if (!mLooping) return;
	}	
	
	printf("Stopped waiting: %s has window size = %d, queued_segments.size=%d\n", mLocalEndPoint.endPoint.readableHexData().c_str(), mCwnd, (int)mQueuedSegments.size());
	
	for (int i = 0; i < mCwnd; i++) {
	  if ( !mQueuedSegments.empty() ) {
	    boost::shared_ptr<ChannelSegment> segment = mQueuedSegments.front();
	    
	    CBR::Protocol::SST::SSTChannelHeader sstMsg;
	    sstMsg.set_channel_id( mRemoteChannelID );
	    sstMsg.set_transmit_sequence_number(segment->mChannelSequenceNumber);
	    sstMsg.set_ack_count(1);
	    sstMsg.set_ack_sequence_number(segment->mAckSequenceNumber);
	    
	    sstMsg.set_payload(segment->mBuffer, segment->mBufferLength);
	    
	    sendSSTChannelPacket(sstMsg);
	    
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

      if (mState == CONNECTION_PENDING_CONNECT) {
	boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());
	
        printf("Remote endpoint not available to connect\n");
        mConnectionReturnCallbackMap[mRemoteEndPoint](-1, 
                                                boost::shared_ptr<Connection<EndPointType> > (weak_this) ); 
        return;
      }
      
      printf("%s mOutstandingSegments.size()=%d\n", mLocalEndPoint.endPoint.toString().c_str() , (int) mOutstandingSegments.size() );

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
	    mRTO = CC_ALPHA * mRTO + (1.0-CC_ALPHA) * (segment->mAckTime - segment->mTransmitTime).toMicroseconds();
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
    boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());
    
    if (mConnectionMap.find(localEndPoint) != mConnectionMap.end()) {
      return false;
    }
    
    boost::shared_ptr<Connection>  conn =  boost::shared_ptr<Connection> (
						           new Connection(ctx, localEndPoint, remoteEndPoint));
    conn->weak_this = conn;
    mConnectionMap[localEndPoint] = conn;

    conn->setState(CONNECTION_PENDING_CONNECT);    

    uint8 payload[1];
    payload[0] = getAvailableChannel();
    
    conn->setLocalChannelID(1);
    conn->sendData(payload, 1);

    mConnectionReturnCallbackMap[remoteEndPoint] = cb;

    return true;
  }

  static bool listen(StreamReturnCallbackFunction cb, EndPoint<EndPointType> listeningEndPoint) {
    boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());
    
    mListeningConnectionsCallbackMap[listeningEndPoint] = cb;

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
                                 should have the signature void (int,boost::shared_ptr<Stream>).
                                 
  */
  virtual void stream(StreamReturnCallbackFunction cb, void* initial_data, int length) {
    stream(cb, initial_data, length, NULL);
  }

  virtual void stream(StreamReturnCallbackFunction cb, void* initial_data, int length, 
                      Stream<EndPointType>* parentStream) {
    USID usid = createNewUSID();
    LSID lsid = ++mNumStreams;

    boost::shared_ptr<Stream<EndPointType> > stream =
      boost::shared_ptr<Stream<EndPointType> >
      ( new Stream<EndPointType>(parentStream, weak_this, usid, lsid, initial_data, length, false, 0, cb) );

    mOutgoingSubstreamMap[lsid]=stream;
  }

  uint64 sendData(const void* data2, uint32 length) {
    boost::lock_guard<boost::mutex> lock(mQueueMutex);

    assert(length <= MAX_PAYLOAD_SIZE);

    uint8* data = new uint8[length];
    memcpy(data, (uint8*) data2, length);
    
    CBR::Protocol::SST::SSTStreamHeader* stream_msg =
                       new CBR::Protocol::SST::SSTStreamHeader();

    std::string str = std::string( (char*)data, length);

    bool parsed = parsePBJMessage(stream_msg, str);

    uint64 transmitSequenceNumber =  mTransmitSequenceNumber;

    bool pushedIntoQueue = false;

    if ( stream_msg->type() !=  stream_msg->ACK) {
      if (mQueuedSegments.size() < MAX_QUEUED_SEGMENTS) { 
	mQueuedSegments.push_back( boost::shared_ptr<ChannelSegment>( 
                    new ChannelSegment(data, length, mTransmitSequenceNumber, mLastReceivedSequenceNumber) ) );
	pushedIntoQueue = true;
      }
    }
    else {
      CBR::Protocol::SST::SSTChannelHeader sstMsg;
      sstMsg.set_channel_id( mRemoteChannelID );
      sstMsg.set_transmit_sequence_number(mTransmitSequenceNumber);
      sstMsg.set_ack_count(1);
      sstMsg.set_ack_sequence_number(mLastReceivedSequenceNumber); 

      sstMsg.set_payload(data, length);
      
      sendSSTChannelPacket(sstMsg);      
    }

    mTransmitSequenceNumber++;    
    
    if (pushedIntoQueue) {
      mQueueEmptyCondVar.notify_all();
    }
    return transmitSequenceNumber;    
  }

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

  void markAcknowledgedPacket(uint64 receivedAckNum) {
    if (mOutstandingSegments.size() > 0){
      printf("%s mOutstandingSegments[0]->mChannelSequenceNumber=%d, receivedAckNum=%d\n", mLocalEndPoint.endPoint.toString().c_str(), (int)(mOutstandingSegments[0]->mChannelSequenceNumber), (int)(receivedAckNum) );
    }

    for (uint i = 0; i < mOutstandingSegments.size(); i++) {
      if (mOutstandingSegments[i]->mChannelSequenceNumber == receivedAckNum) {
        printf("Packet acked at %s\n", mLocalEndPoint.endPoint.toString().c_str());
	mOutstandingSegments[i]->mAckTime = Timer::now();
      }
    }
  }

  void receiveMessage(void* recv_buff, int len) {
    uint8* data = (uint8*) recv_buff;
    std::string str = std::string((char*) data, len);
    
    CBR::Protocol::SST::SSTChannelHeader* received_msg = 
                       new CBR::Protocol::SST::SSTChannelHeader();
    bool parsed = parsePBJMessage(received_msg, str);

    mLastReceivedSequenceNumber = received_msg->transmit_sequence_number();

    uint64 receivedAckNum = received_msg->ack_sequence_number();

    markAcknowledgedPacket(receivedAckNum);

    if (mState == CONNECTION_PENDING_CONNECT) {
      mState = CONNECTION_CONNECTED;

      uint8* received_payload = (uint8*) received_msg->payload().data();

      this->setRemoteChannelID(received_payload[0]);
               
      sendData( received_payload, 0 );

      if (mConnectionReturnCallbackMap.find(mRemoteEndPoint) != mConnectionReturnCallbackMap.end()) 
      {
	if (mConnectionMap.find(mLocalEndPoint) != mConnectionMap.end()) {
	  boost::shared_ptr<Connection> conn = mConnectionMap[mLocalEndPoint];	

	  mConnectionReturnCallbackMap[mRemoteEndPoint] (0, conn);
	}
      }

      mConnectionReturnCallbackMap.erase(mRemoteEndPoint);
    }
    else if (mState == CONNECTION_PENDING_RECEIVE_CONNECT) {
      mState = CONNECTION_CONNECTED;      

      std::cout << "Receiving side connected!!!\n";
    }
    else if (mState == CONNECTION_CONNECTED) {
      if (received_msg->payload().size() > 0) {
	parsePacket(recv_buff, len);
      }
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

    
    if (received_stream_msg->type() == received_stream_msg->INIT) {
      handleInitPacket(received_stream_msg);
    }
    else if (received_stream_msg->type() == received_stream_msg->REPLY) {
      handleReplyPacket(received_stream_msg);      
    }
    else if (received_stream_msg->type() == received_stream_msg->DATA) {
      handleDataPacket(received_stream_msg);
    }
    else if (received_stream_msg->type() == received_stream_msg->ACK) {       
      handleAckPacket(received_channel_msg, received_stream_msg);
    }
    else if (received_stream_msg->type() == received_stream_msg->DATAGRAM) {      
      handleDatagram(received_stream_msg);
    }

    delete received_channel_msg;
    delete received_stream_msg ;
  }

  void handleInitPacket(CBR::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    std::cout << "INIT packet received\n"; 
    LSID incomingLsid = received_stream_msg->lsid();
    
    if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
      uint8 buf[1];
      //create a new stream
      USID usid = createNewUSID();
      LSID newLSID = ++mNumStreams;
      boost::shared_ptr<Stream<EndPointType> > stream = 
	boost::shared_ptr<Stream<EndPointType> > 
	(new Stream<EndPointType>(NULL, weak_this, usid, newLSID,
				  buf, 0, true, incomingLsid, NULL));
      mOutgoingSubstreamMap[newLSID] = stream;
      mIncomingSubstreamMap[incomingLsid] = stream;
      
      stream->receiveData(received_stream_msg, received_stream_msg->payload().data(),
			  received_stream_msg->bsn(),
			  received_stream_msg->payload().size() );

      mListeningConnectionsCallbackMap[mLocalEndPoint](0, stream);

      //mListeningConnectionsCallback.erase(mLocalEndPoint);
    }
  }

  void handleReplyPacket(CBR::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
      LSID initiatingLSID = received_stream_msg->rsid();
      
      if (mOutgoingSubstreamMap.find(initiatingLSID) != mOutgoingSubstreamMap.end()) {
	boost::shared_ptr< Stream<EndPointType> > stream = mOutgoingSubstreamMap[initiatingLSID];
	mIncomingSubstreamMap[incomingLsid] = stream;
	std::cout << "REPLY packet received\n";
	
	if (stream->mStreamReturnCallback != NULL){
	  stream->mStreamReturnCallback(0, stream);
	  stream->receiveData(received_stream_msg, received_stream_msg->payload().data(),
			      received_stream_msg->bsn(),
			      received_stream_msg->payload().size() );
	}
      }
      else {
	std::cout << "Received reply packet for unknown stream\n";
      }
    }
  }

  void handleDataPacket(CBR::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    printf("DATA received\n");
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) != mIncomingSubstreamMap.end()) {
      boost::shared_ptr< Stream<EndPointType> > stream_ptr = 
	mIncomingSubstreamMap[incomingLsid];
      stream_ptr->receiveData( received_stream_msg,
			       received_stream_msg->payload().data(),
			       received_stream_msg->bsn(),
			       received_stream_msg->payload().size()				  
			       );
    }
  }

  void handleAckPacket(CBR::Protocol::SST::SSTChannelHeader* received_channel_msg,
		       CBR::Protocol::SST::SSTStreamHeader* received_stream_msg) 
  {
    printf("ACK received : offset = %d\n", (int)received_channel_msg->ack_sequence_number() );
    LSID incomingLsid = received_stream_msg->lsid();
    
    if (mIncomingSubstreamMap.find(incomingLsid) != mIncomingSubstreamMap.end()) {
      boost::shared_ptr< Stream<EndPointType> > stream_ptr = 
	mIncomingSubstreamMap[incomingLsid];
      stream_ptr->receiveData( received_stream_msg,
			       received_stream_msg->payload().data(),
			       received_channel_msg->ack_sequence_number(),
			       received_stream_msg->payload().size()				  
			       );
    }
  }

  void handleDatagram(CBR::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    printf("DATAGRAM received\n" );

    uint8* payload = (uint8*) received_stream_msg->payload().data();
    uint payload_size = received_stream_msg->payload().size();
      
    for (uint i=0 ; i < mReadDatagramCallbacks.size(); i++) {
      mReadDatagramCallbacks[i](payload, payload_size);
    }
 
    boost::lock_guard<boost::mutex> lock(mQueueMutex);
   
    CBR::Protocol::SST::SSTChannelHeader sstMsg;
    sstMsg.set_channel_id( mRemoteChannelID );
    sstMsg.set_transmit_sequence_number(mTransmitSequenceNumber);
    sstMsg.set_ack_count(1);
    sstMsg.set_ack_sequence_number(mLastReceivedSequenceNumber);
            
    sendSSTChannelPacket(sstMsg);
    
    mTransmitSequenceNumber++;    
  }

public:

  ~Connection() {
    mLooping = false;
    mQueueEmptyCondVar.notify_all();
    
    mThread->join();

    delete mThread;

    printf("Connection on %s getting destroyed\n", mLocalEndPoint.endPoint.readableHexData().c_str());
    fflush(stdout);
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
    int currOffset = 0;

    LSID lsid = ++mNumStreams;

    while (currOffset < length) {	
      int buffLen = (length-currOffset > MAX_PAYLOAD_SIZE) ? 
	            MAX_PAYLOAD_SIZE : 
           	    (length-currOffset);

      printf("buffLen=%d\n", buffLen);

      CBR::Protocol::SST::SSTStreamHeader sstMsg;
      sstMsg.set_lsid( lsid );
      sstMsg.set_type(sstMsg.DATAGRAM); 
      sstMsg.set_flags(0);
      sstMsg.set_window( 1024 );
      
      sstMsg.set_payload( ((uint8*)data)+currOffset, buffLen);
      
      std::string buffer = serializePBJMessage(sstMsg);
      sendData(  buffer.data(), buffer.size() );            

      currOffset += buffLen;
    }

    if (cb != NULL) {
      //invoke the callback function      
    }

    printf("datagram() exited\n");
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
    mReadDatagramCallbacks.push_back(cb);

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
    boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());

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

      if (mListeningConnectionsCallbackMap.find(localEndPoint) != mListeningConnectionsCallbackMap.end()) {
        uint8* received_payload = (uint8*) received_msg->payload().data();
      
        uint8 payload[1];
        payload[0] = getAvailableChannel();      

        boost::shared_ptr<Connection>  conn =  
                   boost::shared_ptr<Connection>(
				    new Connection(ctx, localEndPoint, remoteEndPoint));
        conn->weak_this = conn;
        mConnectionMap[localEndPoint] = conn;

        conn->setLocalChannelID(2);      
        conn->setRemoteChannelID(received_payload[0]);
        conn->setState(CONNECTION_PENDING_RECEIVE_CONNECT);

        conn->sendData(payload, 1);        
      }
      else {
        std::cout << "No one listening on this connection\n";
      }
    }

    delete received_msg;
  }
  
};


class StreamBuffer{
public:

  uint8* mBuffer;
  uint16 mBufferLength;
  uint32 mOffset;

  StreamBuffer(const uint8* data, int len, int offset){
    assert(len > 0);

    printf("Constructing StreamBuffer\n");
    fflush(stdout);

    mBuffer = new uint8[len];
    memcpy(mBuffer, data, len);
    //mBuffer = data;
    mBufferLength = len;
    mOffset = offset;
  }

  ~StreamBuffer() {
    delete mBuffer;

    printf("Destructing StreamBuffer\n");
    fflush(stdout);
  }
};

template <class EndPointType>
class Stream  {
public:      
  
   enum StreamStates {
       DISCONNECTED = 1,       
       CONNECTED=2,
       PENDING_DISCONNECT=3,
     };

  

  ~Stream() {
    printf("Stream getting destroyed\n");
    fflush(stdout);    

    mLooping = false;
    mThread->join();

    delete mThread;
    
    delete [] mReceiveBuffer;
  }

  static bool connectStream(const ObjectHostContext* ctx, 
			    EndPoint <EndPointType> localEndPoint, 
			    EndPoint <EndPointType> remoteEndPoint, 
			    StreamReturnCallbackFunction cb) 
  {
    boost::mutex::scoped_lock lock(mStreamCreationMutex.getMutex());
    if (mStreamReturnCallbackMap.find(localEndPoint) != mStreamReturnCallbackMap.end()) {
      return false;
    }    

    mStreamReturnCallbackMap[localEndPoint] = cb;

    bool result = Connection<EndPointType>::createConnection(ctx, localEndPoint, 
					       remoteEndPoint, 
					       connectionCreated);

    return result;
  }

  static bool listen(StreamReturnCallbackFunction cb, EndPoint <EndPointType> listeningEndPoint) {    
    return Connection<EndPointType>::listen(cb, listeningEndPoint);    
  }

  
  /* Writes data bytes to the stream. If not all bytes can be transmitted
     immediately, they are queued locally until ready to transmit. 
     @param data the buffer containing the bytes to be written
     @param len the length of the buffer
     @return the number of bytes written or enqueued, or -1 if an error  
             occurred
  */
  virtual int write(const uint8* data, int len) {
    if (mState != CONNECTED) {
      return -1;
    }

    boost::mutex::scoped_lock l(mQueueMutex);
    int count = 0;

    printf("Calling write with len=%d\n", len);

    if (len <= MAX_PAYLOAD_SIZE) {
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
	int buffLen = (len-currOffset > MAX_PAYLOAD_SIZE) ? 
	              MAX_PAYLOAD_SIZE : 
	              (len-currOffset);

	printf("NUM PKTS GEN = %d, currOffset=%d, mCurrentQueueLength=%d\n", count, currOffset, mCurrentQueueLength);
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

    return -1;
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
    int totalBytesWritten = 0;

    for (int i=0; i < count; i++) {
      int numWritten = write( (const uint8*) vec[i].iov_base, vec[i].iov_len);
      
      if (numWritten < 0) return -1;

      totalBytesWritten += numWritten;

      if (numWritten == 0) {
	return totalBytesWritten;
      }
    }
    
    return totalBytesWritten;
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
  virtual bool registerReadCallback( ReadCallback callback) {
    mReadCallback = callback;

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
  virtual bool close(bool force) {
    if (force) {
      mLooping = false;
      mThread->join();
      mState = DISCONNECTED;
      return true;
    }
    else {
      mState = PENDING_DISCONNECT;
      return true;
    }
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
  virtual boost::weak_ptr<Connection<EndPointType> > connection() {
    return mConnection;
  }
    
  /* Creates a child stream. The function also queues up
     any initial data that needs to be sent on the child stream. The function does not
     return a stream immediately since stream creation might take some time and
     yet fail in the end. So the function returns without synchronizing with the
     remote host. Instead the callback function provides a reference-counted,
     shared-pointer to the stream. If this connection hasn't synchronized with 
     the remote endpoint yet, this function will also take care of doing that.

     @data A pointer to the initial data buffer that needs to be sent on this stream.
         Having this pointer removes the need for the application to enqueue data
         until the stream is actually created.
     @port The length of the data buffer.
     @StreamReturnCallbackFunction A callback function which will be called once the 
                                  stream is created and the initial data queued up
                                  (or actually sent?). The function will provide  a 
                                  reference counted, shared pointer to the  connection.
  */
  virtual void createChildStream(StreamReturnCallbackFunction cb, void* data, int length) {
    printf("Creating child stream\n");
    fflush(stdout);

    mConnection.lock()->stream(cb, data, length, this);
  }

private:
  Stream(Stream<EndPointType>* parent, boost::weak_ptr<Connection<EndPointType> > conn,
	 USID usid, LSID lsid, void* initial_data, uint32 length, 
	 bool remotelyInitiated, LSID remoteLSID, StreamReturnCallbackFunction cb)
  :     
    mState(CONNECTED),
    mParentStream(parent),
    mConnection(conn),    
    mUSID(usid),
    mLSID(lsid),
    MAX_PAYLOAD_SIZE(1000),
    MAX_QUEUE_LENGTH(4000000),
    MAX_RECEIVE_WINDOW(15000),
    mRetransmitTimer(mIOService),
    mTransmitWindowSize(MAX_RECEIVE_WINDOW),
    mReceiveWindowSize(MAX_RECEIVE_WINDOW),
    mNumOutstandingBytes(0),
    mNextByteExpected(0),
    mLastContiguousByteReceived(-1),
    mLastByteReceived(-1),   
    mStreamReturnCallback(cb),
    mConnected (false),           
    MAX_INIT_RETRANSMISSIONS(10)
  {
    printf("Creating Stream\n");
    
    if (remotelyInitiated) {
      mConnected = true;
    }

    mInitialDataLength = (length <= MAX_PAYLOAD_SIZE) ? length : MAX_PAYLOAD_SIZE;

    if (initial_data != NULL) {
      mInitialData = new uint8[mInitialDataLength];
   
      memcpy(mInitialData, initial_data, mInitialDataLength);
    }
    else {
      mInitialData = new uint8[1];
      mInitialDataLength = 0;
    }

    mReceiveBuffer = new uint8[mReceiveWindowSize];
    mReceiveBitmap = new uint8[mReceiveWindowSize];
    memset(mReceiveBitmap, 0, mReceiveWindowSize);

    mQueuedBuffers.clear();
    
    if (remotelyInitiated) {
      sendReplyPacket(initial_data, mInitialDataLength, remoteLSID);
    }
    else {
      sendInitPacket(initial_data, mInitialDataLength);
    }

    mNumInitRetransmissions = 1;
    mNumBytesSent = mInitialDataLength;

    mCurrentQueueLength = 0;
   
    if (length > mInitialDataLength) {
      write( ((uint8*)initial_data) + mInitialDataLength, length - mInitialDataLength);
    }    
    
    mThread = new Thread(boost::bind(&(Stream<EndPointType>::sendStreamSegmentLoop), this));
    //boost::thread t(boost::bind(&boost::asio::io_service::run, &mIOService));
  }

  static void connectionCreated( int errCode, boost::shared_ptr<Connection<EndPointType> > c) {
    boost::mutex::scoped_lock lock(mStreamCreationMutex.getMutex());

    if (errCode != 0) {
      mStreamReturnCallbackMap[c->localEndPoint()](-1, boost::shared_ptr<Stream<EndPointType> >() );
      
      mStreamReturnCallbackMap.erase(c->localEndPoint());

      return;
    }

    int length = 2000000;
    uint8* f = new uint8[length];
    for (int i=0; i<length; i++) {
      f[i] = i % 255;
    }

    std::cout << "c->localEndPoint=" << c->localEndPoint().endPoint.toString() << "\n";
    assert(mStreamReturnCallbackMap.find(c->localEndPoint()) != mStreamReturnCallbackMap.end());

    c->stream(mStreamReturnCallbackMap[c->localEndPoint()], f , length);

    mStreamReturnCallbackMap.erase(c->localEndPoint());
  }  


  void sendStreamSegmentLoop() {
    Time start_time = Timer::now();

    while (!mConnected && mNumInitRetransmissions < MAX_INIT_RETRANSMISSIONS ) {
      Time cur_time = Timer::now();

      if ( (cur_time - start_time).toMilliseconds() > 200) {	
        sendInitPacket(mInitialData, mInitialDataLength); 
        start_time = cur_time;
	mNumInitRetransmissions++;
      }
    }

    
    delete [] mInitialData;
    mInitialDataLength = 0;
    

    if (!mConnected) {
      std::cout << mConnection.lock()->localEndPoint().endPoint.toString() << " not connected!!\n";

      //send back an error to the app by calling mStreamReturnCallback
      //with an error code.
      mStreamReturnCallback(-1, boost::shared_ptr<Stream<UUID> >() );
    }

    start_time = Timer::now();
    mLooping = true;
    while (mLooping) {

      //this should wait for the queue to get occupied... right now it is
      //just polling...
      Time cur_time = Timer::now();
      if ( (cur_time - start_time).toMilliseconds() > 1000) {
        resendUnackedPackets();
        start_time = cur_time;
      }

      if (mState == PENDING_DISCONNECT && 
          mQueuedBuffers.empty()  && 
          mChannelToBufferMap.empty() ) 
      {
        mLooping = false;
        mState = DISCONNECTED;

        break;
      }
      
      boost::mutex::scoped_lock l(mQueueMutex);

      while ( !mQueuedBuffers.empty() && mLooping ) {
        boost::shared_ptr<StreamBuffer> buffer = mQueuedBuffers.front();

        if (mTransmitWindowSize < buffer->mBufferLength) {
	  break;
	}

        printf("ioServicingLoop enqueuing packet at offset %d into send queue\n", buffer->mOffset);
        printf("ioServicingLoop: mTransmitWindowSize=%d\n", (int) mTransmitWindowSize);

	uint64 channelID = sendDataPacket(buffer->mBuffer, 
					  buffer->mBufferLength,
					  buffer->mOffset
					  );

	if ( mChannelToBufferMap.find(channelID) == mChannelToBufferMap.end() ) {	  
	  mChannelToBufferMap[channelID] = buffer;
	}
	
	mQueuedBuffers.pop_front();
        start_time = cur_time;

	assert(buffer->mBufferLength <= mTransmitWindowSize);
	mTransmitWindowSize -= buffer->mBufferLength;
	mNumOutstandingBytes += buffer->mBufferLength;

	/*mRetransmitTimer.expires_from_now(boost::posix_time::milliseconds(100));
	mRetransmitTimer.async_wait(boost::bind(&(Stream<EndPointType>::resendUnackedPackets), 
                   	            this, boost::asio::placeholders::error));*/
      }
    }

    printf("ioServiceLoop exited\n");
  }

  void resendUnackedPackets(/*const boost::system::error_code& error*/) {
     //printf("Called resendUnackedPackets\n");
     //if (error) {
     //  printf("timer handler had ERROR\n");
     //}
     boost::mutex::scoped_lock l(mQueueMutex);     
     
     for(std::map<uint64,boost::shared_ptr<StreamBuffer> >::iterator it=mChannelToBufferMap.begin();
	 it != mChannelToBufferMap.end(); ++it)
     {
        mQueuedBuffers.push_front(it->second);
        printf("Resending unacked packet at offset %d\n", (int)(it->second->mOffset));	

	if (mTransmitWindowSize < it->second->mBufferLength){
	  assert( ((int) it->second->mBufferLength) > 0);
	  mTransmitWindowSize = it->second->mBufferLength;
	}
     }

     if (mChannelToBufferMap.empty() && !mQueuedBuffers.empty()) {
       boost::shared_ptr<StreamBuffer> buffer = mQueuedBuffers.front();

       if (mTransmitWindowSize < buffer->mBufferLength) {
	 mTransmitWindowSize = buffer->mBufferLength;
       }
     }

     mNumOutstandingBytes = 0;
     mChannelToBufferMap.clear();
  }
  


  void receiveData( CBR::Protocol::SST::SSTStreamHeader* streamMsg,
		    const void* buffer, uint64 offset, uint32 len ) 
  {
    if (streamMsg->type() == streamMsg->REPLY) { 
      if (!mConnected) printf("Connected NOW!\n");

      mConnected = true;
    }
    else if (streamMsg->type() == streamMsg->ACK) {
      boost::mutex::scoped_lock l(mQueueMutex);
      
      if (mChannelToBufferMap.find(offset) != mChannelToBufferMap.end()) {
	uint64 dataOffset = mChannelToBufferMap[offset]->mOffset;
	mNumOutstandingBytes -= mChannelToBufferMap[offset]->mBufferLength;

	printf("mNumOutstandingBytes=%d, (pow(2, streamMsg->window())=%f\n", mNumOutstandingBytes,
	                                                                     (pow(2, streamMsg->window())));

	if ( (int) (pow(2, streamMsg->window()) - mNumOutstandingBytes) > 0 ) {
	  assert( pow(2, streamMsg->window()) - mNumOutstandingBytes > 0);
	  mTransmitWindowSize = pow(2, streamMsg->window()) - mNumOutstandingBytes;
	}
	else {
	  mTransmitWindowSize = 0;
	}
        
	printf("REMOVED ACKED PACKET. Offset: %d, mTransmitWindowSize=%d\n", (int) mChannelToBufferMap[offset]->mOffset,(int) mTransmitWindowSize);
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
    else if (streamMsg->type() == streamMsg->DATA || streamMsg->type() == streamMsg->INIT) {
      assert ( pow(2, streamMsg->window()) - mNumOutstandingBytes > 0);
      mTransmitWindowSize = pow(2, streamMsg->window()) - mNumOutstandingBytes;

      printf("offset=%d,  mLastContiguousByteReceived=%d, mNextByteExpected=%d, mLastByteReceived=%d\n", (int)offset,  (int)mLastContiguousByteReceived, (int)mNextByteExpected, (int)mLastByteReceived);

      if ( (int)(offset) == mNextByteExpected) {
        uint32 offsetInBuffer = offset - mLastContiguousByteReceived - 1;
        if (offsetInBuffer + len <= MAX_RECEIVE_WINDOW) {
	  mReceiveWindowSize -= len;

	  memcpy(mReceiveBuffer+offsetInBuffer, buffer, len);
	  memset(mReceiveBitmap+offsetInBuffer, 1, len);
          
          uint32 readyBufferSize = len;

	  for (uint i=len; i < MAX_RECEIVE_WINDOW; i++) {
	    if (mReceiveBitmap[i] == 1) {
	      readyBufferSize++;
	    }
	    else if (mReceiveBitmap[i] == 0) {
	      break;
	    }
	  }

	  //pass data up to the app from 0 to readyBufferSize;
          //
	  if (mReadCallback != NULL) {
	    mReadCallback(mReceiveBuffer, readyBufferSize);
	  }

          printf("offset=%d, offsetInBuffer=%d, readyBufferSize=%d, mLastContiguousByteReceived=%d, mNextByteExpected=%d, mLastByteReceived=%d\n", (int)offset, (int)offsetInBuffer, (int)readyBufferSize, (int)mLastContiguousByteReceived, (int)mNextByteExpected, (int)mLastByteReceived);
 	  fflush(stdout);

	  //now move the window forward...
	  mLastContiguousByteReceived = mLastContiguousByteReceived + readyBufferSize;
          mNextByteExpected = mLastContiguousByteReceived + 1;
          mLastByteReceived = ( (int)(offset + len - 1) > mLastByteReceived ) ? offset+len -1 : mLastByteReceived;

          memset(mReceiveBitmap, 0, readyBufferSize);
          memmove(mReceiveBitmap, mReceiveBitmap + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);
	  memmove(mReceiveBuffer, mReceiveBuffer + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);

	  mReceiveWindowSize += readyBufferSize;

	  //send back an ack.
          sendAckPacket();
          mReceivedSegments[offset] = 1;
          printf("On %d, mReceivedSegments.size() = %d\n", (int) mLSID, (int)mReceivedSegments.size());
          printf("Received Segment at offset = %d\n", (int) offset);	  
	}
        else {
           //dont ack this packet.. its falling outside the receive window.
        }
      }
      else {        

        int32 offsetInBuffer = offset - mLastContiguousByteReceived - 1;	

	std::cout << offsetInBuffer << "  ,  " << offsetInBuffer+len << "\n";

	if ( (int)(offset+len-1) <= (int)mLastContiguousByteReceived) {
	  printf("Acking packet which we had already received previously\n");
	  sendAckPacket();
	}
        else if (offsetInBuffer + len <= MAX_RECEIVE_WINDOW) {
	  assert (offsetInBuffer + len > 0);

          mReceiveWindowSize -= len;

   	  memcpy(mReceiveBuffer+offsetInBuffer, buffer, len);
	  memset(mReceiveBitmap+offsetInBuffer, 1, len);

          mLastByteReceived = ( (int) (offset+len-1)  > mLastByteReceived ) ? offset + len - 1 : mLastByteReceived;

          sendAckPacket();
          mReceivedSegments[offset] = 1;
          printf("On %d, mReceivedSegments.size() = %d\n", (int)mLSID , (int)mReceivedSegments.size() );
          printf("Received Non-Contiguous Segment at offset = %d\n", (int) offset);
	}	
	else{
	  //dont ack this packet; its falling outside the receive window.
	}
      }
    }
  }

  LSID getLSID() {
    return mLSID;
  }

  void sendInitPacket(void* data, uint32 len) {
    printf("Sending Init packet\n");

    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.INIT); 
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );

    if (mParentStream != NULL) {
      sstMsg.set_psid(mParentStream->getLSID());
    }
    else {
      sstMsg.set_psid(0);
    }
    sstMsg.set_bsn(0);    

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection.lock()->sendData( buffer.data(), buffer.size() );
  }

  void sendAckPacket() {
    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.ACK);
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );

    printf("Sending Ack packet with window %d\n", (int)sstMsg.window());

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection.lock()->sendData(  buffer.data(), buffer.size());
  } 

  uint64 sendDataPacket(const void* data, uint32 len, uint32 offset) {
    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.DATA);
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );
    
    sstMsg.set_bsn(offset);

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    return mConnection.lock()->sendData(  buffer.data(), buffer.size());
  }

  void sendReplyPacket(void* data, uint32 len, LSID remoteLSID) {
    printf("Sending Reply packet\n");
    
    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.REPLY); 
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );
    sstMsg.set_rsid(remoteLSID);
    sstMsg.set_bsn(0);    

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection.lock()->sendData(  buffer.data(), buffer.size());
  }
  
  uint8 mState;

  uint32 mNumBytesSent;

  Stream<EndPointType>* mParentStream;
  boost::weak_ptr<Connection<EndPointType> > mConnection;


  std::map<uint64, boost::shared_ptr<StreamBuffer> >  mChannelToBufferMap;

  std::deque< boost::shared_ptr<StreamBuffer> > mQueuedBuffers;
  uint32 mCurrentQueueLength;
  
  USID mUSID;
  LSID mLSID;

  uint16 MAX_PAYLOAD_SIZE;
  uint32 MAX_QUEUE_LENGTH;
  uint32 MAX_RECEIVE_WINDOW;

  boost::mutex mQueueMutex;

  boost::asio::io_service mIOService;

  std::map<uint64, int> mReceivedSegments;

  Thread* mThread;

  bool mLooping;

  boost::asio::deadline_timer mRetransmitTimer;

  uint32 mTransmitWindowSize;
  uint32 mReceiveWindowSize;
  uint32 mNumOutstandingBytes;

  int64 mNextByteExpected;
  int64 mLastContiguousByteReceived;
  int64 mLastByteReceived;

  uint8* mReceiveBuffer;
  uint8* mReceiveBitmap;
  

  ReadCallback mReadCallback;
  StreamReturnCallbackFunction mStreamReturnCallback;


  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction> StreamReturnCallbackMap;  
  static StreamReturnCallbackMap mStreamReturnCallbackMap;
  static Mutex mStreamCreationMutex;
  

  /* Variables required for the initial connection */  
  bool mConnected;  
  
  uint8* mInitialData;
  uint16 mInitialDataLength;
  uint8 mNumInitRetransmissions;
  uint8 MAX_INIT_RETRANSMISSIONS;

  friend class Connection<EndPointType>;

};

}

#endif
