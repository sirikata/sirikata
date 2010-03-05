/*  cbr
 *  SSTImpl.hpp
 *
 *  Copyright (c) 2009, Tahir Azim.
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
 *  * Neither the name of cbr nor the names of its contributors may
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


#ifndef SST_IMPL_HPP
#define SST_IMPL_HPP

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>

#include "oh/ObjectHostContext.hpp"
#include "Message.hpp"

#include "Utility.hpp"
#include <bitset>

#include <sirikata/util/SerializationCheck.hpp>


namespace CBR {

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

  Mutex() {

  }

  Mutex(const Mutex& mutex) {  }

  boost::mutex& getMutex() {
    return mMutex;
  }

private:
  boost::mutex mMutex;

};

template <class EndPointType>
class Connection;

template <class EndPointType>
class Stream;

#define MESSAGE_ID_SERVER_SHIFT 52
#define MESSAGE_ID_SERVER_BITS 0xFFF0000000000000LL

template <typename EndPointType>
class BaseDatagramLayer:public ObjectMessageRecipient
{
private:

  BaseDatagramLayer(ObjectMessageRouter* router, ObjectMessageDispatcher* dispatcher) :
                    mRouter(router), mDispatcher(dispatcher) { }

  uint64 generateUniqueID(const ServerID& origin) {
    static uint64 sIDSource = 0;

    uint64 id_src = sIDSource++;
    uint64 message_id_server_bits=MESSAGE_ID_SERVER_BITS;
    uint64 server_int = (uint64)origin;
    uint64 server_shifted = server_int << MESSAGE_ID_SERVER_SHIFT;
    assert( (server_shifted & ~message_id_server_bits) == 0 );
    return (server_shifted & message_id_server_bits) | (id_src & ~message_id_server_bits);
  }

public:
  static boost::shared_ptr<BaseDatagramLayer<EndPointType> > getDatagramLayer(EndPointType endPoint) {
    if (mDatagramLayerMap.find(endPoint) != mDatagramLayerMap.end()) {
      return mDatagramLayerMap[endPoint];
    }

    return boost::shared_ptr<BaseDatagramLayer<EndPointType> > ();
  }

  static boost::shared_ptr<BaseDatagramLayer<EndPointType> > createDatagramLayer(
								  EndPointType endPoint,
                                                                  ObjectMessageRouter* router,
	 							  ObjectMessageDispatcher* dispatcher)
  {
    if (mDatagramLayerMap.find(endPoint) != mDatagramLayerMap.end()) {
      return mDatagramLayerMap[endPoint];
    }

    boost::shared_ptr<BaseDatagramLayer<EndPointType> > datagramLayer =
                                            boost::shared_ptr<BaseDatagramLayer<EndPointType> >
                                                           (new BaseDatagramLayer(router, dispatcher));

    mDatagramLayerMap[endPoint] = datagramLayer;

    return datagramLayer;
  }

  static void listen(EndPoint<EndPointType>& listeningEndPoint) {
    EndPointType endPointID = listeningEndPoint.endPoint;

    mDatagramLayerMap[endPointID]->dispatcher()->registerObjectMessageRecipient(
							     listeningEndPoint.port,
							     mDatagramLayerMap[endPointID].get());
  }

  void send(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
    boost::mutex::scoped_lock lock(mMutex);

    CBR::Protocol::Object::ObjectMessage* objectMessage = new CBR::Protocol::Object::ObjectMessage();
    objectMessage->set_source_object(src->endPoint);
    objectMessage->set_source_port(src->port);
    objectMessage->set_dest_object(dest->endPoint);
    objectMessage->set_dest_port(dest->port);
    objectMessage->set_unique(generateUniqueID(0));
    objectMessage->set_payload(String((char*) data, len));

    bool val = mRouter->route(  objectMessage  );
  }

  /* This function will also have to be re-implemented to support receiving
     using other kinds of packets. For now, I'll implement only for ODP. */
  virtual void receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg)  {
    Connection<EndPointType>::handleReceive(mRouter,
                                  EndPoint<UUID> (msg.source_object(), msg.source_port()),
                                  EndPoint<UUID> (msg.dest_object(), msg.dest_port()),
                                  (void*) msg.payload().data(), msg.payload().size() );
  }

  ObjectMessageDispatcher* dispatcher() {
    return mDispatcher;
  }

private:
  ObjectMessageRouter* mRouter;
  ObjectMessageDispatcher* mDispatcher;

  boost::mutex mMutex;

  static std::map<EndPointType, boost::shared_ptr<BaseDatagramLayer<EndPointType> > > mDatagramLayerMap;
};



typedef std::tr1::function< void(int, boost::shared_ptr< Connection<UUID> > ) > ConnectionReturnCallbackFunction;

typedef std::tr1::function< void(int, boost::shared_ptr< Stream<UUID> >) >  StreamReturnCallbackFunction;
typedef std::tr1::function< void (int, void*) >  DatagramSendDoneCallback;

typedef std::tr1::function<void (uint8*, int) >  ReadDatagramCallback;

typedef std::tr1::function<void (uint8*, int) > ReadCallback;

typedef UUID USID;

typedef uint16 LSID;

#define SUCCESS 0
#define FAILURE -1

class ChannelSegment {
public:

  uint8* mBuffer;
  uint16 mBufferLength;
  uint64 mChannelSequenceNumber;
  uint64 mAckSequenceNumber;

  Time mTransmitTime;
  Time mAckTime;

