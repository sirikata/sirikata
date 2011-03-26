/*  Sirikata
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


#ifndef SST_IMPL_HPP
#define SST_IMPL_HPP

#include <sirikata/core/util/Platform.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <sirikata/core/odp/Service.hpp>
#include <sirikata/core/odp/Port.hpp>
#include <sirikata/core/util/Timer.hpp>
#include <sirikata/core/service/PollingService.hpp>
#include <sirikata/core/service/Context.hpp>

#include <bitset>

#include <sirikata/core/util/SerializationCheck.hpp>

#include <sirikata/core/network/Message.hpp>

#include "Protocol_SSTHeader.pbj.hpp"

namespace Sirikata {

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

    std::string toString() const {
        return endPoint.toString() + boost::lexical_cast<std::string>(port);
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

class  SSTConnectionManager;

template <typename EndPointType>
class SIRIKATA_EXPORT BaseDatagramLayer
{
  public:
    typedef std::tr1::shared_ptr<BaseDatagramLayer<EndPointType> > Ptr;
    typedef Ptr BaseDatagramLayerPtr;

    static BaseDatagramLayerPtr getDatagramLayer(EndPointType endPoint) {
        if (sDatagramLayerMap.find(endPoint) != sDatagramLayerMap.end()) {
            return sDatagramLayerMap[endPoint];
        }

        return BaseDatagramLayerPtr();
    }

    static BaseDatagramLayerPtr createDatagramLayer(
        EndPointType endPoint,
        const Context* ctx,
        ODP::Service* odp)
    {
        if (sDatagramLayerMap.find(endPoint) != sDatagramLayerMap.end()) {
            return sDatagramLayerMap[endPoint];
        }

        BaseDatagramLayerPtr datagramLayer(
            new BaseDatagramLayer(ctx, odp)
        );

        sDatagramLayerMap[endPoint] = datagramLayer;

        return datagramLayer;
    }

    static void listen(EndPoint<EndPointType>& listeningEndPoint) {
        EndPointType endPointID = listeningEndPoint.endPoint;

        BaseDatagramLayerPtr bdl = sDatagramLayerMap[endPointID];
        bdl->listenOn(listeningEndPoint);
    }

    void listenOn(EndPoint<EndPointType>& listeningEndPoint, ODP::MessageHandler cb) {
        ODP::Port* port = allocatePort(listeningEndPoint);
        port->receive(cb);
    }

    void unlisten(EndPoint<EndPointType>& ep) {
        // To stop listening, just destroy the corresponding port
        typename PortMap::iterator it = mAllocatedPorts.find(ep);
        if (it == mAllocatedPorts.end()) return;
        delete it->second;
        mAllocatedPorts.erase(it);
    }

    void send(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
        boost::mutex::scoped_lock lock(mMutex);

        ODP::Port* port = getOrAllocatePort(*src);

        port->send(
            ODP::Endpoint(dest->endPoint, dest->port),
            MemoryReference(data, len)
        );
    }

    const Context* context() {
        return mContext;
    }

    uint32 getUnusedPort(const EndPointType& ep) {
        return mODP->unusedODPPort(ep);
    }

  private:
    BaseDatagramLayer(const Context* ctx, ODP::Service* odpservice)
        : mContext(ctx),
        mODP(odpservice)
        {
        }

    ODP::Port* allocatePort(const EndPoint<EndPointType>& ep) {
        ODP::Port* port = mODP->bindODPPort(
            ep.endPoint, ep.port
        );
        mAllocatedPorts[ep] = port;
        return port;
    }

    ODP::Port* getPort(const EndPoint<EndPointType>& ep) {
        typename PortMap::iterator it = mAllocatedPorts.find(ep);
        if (it == mAllocatedPorts.end()) return NULL;
        return it->second;
    }

    ODP::Port* getOrAllocatePort(const EndPoint<EndPointType>& ep) {
        ODP::Port* result = getPort(ep);
        if (result != NULL) return result;
        result = allocatePort(ep);
        return result;
    }

    void listenOn(const EndPoint<EndPointType>& listeningEndPoint) {
        ODP::Port* port = allocatePort(listeningEndPoint);
        port->receive(
            std::tr1::bind(
                &BaseDatagramLayer::receiveMessage, this,
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3
            )
        );
    }

    void receiveMessage(const ODP::Endpoint &src, const ODP::Endpoint &dst, MemoryReference payload) {
        Connection<EndPointType>::handleReceive(
            EndPoint<EndPointType> (SpaceObjectReference(src.space(), src.object()), src.port()),
            EndPoint<EndPointType> (SpaceObjectReference(dst.space(), dst.object()), dst.port()),
            (void*) payload.data(), payload.size()
        );
    }


    const Context* mContext;
    ODP::Service* mODP;

    typedef std::map<EndPoint<EndPointType>, ODP::Port*> PortMap;
    PortMap mAllocatedPorts;

    boost::mutex mMutex;

    static std::map<EndPointType, BaseDatagramLayerPtr > sDatagramLayerMap;
};

#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
//SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT BaseDatagramLayer<Sirikata::UUID>;
SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT BaseDatagramLayer<SpaceObjectReference>;
#endif

//typedef std::tr1::function< void(int, std::tr1::shared_ptr< Connection<UUID> > ) > ConnectionReturnCallbackFunction;
//typedef std::tr1::function< void(int, std::tr1::shared_ptr< Stream<UUID> >) >  StreamReturnCallbackFunction;
typedef std::tr1::function< void(int, std::tr1::shared_ptr< Connection<SpaceObjectReference> > ) > ConnectionReturnCallbackFunction;
typedef std::tr1::function< void(int, std::tr1::shared_ptr< Stream<SpaceObjectReference> >) >  StreamReturnCallbackFunction;

typedef std::tr1::function< void (int, void*) >  DatagramSendDoneCallback;

typedef std::tr1::function<void (uint8*, int) >  ReadDatagramCallback;

typedef std::tr1::function<void (uint8*, int) > ReadCallback;

typedef UUID USID;

typedef uint16 LSID;

#define SST_IMPL_SUCCESS 0
#define SST_IMPL_FAILURE -1

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
class SIRIKATA_EXPORT Connection {
  public:
    typedef std::tr1::shared_ptr<Connection> Ptr;
    typedef Ptr ConnectionPtr;

private:
    typedef BaseDatagramLayer<EndPointType> BaseDatagramLayerType;
    typedef std::tr1::shared_ptr<BaseDatagramLayerType> BaseDatagramLayerPtr;

  friend class Stream<EndPointType>;
  friend class SSTConnectionManager;
  friend class BaseDatagramLayer<EndPointType>;

  typedef std::map<EndPoint<EndPointType>, std::tr1::shared_ptr<Connection> >  ConnectionMap;
  typedef std::map<EndPoint<EndPointType>, ConnectionReturnCallbackFunction>  ConnectionReturnCallbackMap;
  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction>   StreamReturnCallbackMap;


  static ConnectionMap sConnectionMap;
  static std::bitset<65536> sAvailableChannels;

  static ConnectionReturnCallbackMap sConnectionReturnCallbackMap;
  static StreamReturnCallbackMap  sListeningConnectionsCallbackMap;

  static Mutex sStaticMembersLock;

  EndPoint<EndPointType> mLocalEndPoint;
  EndPoint<EndPointType> mRemoteEndPoint;


  BaseDatagramLayerPtr mDatagramLayer;

  int mState;
  uint16 mRemoteChannelID;
  uint16 mLocalChannelID;

  uint64 mTransmitSequenceNumber;
  uint64 mLastReceivedSequenceNumber;   //the last transmit sequence number received from the other side

  typedef std::map<LSID, std::tr1::shared_ptr< Stream<EndPointType> > > LSIDStreamMap;
  std::map<LSID, std::tr1::shared_ptr< Stream<EndPointType> > > mOutgoingSubstreamMap;
  std::map<LSID, std::tr1::shared_ptr< Stream<EndPointType> > > mIncomingSubstreamMap;

  std::map<uint16, StreamReturnCallbackFunction> mListeningStreamsCallbackMap;
  std::map<uint16, std::vector<ReadDatagramCallback> > mReadDatagramCallbacks;
  typedef std::vector<std::string> PartialPayloadList;
  typedef std::map<LSID, PartialPayloadList> PartialPayloadMap;
  PartialPayloadMap mPartialReadDatagrams;

  uint16 mNumStreams;

  std::deque< std::tr1::shared_ptr<ChannelSegment> > mQueuedSegments;
  std::deque< std::tr1::shared_ptr<ChannelSegment> > mOutstandingSegments;

  uint16 mCwnd;
  int64 mRTOMicroseconds; // RTO in microseconds
  bool mFirstRTO;

  boost::mutex mQueueMutex;

  uint16 MAX_DATAGRAM_SIZE;
  uint16 MAX_PAYLOAD_SIZE;
  uint32 MAX_QUEUED_SEGMENTS;
  float  CC_ALPHA;
  Time mLastTransmitTime;

  std::tr1::weak_ptr<Connection<EndPointType> > mWeakThis;

  uint16 mNumInitialRetransmissionAttempts;

  google::protobuf::LogSilencer logSilencer;

private:

  Connection(EndPoint<EndPointType> localEndPoint,
             EndPoint<EndPointType> remoteEndPoint)
    : mLocalEndPoint(localEndPoint), mRemoteEndPoint(remoteEndPoint),
      mState(CONNECTION_DISCONNECTED),
      mRemoteChannelID(0), mLocalChannelID(1), mTransmitSequenceNumber(1),
      mLastReceivedSequenceNumber(1),
      mNumStreams(0), mCwnd(1), mRTOMicroseconds(10000000),
      mFirstRTO(true),  MAX_DATAGRAM_SIZE(1000), MAX_PAYLOAD_SIZE(1300),
      MAX_QUEUED_SEGMENTS(3000),
      CC_ALPHA(0.8), mLastTransmitTime(Time::null()),
      mNumInitialRetransmissionAttempts(0),
      inSendingMode(true), numSegmentsSent(0)
  {
      mDatagramLayer = BaseDatagramLayer<EndPointType>::getDatagramLayer(localEndPoint.endPoint);
      mDatagramLayer->listenOn(
          localEndPoint,
          std::tr1::bind(
              &Connection::receiveODPMessage, this,
              std::tr1::placeholders::_1,
              std::tr1::placeholders::_2,
              std::tr1::placeholders::_3
          )
      );
  }

  void sendSSTChannelPacket(Sirikata::Protocol::SST::SSTChannelHeader& sstMsg) {
    if (mState == CONNECTION_DISCONNECTED) return;

    std::string buffer = serializePBJMessage(sstMsg);
    mDatagramLayer->send(&mLocalEndPoint, &mRemoteEndPoint, (void*) buffer.data(),
				       buffer.size());
  }

  const Context* getContext() {
    return mDatagramLayer->context();
  }

  /* Returns -1 if no channel is available. Otherwise returns the lowest
     available channel. */
  static int getAvailableChannel() {
    //TODO: faster implementation.
    for (uint32 i = sAvailableChannels.size() - 1; i>0; i--) {
      if (!sAvailableChannels.test(i)) {
	sAvailableChannels.set(i, 1);
	return i;
      }
    }

    return 0;
  }

  static void releaseChannel(uint16 channel) {
    assert(channel > 0);

    sAvailableChannels.set(channel, 0);
  }

  bool inSendingMode; uint16 numSegmentsSent;

  void serviceConnectionNoReturn(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
      serviceConnection(conn);
  }
  bool serviceConnection(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
    const Time curTime = Timer::now();

    if (mState == CONNECTION_PENDING_CONNECT) {
      mOutstandingSegments.clear();
    }

    // should start from ssthresh, the slow start lower threshold, but starting
    // from 1 for now. Still need to implement slow start.
    if (mState == CONNECTION_DISCONNECTED) {
      std::tr1::shared_ptr<Connection<EndPointType> > thus (mWeakThis.lock());
        if (thus) {
          cleanup(thus);
        }else {
            SILOG(sst,error,"FATAL: disconnected lost weak pointer for Connection<EndPointType> too early to call cleanup on it");
        }
        return false;
    }
    else if (mState == CONNECTION_PENDING_DISCONNECT) {
      boost::mutex::scoped_lock lock(mQueueMutex);

      if (mQueuedSegments.empty()) {
        mState = CONNECTION_DISCONNECTED;
        std::tr1::shared_ptr<Connection<EndPointType> > thus (mWeakThis.lock());
        if (thus) {
          cleanup(thus);
        }else {
            SILOG(sst,error,"FATAL: pending disconnection lost weak pointer for Connection<EndPointType> too early to call cleanup on it");
        }
        return false;
      }
    }

    if (inSendingMode) {
      boost::mutex::scoped_lock lock(mQueueMutex);

      numSegmentsSent = 0;

      for (int i = 0; (!mQueuedSegments.empty()) && mOutstandingSegments.size() <= mCwnd; i++) {
	  std::tr1::shared_ptr<ChannelSegment> segment = mQueuedSegments.front();

	  Sirikata::Protocol::SST::SSTChannelHeader sstMsg;
	  sstMsg.set_channel_id( mRemoteChannelID );
	  sstMsg.set_transmit_sequence_number(segment->mChannelSequenceNumber);
	  sstMsg.set_ack_count(1);
	  sstMsg.set_ack_sequence_number(segment->mAckSequenceNumber);

	  sstMsg.set_payload(segment->mBuffer, segment->mBufferLength);

          /*printf("%s sending packet from data sending loop to %s \n",
                   mLocalEndPoint.endPoint.toString().c_str()
                   , mRemoteEndPoint.endPoint.toString().c_str());*/


	  sendSSTChannelPacket(sstMsg);

          if (mState == CONNECTION_PENDING_CONNECT) {
            mNumInitialRetransmissionAttempts++;

          }

	  segment->mTransmitTime = curTime;
	  mOutstandingSegments.push_back(segment);

	  numSegmentsSent++;

	  mLastTransmitTime = curTime;


          if (mState != CONNECTION_PENDING_CONNECT || mNumInitialRetransmissionAttempts > 3) {

            inSendingMode = false;
            mQueuedSegments.pop_front();
          }
      }

      if (!inSendingMode || mState == CONNECTION_PENDING_CONNECT) {
        getContext()->mainStrand->post(Duration::microseconds(mRTOMicroseconds),
                                       std::tr1::bind(&Connection<EndPointType>::serviceConnectionNoReturn, this, mWeakThis.lock()) );
      }
    }
    else {
      if (mState == CONNECTION_PENDING_CONNECT) {
        std::tr1::shared_ptr<Connection<EndPointType> > thus (mWeakThis.lock());
        if (thus) {
          cleanup(thus);
        }else {
            SILOG(sst,error,"FATAL: pending connection lost weak pointer for Connection<EndPointType> too early to call cleanup on it");
        }

        return false; //the connection was unable to contact the other endpoint.
      }

      if (mOutstandingSegments.size() > 0) {
        mCwnd /= 2;

        if (mCwnd < 1) {
          mCwnd = 1;
        }

        mOutstandingSegments.clear();
      }

      inSendingMode = true;

      getContext()->mainStrand->post(Duration::microseconds(1),
                                     std::tr1::bind(&Connection<EndPointType>::serviceConnectionNoReturn, this, mWeakThis.lock()) );
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


  /* Create a connection for the application to a remote
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
               void (std::tr1::shared_ptr<Connection>). If the std::tr1::shared_ptr argument
               is NULL, the connection setup failed.

     @return false if it's not possible to create this connection, e.g. if another connection
     is already using the same local endpoint; true otherwise.
  */

  static bool createConnection(EndPoint <EndPointType> localEndPoint,
			       EndPoint <EndPointType> remoteEndPoint,
                               ConnectionReturnCallbackFunction cb,
			       StreamReturnCallbackFunction scb)

  {
    boost::mutex::scoped_lock lock(sStaticMembersLock.getMutex());

    if (sConnectionMap.find(localEndPoint) != sConnectionMap.end()) {
      std::cout << "sConnectionMap.find failed for " << localEndPoint.endPoint.toString() << "\n";

      return false;
    }

    uint16 availableChannel = getAvailableChannel();

    if (availableChannel == 0)
      return false;

    std::tr1::shared_ptr<Connection>  conn =  std::tr1::shared_ptr<Connection> (
                    new Connection(localEndPoint, remoteEndPoint));

    sConnectionMap[localEndPoint] = conn;
    sConnectionReturnCallbackMap[localEndPoint] = cb;

    lock.unlock();

    conn->mWeakThis = conn;
    conn->setState(CONNECTION_PENDING_CONNECT);

    uint16 payload[1];
    payload[0] = htons(availableChannel);

    conn->setLocalChannelID(availableChannel);
    conn->sendData(payload, sizeof(payload), false);

    return true;
  }

  static bool listen(StreamReturnCallbackFunction cb, EndPoint<EndPointType> listeningEndPoint) {
    BaseDatagramLayer<EndPointType>::listen(listeningEndPoint);

    boost::mutex::scoped_lock lock(sStaticMembersLock.getMutex());

    if (sListeningConnectionsCallbackMap.find(listeningEndPoint) != sListeningConnectionsCallbackMap.end()){
      return false;
    }

    sListeningConnectionsCallbackMap[listeningEndPoint] = cb;

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
                                 should have the signature void (int,std::tr1::shared_ptr<Stream>).

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

    std::tr1::shared_ptr<Stream<EndPointType> > stream =
      std::tr1::shared_ptr<Stream<EndPointType> >
      ( new Stream<EndPointType>(parentLSID, mWeakThis, local_port, remote_port,  usid, lsid, cb) );
    stream->mWeakThis = stream;
    stream->init(initial_data, length, false, 0);

    mOutgoingSubstreamMap[lsid]=stream;
  }

  uint64 sendData(const void* data, uint32 length, bool isAck) {
    boost::mutex::scoped_lock lock(mQueueMutex);

    assert(length <= MAX_PAYLOAD_SIZE);

    Sirikata::Protocol::SST::SSTStreamHeader* stream_msg =
                       new Sirikata::Protocol::SST::SSTStreamHeader();

    std::string str = std::string( (char*)data, length);

    bool parsed = parsePBJMessage(stream_msg, str);

    uint64 transmitSequenceNumber =  mTransmitSequenceNumber;

    bool pushedIntoQueue = false;

    if ( isAck ) {
      Sirikata::Protocol::SST::SSTChannelHeader sstMsg;
      sstMsg.set_channel_id( mRemoteChannelID );
      sstMsg.set_transmit_sequence_number(mTransmitSequenceNumber);
      sstMsg.set_ack_count(1);
      sstMsg.set_ack_sequence_number(mLastReceivedSequenceNumber);

      sstMsg.set_payload(data, length);

      sendSSTChannelPacket(sstMsg);
    }
    else {
      if (mQueuedSegments.size() < MAX_QUEUED_SEGMENTS) {
        mQueuedSegments.push_back( std::tr1::shared_ptr<ChannelSegment>(
                                   new ChannelSegment(data, length, mTransmitSequenceNumber, mLastReceivedSequenceNumber) ) );
        pushedIntoQueue = true;

        if (inSendingMode) {
          getContext()->mainStrand->post(Duration::milliseconds(1.0),
                                         std::tr1::bind(&Connection::serviceConnectionNoReturn, this, mWeakThis.lock()) );
        }
      }
    }

    mTransmitSequenceNumber++;

    delete stream_msg;

    return transmitSequenceNumber;
  }

  void setState(int state) {
    mState = state;
  }

  uint8 getState() {
    return mState;
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
    for (std::deque< std::tr1::shared_ptr<ChannelSegment> >::iterator it = mOutstandingSegments.begin();
         it != mOutstandingSegments.end(); it++)
    {
      std::tr1::shared_ptr<ChannelSegment> segment = *it;

      if (segment->mChannelSequenceNumber == receivedAckNum) {
	segment->mAckTime = Timer::now();

        if (mFirstRTO ) {
	    mRTOMicroseconds = ((segment->mAckTime - segment->mTransmitTime).toMicroseconds()) ;
	    mFirstRTO = false;
        }
        else {
          mRTOMicroseconds = CC_ALPHA * mRTOMicroseconds +
            (1.0-CC_ALPHA) * (segment->mAckTime - segment->mTransmitTime).toMicroseconds();
        }

        inSendingMode = true;

        getContext()->mainStrand->post(
                                     std::tr1::bind(&Connection<EndPointType>::serviceConnectionNoReturn, this, mWeakThis.lock()) );

        if (rand() % mCwnd == 0)  {
          mCwnd += 1;
        }

        mOutstandingSegments.erase(it);
        break;
      }
    }
  }

  void receiveODPMessage(const ODP::Endpoint &src, const ODP::Endpoint &dst, MemoryReference payload) {
    receiveMessage((void*) payload.data(), payload.size() );
  }

  void parsePacket(Sirikata::Protocol::SST::SSTChannelHeader* received_channel_msg )
  {
    Sirikata::Protocol::SST::SSTStreamHeader* received_stream_msg =
                       new Sirikata::Protocol::SST::SSTStreamHeader();
    bool parsed = parsePBJMessage(received_stream_msg, received_channel_msg->payload());

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

    delete received_stream_msg ;
  }

  void handleInitPacket(Sirikata::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
      if (mListeningStreamsCallbackMap.find(received_stream_msg->dest_port()) !=
	  mListeningStreamsCallbackMap.end())
      {
	//create a new stream
	USID usid = createNewUSID();
	LSID newLSID = ++mNumStreams;

	std::tr1::shared_ptr<Stream<EndPointType> > stream =
	  std::tr1::shared_ptr<Stream<EndPointType> >
	  (new Stream<EndPointType> (received_stream_msg->psid(), mWeakThis,
				     received_stream_msg->dest_port(),
				     received_stream_msg->src_port(),
				     usid, newLSID,
				     NULL));
        stream->mWeakThis = stream;
        stream->init(NULL, 0, true, incomingLsid);

	mOutgoingSubstreamMap[newLSID] = stream;
	mIncomingSubstreamMap[incomingLsid] = stream;

	mListeningStreamsCallbackMap[received_stream_msg->dest_port()](0, stream);

	stream->receiveData(received_stream_msg, received_stream_msg->payload().data(),
			    received_stream_msg->bsn(),
			    received_stream_msg->payload().size() );
      }
      else {
	std::cout << mLocalEndPoint.endPoint.toString()  << " not listening to streams at: " << received_stream_msg->dest_port() << "\n";
      }
    }
    else {
      mIncomingSubstreamMap[incomingLsid]->sendReplyPacket(NULL, 0, incomingLsid);
    }
  }

  void handleReplyPacket(Sirikata::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) == mIncomingSubstreamMap.end()) {
      LSID initiatingLSID = received_stream_msg->rsid();

      if (mOutgoingSubstreamMap.find(initiatingLSID) != mOutgoingSubstreamMap.end()) {
	std::tr1::shared_ptr< Stream<EndPointType> > stream = mOutgoingSubstreamMap[initiatingLSID];
	mIncomingSubstreamMap[incomingLsid] = stream;
        stream->initRemoteLSID(incomingLsid);

	if (stream->mStreamReturnCallback != NULL){
	  stream->mStreamReturnCallback(SST_IMPL_SUCCESS, stream);
          stream->mStreamReturnCallback = NULL;
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

  void handleDataPacket(Sirikata::Protocol::SST::SSTStreamHeader* received_stream_msg) {
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) != mIncomingSubstreamMap.end()) {
      std::tr1::shared_ptr< Stream<EndPointType> > stream_ptr =
	mIncomingSubstreamMap[incomingLsid];
      stream_ptr->receiveData( received_stream_msg,
			       received_stream_msg->payload().data(),
			       received_stream_msg->bsn(),
			       received_stream_msg->payload().size()
			       );
    }
  }

  void handleAckPacket(Sirikata::Protocol::SST::SSTChannelHeader* received_channel_msg,
		       Sirikata::Protocol::SST::SSTStreamHeader* received_stream_msg)
  {
    //printf("ACK received : offset = %d\n", (int)received_channel_msg->ack_sequence_number() );
    LSID incomingLsid = received_stream_msg->lsid();

    if (mIncomingSubstreamMap.find(incomingLsid) != mIncomingSubstreamMap.end()) {
      std::tr1::shared_ptr< Stream<EndPointType> > stream_ptr =
	mIncomingSubstreamMap[incomingLsid];
      stream_ptr->receiveData( received_stream_msg,
			       received_stream_msg->payload().data(),
			       received_channel_msg->ack_sequence_number(),
			       received_stream_msg->payload().size()
			       );
    }
  }

  void handleDatagram(Sirikata::Protocol::SST::SSTStreamHeader* received_stream_msg) {
      uint8 msg_flags = received_stream_msg->flags();

      if (msg_flags & Sirikata::Protocol::SST::SSTStreamHeader::CONTINUES) {
          // More data is coming, just store the current data
          mPartialReadDatagrams[received_stream_msg->lsid()].push_back( received_stream_msg->payload() );
      }
      else {
          // Extract dispatch information
          uint16 dest_port = received_stream_msg->dest_port();
          std::vector<ReadDatagramCallback> datagramCallbacks;
          if (mReadDatagramCallbacks.find(dest_port) != mReadDatagramCallbacks.end()) {
              datagramCallbacks = mReadDatagramCallbacks[dest_port];
          }

          // The datagram is all here, just deliver
          PartialPayloadMap::iterator it = mPartialReadDatagrams.find(received_stream_msg->lsid());
          if (it != mPartialReadDatagrams.end()) {
              // Had previous partial packets
              // FIXME this should be more efficient
              std::string full_payload;
              for(PartialPayloadList::iterator pp_it = it->second.begin(); pp_it != it->second.end(); pp_it++)
                  full_payload = full_payload + (*pp_it);
              full_payload = full_payload + received_stream_msg->payload();
              mPartialReadDatagrams.erase(it);
              uint8* payload = (uint8*) full_payload.data();
              uint32 payload_size = full_payload.size();
              for (uint32 i=0 ; i < datagramCallbacks.size(); i++) {
                  datagramCallbacks[i](payload, payload_size);;
              }
          }
          else {
              // Only this part, no need to aggregate into single buffer
              uint8* payload = (uint8*) received_stream_msg->payload().data();
              uint32 payload_size = received_stream_msg->payload().size();
              for (uint32 i=0 ; i < datagramCallbacks.size(); i++) {
                  datagramCallbacks[i](payload, payload_size);
              }
          }
      }


    // And ack
    boost::mutex::scoped_lock lock(mQueueMutex);

    Sirikata::Protocol::SST::SSTChannelHeader sstMsg;
    sstMsg.set_channel_id( mRemoteChannelID );
    sstMsg.set_transmit_sequence_number(mTransmitSequenceNumber);
    sstMsg.set_ack_count(1);
    sstMsg.set_ack_sequence_number(mLastReceivedSequenceNumber);

    sendSSTChannelPacket(sstMsg);

    mTransmitSequenceNumber++;
  }

  void receiveMessage(void* recv_buff, int len) {
    uint8* data = (uint8*) recv_buff;
    std::string str = std::string((char*) data, len);

    Sirikata::Protocol::SST::SSTChannelHeader* received_msg =
                       new Sirikata::Protocol::SST::SSTChannelHeader();
    bool parsed = parsePBJMessage(received_msg, str);

    mLastReceivedSequenceNumber = received_msg->transmit_sequence_number();

    uint64 receivedAckNum = received_msg->ack_sequence_number();

    markAcknowledgedPacket(receivedAckNum);

    if (mState == CONNECTION_PENDING_CONNECT) {
      mState = CONNECTION_CONNECTED;

      EndPoint<EndPointType> originalListeningEndPoint(mRemoteEndPoint.endPoint, mRemoteEndPoint.port);

      uint16* received_payload = (uint16*) received_msg->payload().data();

      setRemoteChannelID( ntohs(received_payload[0]));
      mRemoteEndPoint.port = ntohs(received_payload[1]);

      sendData( received_payload, 0, false );

      boost::mutex::scoped_lock lock(sStaticMembersLock.getMutex());

      if (sConnectionReturnCallbackMap.find(mLocalEndPoint) != sConnectionReturnCallbackMap.end())
      {
        if (sConnectionMap.find(mLocalEndPoint) != sConnectionMap.end()) {
          std::tr1::shared_ptr<Connection> conn = sConnectionMap[mLocalEndPoint];

          sConnectionReturnCallbackMap[mLocalEndPoint] (SST_IMPL_SUCCESS, conn);
        }
        sConnectionReturnCallbackMap.erase(mLocalEndPoint);
      }
    }
    else if (mState == CONNECTION_PENDING_RECEIVE_CONNECT) {
      mState = CONNECTION_CONNECTED;
    }
    else if (mState == CONNECTION_CONNECTED) {
      if (received_msg->payload().size() > 0) {
        parsePacket(received_msg);
      }
    }

    delete received_msg;
  }

  uint64 getRTOMicroseconds() {
    return mRTOMicroseconds;
  }

  void eraseDisconnectedStream(Stream<EndPointType>* s) {
    mOutgoingSubstreamMap.erase(s->getLSID());
    mIncomingSubstreamMap.erase(s->getRemoteLSID());
  }


  // This is the version of cleanup is used from all the normal methods in Connection
  static void cleanup(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
    conn->mDatagramLayer->unlisten(conn->mLocalEndPoint);

    int connState = conn->mState;

    if (connState == CONNECTION_PENDING_CONNECT || connState == CONNECTION_DISCONNECTED) {
      //Deal with the connection not getting connected with the remote endpoint.
      //This is in contrast to the case where the connection got connected, but
      //the connection's root stream was unable to do so.

      boost::mutex::scoped_lock lock(sStaticMembersLock.getMutex());
      ConnectionReturnCallbackFunction cb = NULL;
      if (sConnectionReturnCallbackMap.find(conn->localEndPoint()) != sConnectionReturnCallbackMap.end()) {
        cb = sConnectionReturnCallbackMap[conn->localEndPoint()];
      }


      std::tr1::shared_ptr<Connection>  failed_conn = conn;

      sConnectionReturnCallbackMap.erase(conn->localEndPoint());
      sConnectionMap.erase(conn->localEndPoint());

      lock.unlock();


      if (connState == CONNECTION_PENDING_CONNECT && cb ) {
        cb(SST_IMPL_FAILURE, failed_conn);
      }

      conn->mState = CONNECTION_DISCONNECTED;
    }
  }
  // This version should only be called by the destructor!
  void finalCleanup() {
    if (mState != CONNECTION_DISCONNECTED) {
        mDatagramLayer->unlisten(mLocalEndPoint);

        close(true);
        mState = CONNECTION_DISCONNECTED;
    }

    releaseChannel(mLocalChannelID);
  }

  static void closeConnections() {
      // We have to be careful with this function. Because it is going to free
      // the connections, we have to make sure not to let them get freed where
      // the deleter will modify sConnectionMap while we're still modifying it.
      //
      // Our approach is to just pick out the first connection, make a copy of
      // its shared_ptr to make sure it doesn't get freed until we want it to,
      // remove it from sConnectionMap, and then get rid of the shared_ptr to
      // allow the connection to be freed.
      //
      // Note that we don't lock sStaticMembers lock. At this point, that
      // shouldn't be a problem since we should be the only thread still
      // modifying this data. If we did lock it, we'd deadlock since the
      // destructor will also, indirectly, lock it.
      while(!sConnectionMap.empty()) {
          ConnectionPtr saved = sConnectionMap.begin()->second;
          sConnectionMap.erase(sConnectionMap.begin());
          saved.reset();
      }
  }

  static void handleReceive(EndPoint<EndPointType> remoteEndPoint,
                            EndPoint<EndPointType> localEndPoint, void* recv_buffer, int len)
  {
    char* data = (char*) recv_buffer;
    std::string str = std::string(data, len);

    Sirikata::Protocol::SST::SSTChannelHeader* received_msg = new Sirikata::Protocol::SST::SSTChannelHeader();
    bool parsed = parsePBJMessage(received_msg, str);

    uint8 channelID = received_msg->channel_id();

    boost::mutex::scoped_lock lock(sStaticMembersLock.getMutex());

    if (sConnectionMap.find(localEndPoint) != sConnectionMap.end()) {
      if (channelID == 0) {
	/*Someone's already connected at this port. Either don't reply or
	  send back a request rejected message. */

	std::cout << "Someone's already connected at this port on object " << localEndPoint.endPoint.toString() << "\n";
	return;
      }
      std::tr1::shared_ptr<Connection<EndPointType> > conn = sConnectionMap[localEndPoint];

      conn->receiveMessage(data, len);
    }
    else if (channelID == 0) {
      /* it's a new channel request negotiation protocol
	 packet ; allocate a new channel.*/

      if (sListeningConnectionsCallbackMap.find(localEndPoint) != sListeningConnectionsCallbackMap.end()) {
        uint16* received_payload = (uint16*) received_msg->payload().data();

        uint16 payload[2];

        uint16 availableChannel = getAvailableChannel();
        payload[0] = htons(availableChannel);
        uint16 availablePort = availableChannel; //availableChannel is picked from the same 16-bit
                                                 //address space and has to be unique. So why not use
                                                 //use it to identify the port as well...
        payload[1] = htons(availablePort);

        EndPoint<EndPointType> newLocalEndPoint(localEndPoint.endPoint, availablePort);
        std::tr1::shared_ptr<Connection>  conn =
                   std::tr1::shared_ptr<Connection>(
				    new Connection(newLocalEndPoint, remoteEndPoint));

	conn->listenStream(newLocalEndPoint.port, sListeningConnectionsCallbackMap[localEndPoint]);
        conn->mWeakThis = conn;
        sConnectionMap[newLocalEndPoint] = conn;

        conn->setLocalChannelID(availableChannel);
        conn->setRemoteChannelID(ntohs(received_payload[0]));
        conn->setState(CONNECTION_PENDING_RECEIVE_CONNECT);

        conn->sendData(payload, sizeof(payload), false);
      }
      else {
        std::cout << "No one listening on this connection\n";
      }
    }

    delete received_msg;
  }

public:

  virtual ~Connection() {
      // Make sure we've fully cleaned up
      finalCleanup();
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
                                     'errCode' contains SST_IMPL_SUCCESS or SST_IMPL_FAILURE
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
        cb(SST_IMPL_FAILURE, data);
      }
      return false;
    }

    LSID lsid = ++mNumStreams;

    while (currOffset < length) {
        // Because the header is variable size, we have to have this
        // somewhat annoying logic to ensure we come in under the
        // budget.  We start out with an extra 28 bytes as buffer.
        // Hopefully this is usually enough, and is based on the
        // current required header fields, their sizes, and overhead
        // from protocol buffers encoding.  In the worst case, we end
        // up being too large and have to iterate, working with less
        // data over time.
        int header_buffer = 28;
        while(true) {
            int buffLen;
            bool continues;
            if (length-currOffset > (MAX_PAYLOAD_SIZE-header_buffer)) {
                buffLen = MAX_PAYLOAD_SIZE-header_buffer;
                continues = true;
            }
            else {
                buffLen = length-currOffset;
                continues = false;
            }


            Sirikata::Protocol::SST::SSTStreamHeader sstMsg;
            sstMsg.set_lsid( lsid );
            sstMsg.set_type(sstMsg.DATAGRAM);

            uint8 flags = 0;
            if (continues)
                flags = flags | Sirikata::Protocol::SST::SSTStreamHeader::CONTINUES;
            sstMsg.set_flags(flags);

            sstMsg.set_window( (unsigned char)10 );
            sstMsg.set_src_port(local_port);
            sstMsg.set_dest_port(remote_port);

            sstMsg.set_payload( ((uint8*)data)+currOffset, buffLen);

            std::string buffer = serializePBJMessage(sstMsg);

            // If we're not within the payload size, we need to
            // increase our buffer space and try again
            if (buffer.size() > MAX_PAYLOAD_SIZE) {
                header_buffer += 10;
                continue;
            }

            sendData(  buffer.data(), buffer.size(), false );

            currOffset += buffLen;
            // If we got to the send, we can break out of the loop
            break;
        }
    }

    if (cb != NULL) {
      //invoke the callback function
      cb(SST_IMPL_SUCCESS, data);
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
    if (force && mState != CONNECTION_DISCONNECTED) {
      boost::mutex::scoped_lock lock(sStaticMembersLock.getMutex());
      sConnectionMap.erase(mLocalEndPoint);
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

};
#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
//SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Connection<Sirikata::UUID>;
SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Connection<SpaceObjectReference>;
#endif

class StreamBuffer{
public:

  uint8* mBuffer;
  uint32 mBufferLength;
  uint64 mOffset;

  Time mTransmitTime;
  Time mAckTime;


  StreamBuffer(const uint8* data, uint32 len, uint64 offset) :
    mTransmitTime(Time::null()), mAckTime(Time::null())
  {
    assert(len > 0);

    mBuffer = new uint8[len];
    memcpy(mBuffer,data,len);
    mBufferLength = len;
    mOffset = offset;
  }

  ~StreamBuffer() {
      delete []mBuffer;
  }
};

template <class EndPointType>
class SIRIKATA_EXPORT Stream  {
public:
    typedef std::tr1::shared_ptr<Stream> Ptr;
    typedef Ptr StreamPtr;
    typedef Connection<EndPointType> ConnectionType;
    typedef EndPoint<EndPointType> EndpointType;

   enum StreamStates {
       DISCONNECTED = 1,
       CONNECTED=2,
       PENDING_DISCONNECT=3,
       PENDING_CONNECT=4,
       NOT_FINISHED_CONSTRUCTING__CALL_INIT
     };



  virtual ~Stream() {
    close(true);

    delete [] mInitialData;
    delete [] mReceiveBuffer;
    delete [] mReceiveBitmap;

    mConnection.reset();
  }


  static bool connectStream(EndPoint <EndPointType> localEndPoint,
			    EndPoint <EndPointType> remoteEndPoint,
			    StreamReturnCallbackFunction cb)
  {
      if (localEndPoint.port == 0) {
          typename BaseDatagramLayer<EndPointType>::Ptr bdl = BaseDatagramLayer<EndPointType>::getDatagramLayer(localEndPoint.endPoint);
          localEndPoint.port = bdl->getUnusedPort(localEndPoint.endPoint);
      }

    if (mStreamReturnCallbackMap.find(localEndPoint) != mStreamReturnCallbackMap.end()) {
      return false;
    }

    mStreamReturnCallbackMap[localEndPoint] = cb;


    bool result = Connection<EndPointType>::createConnection(localEndPoint,
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
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);

    conn->listenStream(port, scb);
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

    boost::mutex::scoped_lock lock(mQueueMutex);
    int count = 0;

    if (len <= MAX_PAYLOAD_SIZE) {
      if (mCurrentQueueLength+len > MAX_QUEUE_LENGTH) {
	return 0;
      }
      mQueuedBuffers.push_back( std::tr1::shared_ptr<StreamBuffer>(new StreamBuffer(data, len, mNumBytesSent)) );
      mCurrentQueueLength += len;
      mNumBytesSent += len;


      std::tr1::shared_ptr<Connection<EndPointType> > conn =  mConnection.lock();
      if (conn)
        getContext()->mainStrand->post(Duration::seconds(0.01),
          std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn) );

      return len;
    }
    else {
      int currOffset = 0;
      while (currOffset < len) {
	int buffLen = (len-currOffset > MAX_PAYLOAD_SIZE) ?
	              MAX_PAYLOAD_SIZE :
	              (len-currOffset);

	if (mCurrentQueueLength + buffLen > MAX_QUEUE_LENGTH) {
	  break;
	}

	mQueuedBuffers.push_back( std::tr1::shared_ptr<StreamBuffer>(new StreamBuffer(data+currOffset, buffLen, mNumBytesSent)) );
	currOffset += buffLen;
	mCurrentQueueLength += buffLen;
	mNumBytesSent += buffLen;

	count++;
      }


      std::tr1::shared_ptr<Connection<EndPointType> > conn =  mConnection.lock();
      if (conn)
        getContext()->mainStrand->post(Duration::seconds(0.01),
          std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn) );

      return currOffset;
    }

    return -1;
  }

#if SIRIKATA_PLATFORM != SIRIKATA_WINDOWS
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
#endif

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

    boost::recursive_mutex::scoped_lock lock(mReceiveBufferMutex);
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
      std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    if (force) {
      mConnected = false;
      mState = DISCONNECTED;

      if (conn)
        conn->eraseDisconnectedStream(this);

      return true;
    }
    else {
      mState = PENDING_DISCONNECT;
      if (conn) {
          getContext()->mainStrand->post(
              std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn)
          );
      }
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
  virtual std::tr1::weak_ptr<Connection<EndPointType> > connection() {
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
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);
    conn->stream(cb, data, length, local_port, remote_port, mLSID);
  }

  /*
    Returns the local endpoint to which this connection is bound.

    @return the local endpoint.
  */
  virtual EndPoint <EndPointType> localEndPoint()  {
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);

    return EndPoint<EndPointType> (conn->localEndPoint().endPoint, mLocalPort);
  }

  /*
    Returns the remote endpoint to which this connection is bound.

    @return the remote endpoint.
  */
  virtual EndPoint <EndPointType> remoteEndPoint()  {
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);

    return EndPoint<EndPointType> (conn->remoteEndPoint().endPoint, mRemotePort);
  }

  virtual uint8 getState() {
    return mState;
  }