  ChannelSegment( const void* data, int len, uint64 channelSeqNum, uint64 ackSequenceNum) :
                                               mBufferLength(len),
					      mChannelSequenceNumber(channelSeqNum),
					      mAckSequenceNumber(ackSequenceNum),
					      mTransmitTime(Time::null()), mAckTime(Time::null())
  {
    mBuffer = new uint8[len];
    memcpy( mBuffer, (const uint8*) data, len);
  }

  ~ChannelSegment() {
    delete [] mBuffer;
  }

  void setAckTime(Time& ackTime) {
    mAckTime = ackTime;
  }

};


template <class EndPointType>
class Connection : public ObjectMessageRecipient {

private:
  friend class Stream<EndPointType>;

  typedef std::map<EndPoint<EndPointType>, boost::shared_ptr<Connection> >  ConnectionMap;
  typedef std::map<EndPoint<EndPointType>, ConnectionReturnCallbackFunction>  ConnectionReturnCallbackMap;
  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction>   StreamReturnCallbackMap;


  static ConnectionMap mConnectionMap;
  static std::bitset<65536> mAvailableChannels;
  static uint16 mLastAssignedPort;

  static ConnectionReturnCallbackMap mConnectionReturnCallbackMap;
  static StreamReturnCallbackMap  mListeningConnectionsCallbackMap;

  static Mutex mStaticMembersLock;

  EndPoint<EndPointType> mLocalEndPoint;
  EndPoint<EndPointType> mRemoteEndPoint;

  boost::shared_ptr<BaseDatagramLayer<UUID> > mDatagramLayer;

  int mState;
  uint16 mRemoteChannelID;
  uint16 mLocalChannelID;

  uint64 mTransmitSequenceNumber;
  uint64 mLastReceivedSequenceNumber;   //the last transmit sequence number received from the other side

  std::map<LSID, boost::shared_ptr< Stream<EndPointType> > > mOutgoingSubstreamMap;
  std::map<LSID, boost::shared_ptr< Stream<EndPointType> > > mIncomingSubstreamMap;

  std::map<uint16, StreamReturnCallbackFunction> mListeningStreamsCallbackMap;
  std::map<uint16, std::vector<ReadDatagramCallback> > mReadDatagramCallbacks;

  uint16 mNumStreams;

  std::deque< boost::shared_ptr<ChannelSegment> > mQueuedSegments;
  std::deque< boost::shared_ptr<ChannelSegment> > mOutstandingSegments;

  uint16 mCwnd;
  int64 mRTOMicroseconds; // RTO in microseconds
  bool mFirstRTO;

  boost::mutex mQueueMutex;

  uint16 MAX_DATAGRAM_SIZE;
  uint16 MAX_PAYLOAD_SIZE;
  uint32 MAX_QUEUED_SEGMENTS;
  float  CC_ALPHA;
  Time mLastTransmitTime;

  boost::weak_ptr<Connection<EndPointType> > mWeakThis;

  google::protobuf::LogSilencer logSilencer;

private:

  Connection(ObjectMessageRouter* router, EndPoint<EndPointType> localEndPoint,
             EndPoint<EndPointType> remoteEndPoint)
    : mLocalEndPoint(localEndPoint), mRemoteEndPoint(remoteEndPoint),
      mState(CONNECTION_DISCONNECTED),
      mRemoteChannelID(0), mLocalChannelID(1), mTransmitSequenceNumber(1),
      mLastReceivedSequenceNumber(1),
      mNumStreams(0), mCwnd(1), mRTOMicroseconds(1000000),
      mFirstRTO(true),  MAX_DATAGRAM_SIZE(1000), MAX_PAYLOAD_SIZE(1300),
      MAX_QUEUED_SEGMENTS(300),
      CC_ALPHA(0.8), mLastTransmitTime(Time::null()), inSendingMode(true), numSegmentsSent(0)
  {
    mDatagramLayer = BaseDatagramLayer<EndPointType>::getDatagramLayer(localEndPoint.endPoint);
    mDatagramLayer->dispatcher()->registerObjectMessageRecipient(localEndPoint.port, this);
  }

  void sendSSTChannelPacket(CBR::Protocol::SST::SSTChannelHeader& sstMsg) {
    if (mState == CONNECTION_DISCONNECTED) return;

    std::string buffer = serializePBJMessage(sstMsg);
    mDatagramLayer->send(&mLocalEndPoint, &mRemoteEndPoint, (void*) buffer.data(),
				       buffer.size());
  }

  static uint16 getAvailablePort() {
    if (mLastAssignedPort <= 2048) {
      mLastAssignedPort = 65530;
    }

    return (mLastAssignedPort--);
  }

  /* Returns -1 if no channel is available. Otherwise returns the lowest
     available channel. */
  static int getAvailableChannel() {
    //TODO: faster implementation.
    for (uint i=1; i<mAvailableChannels.size(); i++) {
      if (!mAvailableChannels.test(i)) {
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

  bool inSendingMode; uint16 numSegmentsSent;

  bool serviceConnection(const Time& curTime) { 
    // should start from ssthresh, the slow start lower threshold, but starting
    // from 1 for now. Still need to implement slow start.
    if (mState == CONNECTION_DISCONNECTED) return false;

    //firstly, service the streams
    for (typename std::map<LSID, boost::shared_ptr< Stream<EndPointType> > >::iterator it=mOutgoingSubstreamMap.begin();
	 it != mOutgoingSubstreamMap.end(); ++it)
    {
      if ( (it->second->serviceStream(curTime)) == false) {
	return false;
      }
    }

    if (inSendingMode) {
      boost::unique_lock<boost::mutex> lock(mQueueMutex);

      numSegmentsSent = 0;

      for (int i = 0; (!mQueuedSegments.empty()) && i < mCwnd; i++) {
	  boost::shared_ptr<ChannelSegment> segment = mQueuedSegments.front();

	  CBR::Protocol::SST::SSTChannelHeader sstMsg;
	  sstMsg.set_channel_id( mRemoteChannelID );
	  sstMsg.set_transmit_sequence_number(segment->mChannelSequenceNumber);
	  sstMsg.set_ack_count(1);
	  sstMsg.set_ack_sequence_number(segment->mAckSequenceNumber);

	  sstMsg.set_payload(segment->mBuffer, segment->mBufferLength);

	  /*printf("%s sending packet from data sending loop to %s\n",
	    mLocalEndPoint.endPoint.readableHexData().c_str()
	    , mRemoteEndPoint.endPoint.readableHexData().c_str());*/

	  sendSSTChannelPacket(sstMsg);

	  segment->mTransmitTime = curTime;
	  mOutstandingSegments.push_back(segment);

	  mQueuedSegments.pop_front();

	  numSegmentsSent++;

	  mLastTransmitTime = curTime;

	  inSendingMode = false;  
      }
    }
    else { 
      if ( (curTime - mLastTransmitTime).toMicroseconds() < mRTOMicroseconds)
      {
	return true;
      }

      if (mState == CONNECTION_PENDING_CONNECT) {
	return false;
      }

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
	    mRTOMicroseconds = ((segment->mAckTime - segment->mTransmitTime).toMicroseconds()) ;
	    mFirstRTO = false;
	  }
	  else {
	    mRTOMicroseconds = CC_ALPHA * mRTOMicroseconds +
	      (1.0-CC_ALPHA) * (segment->mAckTime - segment->mTransmitTime).toMicroseconds();
	  }
	}
      }

      //printf("mRTOMicroseconds=%d\n", (int)mRTOMicroseconds);

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

      inSendingMode = true;
    }

    return true;
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

  static bool createConnection(ObjectMessageRouter* router,
			       EndPoint <EndPointType> localEndPoint,
			       EndPoint <EndPointType> remoteEndPoint,
                               ConnectionReturnCallbackFunction cb,
			       StreamReturnCallbackFunction scb)

  {
    boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());

    if (mConnectionMap.find(localEndPoint) != mConnectionMap.end()) {
      std::cout << "mConnectionMap.find failed for " << localEndPoint.endPoint.toString() << "\n";

      return false;
    }

    boost::shared_ptr<Connection>  conn =  boost::shared_ptr<Connection> (
						        new Connection(router, localEndPoint, remoteEndPoint));
    conn->mWeakThis = conn;
    mConnectionMap[localEndPoint] = conn;

    conn->setState(CONNECTION_PENDING_CONNECT);

    uint16 payload[1];
    payload[0] = getAvailableChannel();

    conn->setLocalChannelID(payload[0]);
    conn->sendData(payload, sizeof(payload));

    mConnectionReturnCallbackMap[localEndPoint] = cb;

    return true;
  }

  static bool listen(StreamReturnCallbackFunction cb, EndPoint<EndPointType> listeningEndPoint) {
    BaseDatagramLayer<EndPointType>::listen(listeningEndPoint);

    boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());

    if (mListeningConnectionsCallbackMap.find(listeningEndPoint) != mListeningConnectionsCallbackMap.end()){
      return false;
    }

    mListeningConnectionsCallbackMap[listeningEndPoint] = cb;