private:
  Stream(LSID parentLSID, std::tr1::weak_ptr<Connection<EndPointType> > conn,
	 uint16 local_port, uint16 remote_port,
	 USID usid, LSID lsid, StreamReturnCallbackFunction cb)
    :
    mState(NOT_FINISHED_CONSTRUCTING__CALL_INIT),
    mLocalPort(local_port),
    mRemotePort(remote_port),
    mParentLSID(parentLSID),
    mConnection(conn),mContext(NULL),
    mUSID(usid),
    mLSID(lsid),
    mRemoteLSID(-1),
    MAX_PAYLOAD_SIZE(1000),
    MAX_QUEUE_LENGTH(4000000),
    MAX_RECEIVE_WINDOW(10000),
    mFirstRTO(true),
    mStreamRTOMicroseconds(10000000),
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
    mInitialData = NULL;
    mInitialDataLength = 0;

    mReceiveBuffer = new uint8[mReceiveWindowSize];
    mReceiveBitmap = new uint8[mReceiveWindowSize];
    memset(mReceiveBitmap, 0, mReceiveWindowSize);

    mQueuedBuffers.clear();
    mCurrentQueueLength = 0;

    // Continues in init, when we have mWeakThis set
  }

  void init(void* initial_data, uint32 length,
      bool remotelyInitiated, LSID remoteLSID) {
    if (remotelyInitiated) {
        mRemoteLSID = remoteLSID;
        mConnected = true;
        mState = CONNECTED;
    }
    else {
        mConnected = false;
        mState = PENDING_CONNECT;
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

    if (remotelyInitiated) {
      sendReplyPacket(mInitialData, mInitialDataLength, remoteLSID);
    }
    else {
      sendInitPacket(mInitialData, mInitialDataLength);
    }

    mNumInitRetransmissions = 1;
    mNumBytesSent = mInitialDataLength;

    if (length > mInitialDataLength) {
      write( ((uint8*)initial_data) + mInitialDataLength, length - mInitialDataLength);
    }
  }

  void initRemoteLSID(LSID remoteLSID) {
      mRemoteLSID = remoteLSID;
  }

  const Context* getContext() {
    if (mContext == NULL) {
      std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
      assert(conn);

      mContext = conn->getContext();
    }

    return mContext;
  }

  static void connectionCreated( int errCode, std::tr1::shared_ptr<Connection<EndPointType> > c) {
    assert(mStreamReturnCallbackMap.find(c->localEndPoint()) != mStreamReturnCallbackMap.end());

    if (errCode != SST_IMPL_SUCCESS) {

      StreamReturnCallbackFunction cb = mStreamReturnCallbackMap[c->localEndPoint()];
      mStreamReturnCallbackMap.erase(c->localEndPoint());

      cb(SST_IMPL_FAILURE, StreamPtr() );

      return;
    }

    c->stream(mStreamReturnCallbackMap[c->localEndPoint()], NULL , 0,
	      c->localEndPoint().port, c->remoteEndPoint().port);

    mStreamReturnCallbackMap.erase(c->localEndPoint());
  }

  void serviceStreamNoReturn(std::tr1::shared_ptr<Stream<EndPointType> > strm, std::tr1::shared_ptr<Connection<EndPointType> > conn) {
      serviceStream(strm, conn);
  }

  /* Returns false only if this is the root stream of a connection and it was
     unable to connect. In that case, the connection for this stream needs to
     be closed and the 'false' return value is an indication of this for
     the underlying connection. */

  bool serviceStream(std::tr1::shared_ptr<Stream<EndPointType> > strm, std::tr1::shared_ptr<Connection<EndPointType> > conn) {
      assert(strm.get() == this);

    const Time curTime = Timer::now();

    if (mState != CONNECTED && mState != DISCONNECTED && mState != PENDING_DISCONNECT) {

      if (!mConnected && mNumInitRetransmissions < MAX_INIT_RETRANSMISSIONS ) {

        sendInitPacket(mInitialData, mInitialDataLength);

	mLastSendTime = curTime;

	mNumInitRetransmissions++;

	return true;
      }

      mInitialDataLength = 0;

      if (!mConnected) {
        std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
        assert(conn);

        mStreamReturnCallbackMap.erase(conn->localEndPoint());

	// If this is the root stream that failed to connect, close the
	// connection associated with it as well.
	if (mParentLSID == 0) {
          conn->close(true);

          Connection<EndPointType>::cleanup(conn);
	}

	//send back an error to the app by calling mStreamReturnCallback
	//with an error code.
        if (mStreamReturnCallback) {
            mStreamReturnCallback(SST_IMPL_FAILURE, StreamPtr());
            mStreamReturnCallback = NULL;
        }

        mState = DISCONNECTED;

	return false;
      }
      else {
	mState = CONNECTED;
        // Schedule another servicing immediately in case any other operations
        // should occur, e.g. sending data which was added after the initial
        // connection request.
        std::tr1::shared_ptr<Connection<EndPointType> > conn =  mConnection.lock();
        if (conn)
            getContext()->mainStrand->post(std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn) );
      }
    }
    else {
      if (mState != DISCONNECTED) {

        //if the stream has been waiting for an ACK for > 2*mStreamRTOMicroseconds,
        //resend the unacked packets.
        if ( mLastSendTime != Time::null()
             && (curTime - mLastSendTime).toMicroseconds() > 2*mStreamRTOMicroseconds)
        {
	  resendUnackedPackets();
	  mLastSendTime = curTime;
        }

	boost::mutex::scoped_lock lock(mQueueMutex);

	if (mState == PENDING_DISCONNECT &&
	    mQueuedBuffers.empty()  &&
	    mChannelToBufferMap.empty() )
	{
	    mState = DISCONNECTED;

            std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
            assert(conn);
            conn->eraseDisconnectedStream(this);

	    return true;
	}

        bool sentSomething = false;
	while ( !mQueuedBuffers.empty() ) {
	  std::tr1::shared_ptr<StreamBuffer> buffer = mQueuedBuffers.front();

	  if (mTransmitWindowSize < buffer->mBufferLength) {
	    break;
	  }

	  uint64 channelID = sendDataPacket(buffer->mBuffer,
					    buffer->mBufferLength,
					    buffer->mOffset
					    );
          buffer->mTransmitTime = curTime;
          sentSomething = true;

	  if ( mChannelToBufferMap.find(channelID) == mChannelToBufferMap.end() ) {
	    mChannelToBufferMap[channelID] = buffer;
            mChannelToStreamOffsetMap[channelID] = buffer->mOffset;
	  }

	  mQueuedBuffers.pop_front();
	  mCurrentQueueLength -= buffer->mBufferLength;
	  mLastSendTime = curTime;

	  assert(buffer->mBufferLength <= mTransmitWindowSize);
	  mTransmitWindowSize -= buffer->mBufferLength;
	  mNumOutstandingBytes += buffer->mBufferLength;
	}

        if (sentSomething) {
          std::tr1::shared_ptr<Connection<EndPointType> > conn =  mConnection.lock();
          if (conn)
            getContext()->mainStrand->post(Duration::microseconds(2*mStreamRTOMicroseconds),
              std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn) );
        }
      }
    }

    return true;
  }

  inline void resendUnackedPackets() {
    boost::mutex::scoped_lock lock(mQueueMutex);

    for(std::map<uint64,std::tr1::shared_ptr<StreamBuffer> >::const_reverse_iterator it=mChannelToBufferMap.rbegin(),
            it_end=mChannelToBufferMap.rend();
        it != it_end; it++)
     {
       mQueuedBuffers.push_front(it->second);
       mCurrentQueueLength += it->second->mBufferLength;

       /*printf("On %d, resending unacked packet at offset %d:%d\n",
         (int)mLSID, (int)it->first, (int)(it->second->mOffset));fflush(stdout);*/

       if (mTransmitWindowSize < it->second->mBufferLength){
         assert( ((int) it->second->mBufferLength) > 0);
         mTransmitWindowSize = it->second->mBufferLength;
       }
     }


    std::tr1::shared_ptr<Connection<EndPointType> > conn =  mConnection.lock();
    if (conn)
      getContext()->mainStrand->post(Duration::seconds(0.01),
        std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn) );

    if (mChannelToBufferMap.empty() && !mQueuedBuffers.empty()) {
      std::tr1::shared_ptr<StreamBuffer> buffer = mQueuedBuffers.front();

      if (mTransmitWindowSize < buffer->mBufferLength) {
        mTransmitWindowSize = buffer->mBufferLength;
      }
    }

    mNumOutstandingBytes = 0;

    if (!mChannelToBufferMap.empty()) {
      if (mStreamRTOMicroseconds < 20000000) {
        mStreamRTOMicroseconds *= 2;
      }
      mChannelToBufferMap.clear();
    }
  }

  /* This function sends received data up to the application interface.
     mReceiveBufferMutex must be locked before calling this function. */
  void sendToApp(uint32 skipLength) {
    uint32 readyBufferSize = skipLength;

    for (uint32 i=skipLength; i < MAX_RECEIVE_WINDOW; i++) {
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

      memmove(mReceiveBitmap, mReceiveBitmap + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);
      memset(mReceiveBitmap + (MAX_RECEIVE_WINDOW - readyBufferSize), 0, readyBufferSize);

      memmove(mReceiveBuffer, mReceiveBuffer + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);

      mReceiveWindowSize += readyBufferSize;
    }
  }

  void receiveData( Sirikata::Protocol::SST::SSTStreamHeader* streamMsg,
		    const void* buffer, uint64 offset, uint32 len )
  {
    if (streamMsg->type() == streamMsg->REPLY) {
      mConnected = true;
    }
    else if (streamMsg->type() == streamMsg->DATA || streamMsg->type() == streamMsg->INIT) {
      boost::recursive_mutex::scoped_lock lock(mReceiveBufferMutex);

      assert ( pow(2.0, streamMsg->window()) - mNumOutstandingBytes > 0);
      mTransmitWindowSize = pow(2.0, streamMsg->window()) - mNumOutstandingBytes;

      /*std::cout << "offset=" << offset << " , mLastContiguousByteReceived=" << mLastContiguousByteReceived
        << " , mNextByteExpected=" << mNextByteExpected <<"\n";*/

      int64 offsetInBuffer = offset - mLastContiguousByteReceived - 1;
      if ( len > 0 &&  (int64)(offset) == mNextByteExpected) {
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
      else if (len > 0) {
	if ( (int64)(offset+len-1) <= (int64)mLastContiguousByteReceived) {
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


    //handle any ACKS that might be included in the message...
    boost::mutex::scoped_lock lock(mQueueMutex);

    if (mChannelToBufferMap.find(offset) != mChannelToBufferMap.end()) {
      uint64 dataOffset = mChannelToBufferMap[offset]->mOffset;
      mNumOutstandingBytes -= mChannelToBufferMap[offset]->mBufferLength;

      mChannelToBufferMap[offset]->mAckTime = Timer::now();

      updateRTO(mChannelToBufferMap[offset]->mTransmitTime, mChannelToBufferMap[offset]->mAckTime);

      if ( (int) (pow(2.0, streamMsg->window()) - mNumOutstandingBytes) > 0 ) {
        assert( pow(2.0, streamMsg->window()) - mNumOutstandingBytes > 0);
        mTransmitWindowSize = pow(2.0, streamMsg->window()) - mNumOutstandingBytes;
      }
      else {
        mTransmitWindowSize = 0;
      }

      //printf("REMOVED ack packet at offset %d\n", (int)mChannelToBufferMap[offset]->mOffset);

      mChannelToBufferMap.erase(offset);

      std::vector <uint64> channelOffsets;
      for(std::map<uint64, std::tr1::shared_ptr<StreamBuffer> >::iterator it = mChannelToBufferMap.begin();
          it != mChannelToBufferMap.end(); ++it)
	{
	  if (it->second->mOffset == dataOffset) {
	    channelOffsets.push_back(it->first);
	  }
	}

      for (uint32 i=0; i< channelOffsets.size(); i++) {
        mChannelToBufferMap.erase(channelOffsets[i]);
      }
    }
    else {
      // ACK received but not found in mChannelToBufferMap
      if (mChannelToStreamOffsetMap.find(offset) != mChannelToStreamOffsetMap.end()) {
        uint64 dataOffset = mChannelToStreamOffsetMap[offset];
        mChannelToStreamOffsetMap.erase(offset);

        std::vector <uint64> channelOffsets;
        for(std::map<uint64, std::tr1::shared_ptr<StreamBuffer> >::iterator it = mChannelToBufferMap.begin();
            it != mChannelToBufferMap.end(); ++it)
          {
            if (it->second->mOffset == dataOffset) {
              channelOffsets.push_back(it->first);
            }
          }

        for (uint32 i=0; i< channelOffsets.size(); i++) {
          mChannelToBufferMap.erase(channelOffsets[i]);
        }
      }
    }
  }

  LSID getLSID() {
    return mLSID;
  }

  LSID getRemoteLSID() {
    return mRemoteLSID;
  }

  void updateRTO(Time sampleStartTime, Time sampleEndTime) {


    if (sampleStartTime > sampleEndTime ) {
      std::cout << "Bad sample\n";
      return;
    }

    if (mFirstRTO) {
      mStreamRTOMicroseconds = (sampleEndTime - sampleStartTime).toMicroseconds() ;
      mFirstRTO = false;
    }
    else {

      mStreamRTOMicroseconds = FL_ALPHA * mStreamRTOMicroseconds +
	(1.0-FL_ALPHA) * (sampleEndTime - sampleStartTime).toMicroseconds();
    }

  }

  void sendInitPacket(void* data, uint32 len) {
    Sirikata::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.INIT);
    sstMsg.set_flags(0);
    sstMsg.set_window( log((double)mReceiveWindowSize)/log(2.0)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    sstMsg.set_psid(mParentLSID);

    sstMsg.set_bsn(0);

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);


    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();

    if (!conn) return;

    conn->sendData( buffer.data(), buffer.size(), false );

    getContext()->mainStrand->post(Duration::microseconds(2*mStreamRTOMicroseconds),

        std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn) );

  }

  void sendAckPacket() {
    Sirikata::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.ACK);
    sstMsg.set_flags(0);
    sstMsg.set_window( log((double)mReceiveWindowSize)/log(2.0)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);
    std::string buffer = serializePBJMessage(sstMsg);

    //printf("Sending Ack packet with window %d\n", (int)sstMsg.window());

    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);
    conn->sendData(  buffer.data(), buffer.size(), true);
  }

  uint64 sendDataPacket(const void* data, uint32 len, uint64 offset) {
    Sirikata::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.DATA);
    sstMsg.set_flags(0);
    sstMsg.set_window( log((double)mReceiveWindowSize)/log(2.0)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    sstMsg.set_bsn(offset);

    sstMsg.set_payload(data, len);

    std::string buffer = serializePBJMessage(sstMsg);

    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);
    return conn->sendData(  buffer.data(), buffer.size(), false);
  }

  void sendReplyPacket(void* data, uint32 len, LSID remoteLSID) {
    Sirikata::Protocol::SST::SSTStreamHeader sstMsg;
    sstMsg.set_lsid( mLSID );
    sstMsg.set_type(sstMsg.REPLY);
    sstMsg.set_flags(0);
    sstMsg.set_window( log((double)mReceiveWindowSize)/log(2.0)  );
    sstMsg.set_src_port(mLocalPort);
    sstMsg.set_dest_port(mRemotePort);

    sstMsg.set_rsid(remoteLSID);
    sstMsg.set_bsn(0);

    sstMsg.set_payload(data, len);
    std::string buffer = serializePBJMessage(sstMsg);

    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    assert(conn);
    conn->sendData(  buffer.data(), buffer.size(), false);
  }

  uint8 mState;

  uint16 mLocalPort;
  uint16 mRemotePort;

  uint64 mNumBytesSent;

  LSID mParentLSID;

  //weak_ptr to avoid circular dependency between Connection and Stream classes
  std::tr1::weak_ptr<Connection<EndPointType> > mConnection;
  const Context* mContext;

  std::map<uint64, std::tr1::shared_ptr<StreamBuffer> >  mChannelToBufferMap;
  std::map<uint64, uint32> mChannelToStreamOffsetMap;

  std::deque< std::tr1::shared_ptr<StreamBuffer> > mQueuedBuffers;
  uint32 mCurrentQueueLength;

  USID mUSID;
  LSID mLSID;
  LSID mRemoteLSID;

  uint16 MAX_PAYLOAD_SIZE;
  uint32 MAX_QUEUE_LENGTH;
  uint32 MAX_RECEIVE_WINDOW;

  boost::mutex mQueueMutex;

  bool mFirstRTO;
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
  boost::recursive_mutex mReceiveBufferMutex;

  ReadCallback mReadCallback;
  StreamReturnCallbackFunction mStreamReturnCallback;


  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction> StreamReturnCallbackMap;
  static StreamReturnCallbackMap mStreamReturnCallbackMap;

  friend class Connection<EndPointType>;

  /* Variables required for the initial connection */
  bool mConnected;
  uint8* mInitialData;
  uint16 mInitialDataLength;
  uint8 mNumInitRetransmissions;
  uint8 MAX_INIT_RETRANSMISSIONS;


  std::tr1::weak_ptr<Stream<EndPointType> > mWeakThis;
};
#if SIRIKATA_PLATFORM == SIRIKATA_WINDOWS
//SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Stream<Sirikata::UUID>;
SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Stream<SpaceObjectReference>;
#endif

class SSTConnectionManager : public Service {
public:
    virtual void start() {
    }
    virtual void stop() {
        Connection<SpaceObjectReference>::closeConnections();
    }

    ~SSTConnectionManager() {
        Connection<SpaceObjectReference>::closeConnections();
    }
};


}

#endif