    return true;
  }

  void listenStream(uint16 port, StreamReturnCallbackFunction scb) {
    mListeningStreamsCallbackMap[port] = scb;
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
  virtual void stream(StreamReturnCallbackFunction cb, void* initial_data, int length,
		      uint16 local_port, uint16 remote_port)
  {
    stream(cb, initial_data, length, local_port, remote_port, NULL);
  }

  virtual void stream(StreamReturnCallbackFunction cb, void* initial_data, int length,
                      uint16 local_port, uint16 remote_port, LSID parentLSID)
  {
    USID usid = createNewUSID();
    LSID lsid = ++mNumStreams;

    boost::shared_ptr<Stream<EndPointType> > stream =
      boost::shared_ptr<Stream<EndPointType> >
      ( new Stream<EndPointType>(parentLSID, mWeakThis, local_port, remote_port,  usid, lsid,
				 initial_data, length, false, 0, cb) );

    mOutgoingSubstreamMap[lsid]=stream;
  }

  uint64 sendData(const void* data, uint32 length) {
    boost::lock_guard<boost::mutex> lock(mQueueMutex);

    assert(length <= MAX_PAYLOAD_SIZE);

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

    delete stream_msg;

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
    for (uint i = 0; i < mOutstandingSegments.size(); i++) {
      if (mOutstandingSegments[i]->mChannelSequenceNumber == receivedAckNum) {
	mOutstandingSegments[i]->mAckTime = Timer::now();
      }
    }
  }

  virtual void receiveMessage(const CBR::Protocol::Object::ObjectMessage& msg)  {
    receiveMessage((void*) msg.payload().data(), msg.payload().size() );
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

      EndPoint<EndPointType> originalListeningEndPoint(mRemoteEndPoint.endPoint, mRemoteEndPoint.port);

      uint16* received_payload = (uint16*) received_msg->payload().data();

      setRemoteChannelID(received_payload[0]);
      mRemoteEndPoint.port = received_payload[1];

      sendData( received_payload, 0 );

      if (mConnectionReturnCallbackMap.find(mLocalEndPoint) != mConnectionReturnCallbackMap.end())
      {
	if (mConnectionMap.find(mLocalEndPoint) != mConnectionMap.end()) {
	  boost::shared_ptr<Connection> conn = mConnectionMap[mLocalEndPoint];

	  mConnectionReturnCallbackMap[mLocalEndPoint] (SUCCESS, conn);
	}
        mConnectionReturnCallbackMap.erase(mLocalEndPoint);
      }
    }
    else if (mState == CONNECTION_PENDING_RECEIVE_CONNECT) {
      mState = CONNECTION_CONNECTED;
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
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
      if (mListeningStreamsCallbackMap.find(received_stream_msg->dest_port()) !=
	  mListeningStreamsCallbackMap.end())
      {
	//create a new stream
	USID usid = createNewUSID();
	LSID newLSID = ++mNumStreams;

	boost::shared_ptr<Stream<EndPointType> > stream =
	  boost::shared_ptr<Stream<EndPointType> >
	  (new Stream<EndPointType> (received_stream_msg->psid(), mWeakThis,
				     received_stream_msg->dest_port(),
				     received_stream_msg->src_port(),
				     usid, newLSID,
				     NULL, 0, true, incomingLsid, NULL));
	mOutgoingSubstreamMap[newLSID] = stream;
	mIncomingSubstreamMap[incomingLsid] = stream;

	mListeningStreamsCallbackMap[received_stream_msg->dest_port()](0, stream);

	stream->receiveData(received_stream_msg, received_stream_msg->payload().data(),
			    received_stream_msg->bsn(),
			    received_stream_msg->payload().size() );
      }
      else {
	std::cout << "Not listening to streams at: " << received_stream_msg->dest_port() << "\n";
      }
    }
  }

  void handleReplyPacket(CBR::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
      LSID initiatingLSID = received_stream_msg->rsid();

      if (mOutgoingSubstreamMap.find(initiatingLSID) != mOutgoingSubstreamMap.end()) {
	boost::shared_ptr< Stream<EndPointType> > stream = mOutgoingSubstreamMap[initiatingLSID];
	mIncomingSubstreamMap[incomingLsid] = stream;

	if (stream->mStreamReturnCallback != NULL){
	  stream->mStreamReturnCallback(SUCCESS, stream);
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
    //printf("ACK received : offset = %d\n", (int)received_channel_msg->ack_sequence_number() );
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
    uint8* payload = (uint8*) received_stream_msg->payload().data();
    uint payload_size = received_stream_msg->payload().size();

    uint16 dest_port = received_stream_msg->dest_port();

    std::vector<ReadDatagramCallback> datagramCallbacks;

    if (mReadDatagramCallbacks.find(dest_port) != mReadDatagramCallbacks.end()) {
      datagramCallbacks = mReadDatagramCallbacks[dest_port];
    }

    for (uint i=0 ; i < datagramCallbacks.size(); i++) {
      datagramCallbacks[i](payload, payload_size);
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

  uint64 getRTOMicroseconds() {
    return mRTOMicroseconds;
  }

public:

  ~Connection() {
    //printf("Connection on %s getting destroyed\n", mLocalEndPoint.endPoint.readableHexData().c_str());

    mDatagramLayer->dispatcher()->unregisterObjectMessageRecipient(mLocalEndPoint.port, this);


    if (mState != CONNECTION_DISCONNECTED) {
      // Setting mState to CONNECTION_DISCONNECTED implies close() is being
      //called from the destructor.
      mState = CONNECTION_DISCONNECTED;
      close(true);


    }
  }

  static void closeConnections() {
    mConnectionMap.clear();
  }

  static void service() {
    Time curTime = Timer::now();    

    for (typename ConnectionMap::iterator it = mConnectionMap.begin();
	 it != mConnectionMap.end();
	 ++it)
    {
      boost::shared_ptr<Connection<EndPointType> > conn = it->second;
      int connState = conn->mState;

      bool cleanupConnection = conn->serviceConnection(curTime);

      if (!cleanupConnection) {

	if (connState == CONNECTION_PENDING_CONNECT) {

	  ConnectionReturnCallbackFunction cb = mConnectionReturnCallbackMap[conn->localEndPoint()];
	  boost::shared_ptr<Connection>  failed_conn = it->second;

	  mConnectionReturnCallbackMap[conn->localEndPoint()] (FAILURE, conn);

	  mConnectionReturnCallbackMap.erase(conn->localEndPoint());

	  mConnectionMap.erase(it);

	  cb(FAILURE, failed_conn);
	}

	break;
      }
    }
  }

  /* Sends the specified data buffer using best-effort datagrams on the
     underlying connection. This may be done using an ephemeral stream
     on top of the underlying connection or some other mechanism (e.g.
     datagram packets sent directly on the underlying  connection).

     @param data the buffer to send
     @param length the length of the buffer
     @param local_port the source port
     @param remote_port the destination port
     @param DatagramSendDoneCallback a callback of type
                                     void (int errCode, void*)
                                     which is called when queuing
                                     the datagram failed or succeeded.
                                     'errCode' contains SUCCESS or FAILURE
                                     while the 'void*' argument is a pointer
                                     to the buffer that was being sent.

     @return false if there's an immediate failure while enqueuing the datagram; true, otherwise.
  */
  virtual bool datagram( void* data, int length, uint16 local_port, uint16 remote_port,
			 DatagramSendDoneCallback cb) {
    int currOffset = 0;

    if (mState == CONNECTION_DISCONNECTED
     || mState == CONNECTION_PENDING_DISCONNECT)
    {
      if (cb != NULL) {
        cb(FAILURE, data);
      }
      return false;
    }

    LSID lsid = ++mNumStreams;

    while (currOffset < length) {
      int buffLen = (length-currOffset > MAX_PAYLOAD_SIZE) ?
	            MAX_PAYLOAD_SIZE :
           	    (length-currOffset);

      CBR::Protocol::SST::SSTStreamHeader sstMsg;
      sstMsg.set_lsid( lsid );
      sstMsg.set_type(sstMsg.DATAGRAM);
      sstMsg.set_flags(0);
      sstMsg.set_window( 1024 );
      sstMsg.set_src_port(local_port);
      sstMsg.set_dest_port(remote_port);

      sstMsg.set_payload( ((uint8*)data)+currOffset, buffLen);

      std::string buffer = serializePBJMessage(sstMsg);
      sendData(  buffer.data(), buffer.size() );

      currOffset += buffLen;
    }

    if (cb != NULL) {
      //invoke the callback function
      cb(SUCCESS, data);
    }

    return true;
  }

  /*
    Register a callback which will be called when there is a datagram
    available to be read.

    @param port the local port on which to listen for datagrams.
    @param ReadDatagramCallback a function of type "void (uint8*, int)"
           which will be called when a datagram is available. The
           "uint8*" field will be filled up with the received datagram,
           while the 'int' field will contain its size.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadDatagramCallback(uint16 port, ReadDatagramCallback cb) {
    if (mReadDatagramCallbacks.find(port) == mReadDatagramCallbacks.end()) {
      mReadDatagramCallbacks[port] = std::vector<ReadDatagramCallback>();
    }

    mReadDatagramCallbacks[port].push_back(cb);

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
    /* (mState != CONNECTION_DISCONNECTED) implies close() wasnt called
       through the destructor. */
    if (mState != CONNECTION_DISCONNECTED) {
      boost::mutex::scoped_lock lock(mStaticMembersLock.getMutex());
      mConnectionMap.erase(mLocalEndPoint);
    }

    if (force) {
      mState = CONNECTION_DISCONNECTED;
    }
    else  {
      mState = CONNECTION_PENDING_DISCONNECT;
    }
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

  static void handleReceive(ObjectMessageRouter* router, EndPoint<EndPointType> remoteEndPoint,
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

      if (mListeningConnectionsCallbackMap.find(localEndPoint) != mListeningConnectionsCallbackMap.end()) {
        uint8* received_payload = (uint8*) received_msg->payload().data();

        uint16 payload[2];
        payload[0] = getAvailableChannel();
        payload[1] = getAvailablePort();

        EndPoint<EndPointType> newLocalEndPoint(localEndPoint.endPoint, payload[1]);
        boost::shared_ptr<Connection>  conn =
                   boost::shared_ptr<Connection>(
				    new Connection(router, newLocalEndPoint, remoteEndPoint));

	conn->listenStream(newLocalEndPoint.port, mListeningConnectionsCallbackMap[localEndPoint]);
        conn->mWeakThis = conn;
        mConnectionMap[newLocalEndPoint] = conn;

        conn->setLocalChannelID(payload[0]);
        conn->setRemoteChannelID(received_payload[0]);
        conn->setState(CONNECTION_PENDING_RECEIVE_CONNECT);

        conn->sendData(payload, sizeof(payload));
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

  const uint8* mBuffer;
  uint16 mBufferLength;
  uint32 mOffset;

  Time mTransmitTime;
  Time mAckTime;


  StreamBuffer(const uint8* data, int len, int offset) :
    mTransmitTime(Time::null()), mAckTime(Time::null())
  {
    assert(len > 0);

    mBuffer = data;

    mBufferLength = len;
    mOffset = offset;
  }

  ~StreamBuffer() {

  }
};

template <class EndPointType>
class Stream  {
public:

   enum StreamStates {
       DISCONNECTED = 1,
       CONNECTED=2,
       PENDING_DISCONNECT=3,
       PENDING_CONNECT=4,
     };



  ~Stream() {
    close(true);

    delete [] mInitialData;
    delete [] mReceiveBuffer;
    delete [] mReceiveBitmap;
  }


  static bool connectStream(ObjectMessageRouter* router,
			    EndPoint <EndPointType> localEndPoint,
			    EndPoint <EndPointType> remoteEndPoint,
			    StreamReturnCallbackFunction cb)
  {
    //boost::mutex::scoped_lock lock(mStreamCreationMutex.getMutex());

    if (mStreamReturnCallbackMap.find(localEndPoint) != mStreamReturnCallbackMap.end()) {
      return false;
    }

    mStreamReturnCallbackMap[localEndPoint] = cb;


    bool result = Connection<EndPointType>::createConnection(router, localEndPoint,
							     remoteEndPoint,
							     connectionCreated, cb);

    return result;
  }

  /*
    Start listening for top-level streams on the specified end-point. When
    a new top-level stream connects at the given endpoint, the specified
    callback function is invoked handing the object a top-level stream.
    @param cb the callback function invoked when a new stream is created
    @param listeningEndPoint the endpoint where SST will accept new incoming
           streams.
    @return false, if its not possible to listen to this endpoint (e.g. if listen
            has already been called on this endpoint); true otherwise.
  */
  static bool listen(StreamReturnCallbackFunction cb, EndPoint <EndPointType> listeningEndPoint) {
    return Connection<EndPointType>::listen(cb, listeningEndPoint);
  }

  /*
    Start listening for child streams on the specified port. A remote stream
    can only create child streams under this stream if this stream is listening
    on the port specified for the child stream.

    @param scb the callback function invoked when a new stream is created
    @param port the endpoint where SST will accept new incoming
           streams.
  */
  void listenSubstream(uint16 port, StreamReturnCallbackFunction scb) {
    mConnection.lock()->listenStream(port, scb);
  }


  /* Writes data bytes to the stream. If not all bytes can be transmitted
     immediately, they are queued locally until ready to transmit.
     @param data the buffer containing the bytes to be written
     @param len the length of the buffer
     @return the number of bytes written or enqueued, or -1 if an error
             occurred
  */
  virtual int write(const uint8* data, int len) {
    if (mState == DISCONNECTED || mState == PENDING_DISCONNECT) {
      return -1;
    }

    boost::lock_guard<boost::mutex> lock(mQueueMutex);
    int count = 0;



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

	if (mCurrentQueueLength + buffLen > MAX_QUEUE_LENGTH) {
	  return currOffset;
	}

	mQueuedBuffers.push_back( boost::shared_ptr<StreamBuffer>(new StreamBuffer(data+currOffset, buffLen, mNumBytesSent)) );
	currOffset += buffLen;
	mCurrentQueueLength += buffLen;
	mNumBytesSent += buffLen;

	count++;
      }


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

    boost::mutex::scoped_lock lock(mReceiveBufferMutex);
    sendToApp(0);

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
      mConnected = false;
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

     @param data A pointer to the initial data buffer that needs to be sent on this stream.
         Having this pointer removes the need for the application to enqueue data
         until the stream is actually created.
     @param port The length of the data buffer.
     @param local_port the local port to which the child stream will be bound.
     @param remote_port the remote port to which the child stream should connect.
     @param StreamReturnCallbackFunction A callback function which will be called once the
                                  stream is created and the initial data queued up
                                  (or actually sent?). The function will provide  a
                                  reference counted, shared pointer to the  connection.
  */
  virtual void createChildStream(StreamReturnCallbackFunction cb, void* data, int length,
				 uint16 local_port, uint16 remote_port)
  {
    mConnection.lock()->stream(cb, data, length, local_port, remote_port, mParentLSID);
  }

  /*
    Returns the local endpoint to which this connection is bound.

    @return the local endpoint.
  */
  virtual EndPoint <EndPointType> localEndPoint()  {
    return EndPoint<EndPointType> (mConnection.lock()->localEndPoint().endPoint, mLocalPort);
  }

  /*
    Returns the remote endpoint to which this connection is bound.

    @return the remote endpoint.
  */
  virtual EndPoint <EndPointType> remoteEndPoint()  {
    return EndPoint<EndPointType> (mConnection.lock()->remoteEndPoint().endPoint, mRemotePort);
  }

private:
  Stream(LSID parentLSID, boost::weak_ptr<Connection<EndPointType> > conn,
	 uint16 local_port, uint16 remote_port,
	 USID usid, LSID lsid, void* initial_data, uint32 length,
	 bool remotelyInitiated, LSID remoteLSID, StreamReturnCallbackFunction cb)
    :
    mState(PENDING_CONNECT),
    mLocalPort(local_port),
    mRemotePort(remote_port),
    mParentLSID(parentLSID),
    mConnection(conn),
    mUSID(usid),
    mLSID(lsid),
    MAX_PAYLOAD_SIZE(1000),
    MAX_QUEUE_LENGTH(4000000),
    MAX_RECEIVE_WINDOW(15000),
    mStreamRTOMicroseconds(200000),
    FL_ALPHA(0.8),
    mTransmitWindowSize(MAX_RECEIVE_WINDOW),
    mReceiveWindowSize(MAX_RECEIVE_WINDOW),
    mNumOutstandingBytes(0),
    mNextByteExpected(0),
    mLastContiguousByteReceived(-1),
    mLastSendTime(Time::null()),
    mStreamReturnCallback(cb),
    mConnected (false),
    MAX_INIT_RETRANSMISSIONS(5)
  {
    if (remotelyInitiated) {
      mConnected = true;
      mState = CONNECTED;
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
    mCurrentQueueLength = 0;

    if (remotelyInitiated) {
      sendReplyPacket(initial_data, mInitialDataLength, remoteLSID);
    }
    else {
      sendInitPacket(initial_data, mInitialDataLength);
    }

    mNumInitRetransmissions = 1;
    mNumBytesSent = mInitialDataLength;

    if (length > mInitialDataLength) {
      write( ((uint8*)initial_data) + mInitialDataLength, length - mInitialDataLength);
    }
  }

  static void connectionCreated( int errCode, boost::shared_ptr<Connection<EndPointType> > c) {
    //boost::mutex::scoped_lock lock(mStreamCreationMutex.getMutex());

    if (errCode != SUCCESS) {

      StreamReturnCallbackFunction cb = mStreamReturnCallbackMap[c->localEndPoint()];
      mStreamReturnCallbackMap.erase(c->localEndPoint());

      cb(FAILURE, boost::shared_ptr<Stream<EndPointType> >() );

      return;
    }

    int length = 0;
    uint8* f = new uint8[length];
    for (int i=0; i<length; i++) {
      f[i] = i % 255;
    }

    assert(mStreamReturnCallbackMap.find(c->localEndPoint()) != mStreamReturnCallbackMap.end());

    c->stream(mStreamReturnCallbackMap[c->localEndPoint()], f , length,
	      c->localEndPoint().port, c->remoteEndPoint().port);

    mStreamReturnCallbackMap.erase(c->localEndPoint());
  }


  bool serviceStream(const Time& curTime) {
    if (mState != CONNECTED && mState != DISCONNECTED) {

      if (!mConnected && mNumInitRetransmissions < MAX_INIT_RETRANSMISSIONS ) {
	if ( mLastSendTime != Time::null() && (curTime - mLastSendTime).toMicroseconds() < 2*mStreamRTOMicroseconds) {
	  return true;
	}

	sendInitPacket(mInitialData, mInitialDataLength);

	mLastSendTime = curTime;

	mNumInitRetransmissions++;
	mStreamRTOMicroseconds = (int64) (mStreamRTOMicroseconds * 2);
	return true;
      }

      mInitialDataLength = 0;

      if (!mConnected) {
        mStreamReturnCallbackMap.erase(mConnection.lock()->localEndPoint());

	// If this is the root stream that failed to connect, close the
	// connection associated with it as well.
	if (mParentLSID == 0) {
	   mConnection.lock()->close(true);
	}

	//send back an error to the app by calling mStreamReturnCallback
	//with an error code.
	mStreamReturnCallback(FAILURE, boost::shared_ptr<Stream<UUID> >() );

	return false;
      }
      else {
	mState = CONNECTED;
      }
    }
    else {
      if (mState != DISCONNECTED) {
	//this should wait for the queue to get occupied... right now it is
	//just polling...

        if ( mLastSendTime != Time::null() 
             && (curTime - mLastSendTime).toMicroseconds() > 2*mStreamRTOMicroseconds) 
        {
	  resendUnackedPackets();
	  mLastSendTime = curTime;
        }

	if (mState == PENDING_DISCONNECT &&
	    mQueuedBuffers.empty()  &&
	    mChannelToBufferMap.empty() )
	{
	    mState = DISCONNECTED;

	    return true;
	}

	boost::unique_lock<boost::mutex> lock(mQueueMutex);

	while ( !mQueuedBuffers.empty() ) {
	  boost::shared_ptr<StreamBuffer> buffer = mQueuedBuffers.front();

	  if (mTransmitWindowSize < buffer->mBufferLength) {
	    break;
	  }

	  uint64 channelID = sendDataPacket(buffer->mBuffer,
					    buffer->mBufferLength,
					    buffer->mOffset
					    );

	  buffer->mTransmitTime = curTime;

	  if ( mChannelToBufferMap.find(channelID) == mChannelToBufferMap.end() ) {
	    mChannelToBufferMap[channelID] = buffer;
	  }

	  mQueuedBuffers.pop_front();
	  mCurrentQueueLength -= buffer->mBufferLength;
	  mLastSendTime = curTime;

	  assert(buffer->mBufferLength <= mTransmitWindowSize);
	  mTransmitWindowSize -= buffer->mBufferLength;
	  mNumOutstandingBytes += buffer->mBufferLength;
	}
      }
    }

    return true;
  }

  inline void resendUnackedPackets() {
    boost::lock_guard<boost::mutex> lock(mQueueMutex);

    for(std::map<uint64,boost::shared_ptr<StreamBuffer> >::iterator it=mChannelToBufferMap.begin();
	 it != mChannelToBufferMap.end(); ++it)
     {
        mQueuedBuffers.push_front(it->second);
	mCurrentQueueLength += it->second->mBufferLength;

        //printf("On %d, resending unacked packet at offset %d:%d\n", (int)mLSID, (int)it->first, (int)(it->second->mOffset));

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

     if (!mChannelToBufferMap.empty()) {
       mStreamRTOMicroseconds *= 2;
       mChannelToBufferMap.clear();
     }
  }

  /* This function sends received data up to the application interface.
     mReceiveBufferMutex must be locked before calling this function. */
  void sendToApp(uint64 skipLength) {
    uint32 readyBufferSize = skipLength;

    for (uint i=skipLength; i < MAX_RECEIVE_WINDOW; i++) {
      if (mReceiveBitmap[i] == 1) {
	readyBufferSize++;
      }
      else if (mReceiveBitmap[i] == 0) {
	break;
      }
    }

    //pass data up to the app from 0 to readyBufferSize;
    //
    if (mReadCallback != NULL && readyBufferSize > 0) {
      mReadCallback(mReceiveBuffer, readyBufferSize);

      //now move the window forward...
      mLastContiguousByteReceived = mLastContiguousByteReceived + readyBufferSize;
      mNextByteExpected = mLastContiguousByteReceived + 1;

      memset(mReceiveBitmap, 0, readyBufferSize);
      memmove(mReceiveBitmap, mReceiveBitmap + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);
      memmove(mReceiveBuffer, mReceiveBuffer + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);

      mReceiveWindowSize += readyBufferSize;
    }
  }

  void receiveData( CBR::Protocol::SST::SSTStreamHeader* streamMsg,
		    const void* buffer, uint64 offset, uint32 len )
  {
    if (streamMsg->type() == streamMsg->REPLY) {
      mConnected = true;
    }
    else if (streamMsg->type() == streamMsg->ACK) {
      boost::lock_guard<boost::mutex> lock(mQueueMutex);

      if (mChannelToBufferMap.find(offset) != mChannelToBufferMap.end()) {
	uint64 dataOffset = mChannelToBufferMap[offset]->mOffset;
	mNumOutstandingBytes -= mChannelToBufferMap[offset]->mBufferLength;

	mChannelToBufferMap[offset]->mAckTime = Timer::now();

	updateRTO(mChannelToBufferMap[offset]->mTransmitTime, mChannelToBufferMap[offset]->mAckTime);

	if ( (int) (pow(2, streamMsg->window()) - mNumOutstandingBytes) > 0 ) {
	  assert( pow(2, streamMsg->window()) - mNumOutstandingBytes > 0);
	  mTransmitWindowSize = pow(2, streamMsg->window()) - mNumOutstandingBytes;
	}
	else {
	  mTransmitWindowSize = 0;
	}

	//printf("REMOVED ack packet at offset %d\n", (int)mChannelToBufferMap[offset]->mOffset);

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
      boost::mutex::scoped_lock lock(mReceiveBufferMutex);

      assert ( pow(2, streamMsg->window()) - mNumOutstandingBytes > 0);
      mTransmitWindowSize = pow(2, streamMsg->window()) - mNumOutstandingBytes;

      //printf("offset=%d,  mLastContiguousByteReceived=%d, mNextByteExpected=%d\n", (int)offset,  (int)mLastContiguousByteReceived, (int)mNextByteExpected);

      if ( (int)(offset) == mNextByteExpected) {
        uint32 offsetInBuffer = offset - mLastContiguousByteReceived - 1;
        if (offsetInBuffer + len <= MAX_RECEIVE_WINDOW) {
	  mReceiveWindowSize -= len;

	  memcpy(mReceiveBuffer+offsetInBuffer, buffer, len);
	  memset(mReceiveBitmap+offsetInBuffer, 1, len);

	  sendToApp(len);

	  //send back an ack.
          sendAckPacket();
	}
        else {
           //dont ack this packet.. its falling outside the receive window.
	  sendToApp(0);
        }
      }
      else {

        int32 offsetInBuffer = offset - mLastContiguousByteReceived - 1;

	//std::cout << offsetInBuffer << "  ,  " << offsetInBuffer+len << "\n";

	if ( (int)(offset+len-1) <= (int)mLastContiguousByteReceived) {
	  //printf("Acking packet which we had already received previously\n");
	  sendAckPacket();
	}
        else if (offsetInBuffer + len <= MAX_RECEIVE_WINDOW) {
	  assert (offsetInBuffer + len > 0);

          mReceiveWindowSize -= len;

   	  memcpy(mReceiveBuffer+offsetInBuffer, buffer, len);
	  memset(mReceiveBitmap+offsetInBuffer, 1, len);

          sendAckPacket();
	}
	else {
	  //dont ack this packet.. its falling outside the receive window.
	  sendToApp(0);
	}
      }
    }
  }

  LSID getLSID() {
    return mLSID;
  }

  void updateRTO(Time sampleStartTime, Time sampleEndTime) {
    static bool firstRTO = true;

    if (sampleStartTime > sampleEndTime ) {
      std::cout << "Bad sample\n";
      return;
    }

    if (firstRTO) {
      mStreamRTOMicroseconds = (sampleEndTime - sampleStartTime).toMicroseconds() ;
      firstRTO = false;
    }
    else {

      mStreamRTOMicroseconds = FL_ALPHA * mStreamRTOMicroseconds +
	(1.0-FL_ALPHA) * (sampleEndTime - sampleStartTime).toMicroseconds();
    }
  }

  void sendInitPacket(void* data, uint32 len) {
    //std::cout <<  mConnection.lock()->localEndPoint().endPoint.toString()  << " sending Init packet\n";

    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.INIT);
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    sstMsg.set_psid(mParentLSID);

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
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    //printf("Sending Ack packet with window %d\n", (int)sstMsg.window());

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection.lock()->sendData(  buffer.data(), buffer.size());
  }

  uint64 sendDataPacket(const void* data, uint32 len, uint32 offset) {
    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.DATA);
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    sstMsg.set_bsn(offset);

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    return mConnection.lock()->sendData(  buffer.data(), buffer.size());
  }

  void sendReplyPacket(void* data, uint32 len, LSID remoteLSID) {
    //printf("Sending Reply packet\n");

    CBR::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.REPLY);
    sstMsg.set_flags(0);
    sstMsg.set_window( log(mReceiveWindowSize)/log(2)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    sstMsg.set_rsid(remoteLSID);
    sstMsg.set_bsn(0);

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);
    mConnection.lock()->sendData(  buffer.data(), buffer.size());
  }

  uint8 mState;

  uint16 mLocalPort;
  uint16 mRemotePort;

  uint32 mNumBytesSent;

  LSID mParentLSID;
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

  int64 mStreamRTOMicroseconds;
  float FL_ALPHA;


  uint32 mTransmitWindowSize;
  uint32 mReceiveWindowSize;
  uint32 mNumOutstandingBytes;

  int64 mNextByteExpected;
  int64 mLastContiguousByteReceived;
  Time mLastSendTime;

  uint8* mReceiveBuffer;
  uint8* mReceiveBitmap;
  boost::mutex mReceiveBufferMutex;

  ReadCallback mReadCallback;
  StreamReturnCallbackFunction mStreamReturnCallback;


  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction> StreamReturnCallbackMap;
  static StreamReturnCallbackMap mStreamReturnCallbackMap;
  static Mutex mStreamCreationMutex;

  friend class Connection<EndPointType>;

  /* Variables required for the initial connection */
  bool mConnected;
  uint8* mInitialData;
  uint16 mInitialDataLength;
  uint8 mNumInitRetransmissions;
  uint8 MAX_INIT_RETRANSMISSIONS;
};

class SSTConnectionManager : public PollingService {
public:
    SSTConnectionManager(Context* ctx)
     : PollingService(ctx->mainStrand),
       mProfiler(ctx->profiler->addStage("SSTConnectionManager Service"))
    {
    }

    void poll() {
        mProfiler->started();
        Connection<UUID>::service();
        mProfiler->finished();
    }

    ~SSTConnectionManager() {
        Connection<UUID>::closeConnections();
        delete mProfiler;
    }
private:
    TimeProfiler::Stage* mProfiler;
};


}

#endif
