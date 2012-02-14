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

#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/util/Timer.hpp>
#include <sirikata/core/service/Context.hpp>

#include <sirikata/core/network/Message.hpp>
#include <sirikata/core/network/ObjectMessage.hpp>
#include "Protocol_SSTHeader.pbj.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp> //htons, ntohs

#define SST_LOG(lvl,msg) SILOG(sst,lvl,msg);

namespace Sirikata {
namespace SST {

template <typename EndObjectType>
class EndPoint {
public:
  EndObjectType endPoint;
  ObjectMessagePort port;

  EndPoint() {
  }

  EndPoint(EndObjectType endPoint, ObjectMessagePort port) {
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

template <typename EndPointType>
class BaseDatagramLayer;

template <typename EndPointType>
class ConnectionManager;

template <typename EndPointType>
class CallbackTypes {
public:
    typedef std::tr1::function< void(int, std::tr1::shared_ptr< Connection<EndPointType> > ) > ConnectionReturnCallbackFunction;
    typedef std::tr1::function< void(int, std::tr1::shared_ptr< Stream<EndPointType> >) >  StreamReturnCallbackFunction;

    typedef std::tr1::function< void (int, void*) >  DatagramSendDoneCallback;
    typedef std::tr1::function<void (uint8*, int) >  ReadDatagramCallback;
    typedef std::tr1::function<void (uint8*, int) > ReadCallback;
};

typedef UUID USID;

typedef uint32 LSID;

template <class EndPointType>
class ConnectionVariables {
public:

    typedef std::tr1::shared_ptr<BaseDatagramLayer<EndPointType> > BaseDatagramLayerPtr;
    typedef CallbackTypes<EndPointType> CBTypes;
    typedef typename CBTypes::ConnectionReturnCallbackFunction ConnectionReturnCallbackFunction;
    typedef typename CBTypes::StreamReturnCallbackFunction StreamReturnCallbackFunction;

  /* Returns 0 if no channel is available. Otherwise returns the lowest
     available channel. */
    uint32 getAvailableChannel(EndPointType& endPointType) {
      BaseDatagramLayerPtr datagramLayer = getDatagramLayer(endPointType);
      assert (datagramLayer != BaseDatagramLayerPtr());

      return datagramLayer->getUnusedPort(endPointType);
    }

    void releaseChannel(EndPointType& ept, uint32 channel) {

      BaseDatagramLayerPtr datagramLayer = getDatagramLayer(ept);
      if (datagramLayer != BaseDatagramLayerPtr()) {

        EndPoint<EndPointType> ep(ept, channel);

        datagramLayer->unlisten(ep);
      }
    }

    BaseDatagramLayerPtr getDatagramLayer(EndPointType& endPoint)
    {
        if (sDatagramLayerMap.find(endPoint) != sDatagramLayerMap.end()) {
            return sDatagramLayerMap[endPoint];
        }

        return BaseDatagramLayerPtr();
    }

    void addDatagramLayer(EndPointType& endPoint, BaseDatagramLayerPtr datagramLayer)
    {
        sDatagramLayerMap[endPoint] = datagramLayer;
    }

    void removeDatagramLayer(EndPointType& endPoint, bool warn = false)
    {
        typename std::map<EndPointType, BaseDatagramLayerPtr >::iterator wherei = sDatagramLayerMap.find(endPoint);
        if (wherei != sDatagramLayerMap.end()) {
            sDatagramLayerMap.erase(wherei);
        } else if (warn) {
            SILOG(sst,error,"FATAL: Invalidating BaseDatagramLayer that's invalid");
        }
    }

private:
    std::map<EndPointType, BaseDatagramLayerPtr > sDatagramLayerMap;

public:
    typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction> StreamReturnCallbackMap;
    StreamReturnCallbackMap mStreamReturnCallbackMap;

    typedef std::map<EndPoint<EndPointType>, std::tr1::shared_ptr<Connection<EndPointType> > >  ConnectionMap;
    ConnectionMap sConnectionMap;

    typedef std::map<EndPoint<EndPointType>, ConnectionReturnCallbackFunction>  ConnectionReturnCallbackMap;
    ConnectionReturnCallbackMap sConnectionReturnCallbackMap;

    StreamReturnCallbackMap  sListeningConnectionsCallbackMap;
    Mutex sStaticMembersLock;

};

// This is just a template definition. The real implementation of BaseDatagramLayer
// lies in libcore/include/sirikata/core/odp/SST.hpp and
// libcore/include/sirikata/core/ohdp/SST.hpp.
template <typename EndPointType>
class SIRIKATA_EXPORT BaseDatagramLayer
{
    // This class connects SST to the underlying datagram protocol. This isn't
    // an implementation -- the implementation will vary significantly for each
    // underlying datagram protocol -- but it does specify the interface. We
    // keep all types private in this version so it is obvious when you are
    // trying to incorrectly use this implementation instead of a real one.
  private:
    typedef std::tr1::shared_ptr<BaseDatagramLayer<EndPointType> > Ptr;
    typedef Ptr BaseDatagramLayerPtr;

    typedef std::tr1::function<void(void*, int)> DataCallback;

    /** Create a datagram layer. Required parameters are the
     *  ConnectionVariables, Endpoint, and Context. Additional variables are
     *  permitted (this is called via a templated function in
     *  ConnectionManager).
     *
     *  Should insert into ConnectionVariable's datagram layer map; should also
     *  reuse existing datagram layers.
     */
    static BaseDatagramLayerPtr createDatagramLayer(
        ConnectionVariables<EndPointType>* sstConnVars,
        EndPointType endPoint,
        const Context* ctx,
        void* extra)
    {
        return BaseDatagramLayerPtr();
    }

    /** Get the datagram layer for the given endpoint, if it exists. */
    static BaseDatagramLayerPtr getDatagramLayer(ConnectionVariables<EndPointType>* sstConnVars,
                                                 EndPointType endPoint)
    {
        return BaseDatagramLayerPtr();
    }

    /** Get the Context for this datagram layer. */
    const Context* context() {
        return NULL;
    }

    /** Get a port that isn't currently in use. */
    uint32 getUnusedPort(const EndPointType& ep) {
      return 0;
    }


    /** Stop listening to the specified endpoint and also remove from the
     *  ConnectionVariables datagram layer map.
     */
    static void stopListening(ConnectionVariables<EndPointType>* sstConnVars, EndPoint<EndPointType>& listeningEndPoint) {
    }

    /** Listen to the specified endpoint and invoke the given callback when data
     *  arrives.
     */
    void listenOn(EndPoint<EndPointType>& listeningEndPoint, DataCallback cb) {
    }

    /** Listen to the specified endpoint, invoking
     *  Connection::handleReceive() when data arrives.
     */
    void listenOn(EndPoint<EndPointType>& listeningEndPoint) {
    }

    /** Send the given data from the given source port (possibly not allocated
     *  yet) to the given destination. This is the core function for outbound
     *  communication.
     */
    void send(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
    }

    /** Stop listening on the given endpoint. You can fully deallocate the
     *  underlying resources for the endpoint.
     */
    void unlisten(EndPoint<EndPointType>& ep) {
    }

    /** Mark this BaseDatagramLayer as invalid, ensuring that no more writes to
     *  the underlying datagram protocol will occur. Also remove this
     *  BaseDatagramLayer from the ConnectionVariables.
     */
    void invalidate() {
    }
};

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

    typedef CallbackTypes<EndPointType> CBTypes;
    typedef typename CBTypes::ConnectionReturnCallbackFunction ConnectionReturnCallbackFunction;
    typedef typename CBTypes::StreamReturnCallbackFunction StreamReturnCallbackFunction;
    typedef typename CBTypes::DatagramSendDoneCallback DatagramSendDoneCallback;
    typedef typename CBTypes::ReadDatagramCallback ReadDatagramCallback;

  friend class Stream<EndPointType>;
  friend class ConnectionManager<EndPointType>;
  friend class BaseDatagramLayer<EndPointType>;

  typedef std::map<EndPoint<EndPointType>, std::tr1::shared_ptr<Connection> >  ConnectionMap;
  typedef std::map<EndPoint<EndPointType>, ConnectionReturnCallbackFunction>  ConnectionReturnCallbackMap;
  typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction> StreamReturnCallbackMap;

  EndPoint<EndPointType> mLocalEndPoint;
  EndPoint<EndPointType> mRemoteEndPoint;

  ConnectionVariables<EndPointType>* mSSTConnVars;
  BaseDatagramLayerPtr mDatagramLayer;

  int mState;
  uint32 mRemoteChannelID;
  uint32 mLocalChannelID;

  uint64 mTransmitSequenceNumber;
  uint64 mLastReceivedSequenceNumber;   //the last transmit sequence number received from the other side

  typedef std::map<LSID, std::tr1::shared_ptr< Stream<EndPointType> > > LSIDStreamMap;
  std::map<LSID, std::tr1::shared_ptr< Stream<EndPointType> > > mOutgoingSubstreamMap;
  std::map<LSID, std::tr1::shared_ptr< Stream<EndPointType> > > mIncomingSubstreamMap;

  std::map<uint32, StreamReturnCallbackFunction> mListeningStreamsCallbackMap;
  std::map<uint32, std::vector<ReadDatagramCallback> > mReadDatagramCallbacks;
  typedef std::vector<std::string> PartialPayloadList;
  typedef std::map<LSID, PartialPayloadList> PartialPayloadMap;
  PartialPayloadMap mPartialReadDatagrams;

  uint32 mNumStreams;

  std::deque< std::tr1::shared_ptr<ChannelSegment> > mQueuedSegments;
  std::deque< std::tr1::shared_ptr<ChannelSegment> > mOutstandingSegments;
  boost::mutex mOutstandingSegmentsMutex;

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

  bool mInSendingMode;

private:

  Connection(ConnectionVariables<EndPointType>* sstConnVars,
             EndPoint<EndPointType> localEndPoint,
             EndPoint<EndPointType> remoteEndPoint)
    : mLocalEndPoint(localEndPoint), mRemoteEndPoint(remoteEndPoint),
      mSSTConnVars(sstConnVars),
      mState(CONNECTION_DISCONNECTED),
      mRemoteChannelID(0), mLocalChannelID(1), mTransmitSequenceNumber(1),
      mLastReceivedSequenceNumber(1),
      mNumStreams(0), mCwnd(1), mRTOMicroseconds(2000000),
      mFirstRTO(true),  MAX_DATAGRAM_SIZE(1000), MAX_PAYLOAD_SIZE(1300),
      MAX_QUEUED_SEGMENTS(3000),
      CC_ALPHA(0.8), mLastTransmitTime(Time::null()),
      mNumInitialRetransmissionAttempts(0),
      mInSendingMode(true)
  {
      mDatagramLayer = sstConnVars->getDatagramLayer(localEndPoint.endPoint);

      mDatagramLayer->listenOn(
          localEndPoint,
          std::tr1::bind(
              &Connection::receiveMessage, this,
              std::tr1::placeholders::_1,
              std::tr1::placeholders::_2
          )
      );

  }

  void checkIfAlive(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
    if (mOutgoingSubstreamMap.size() == 0 && mIncomingSubstreamMap.size() == 0) {
      close(true);
      return;
    }

    getContext()->mainStrand->post(Duration::seconds(300),
        std::tr1::bind(&Connection<EndPointType>::checkIfAlive, this, conn),
        "Connection<EndPointType>::checkIfAlive"
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

  void serviceConnectionNoReturn(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
      serviceConnection(conn);
  }

  bool serviceConnection(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
    const Time curTime = Timer::now();

    boost::mutex::scoped_lock lock(mOutstandingSegmentsMutex);


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

    if (mInSendingMode) {
      boost::mutex::scoped_lock lock(mQueueMutex);

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

	  mLastTransmitTime = curTime;

          if (mState != CONNECTION_PENDING_CONNECT || mNumInitialRetransmissionAttempts > 5) {
            mInSendingMode = false;
            mQueuedSegments.pop_front();
          }
      }

      if (!mInSendingMode || mState == CONNECTION_PENDING_CONNECT) {
        getContext()->mainStrand->post(Duration::microseconds(mRTOMicroseconds*pow(2.0,mNumInitialRetransmissionAttempts)),
            std::tr1::bind(&Connection<EndPointType>::serviceConnectionNoReturn, this, mWeakThis.lock()),
            "Connection<EndPointType>::serviceConnectionNoReturn"
        );
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

      mInSendingMode = true;

      getContext()->mainStrand->post(Duration::microseconds(1),
          std::tr1::bind(&Connection<EndPointType>::serviceConnectionNoReturn, this, mWeakThis.lock()),
          "Connection<EndPointType>::serviceConnectionNoReturn"
      );
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

  static bool createConnection(ConnectionVariables<EndPointType>* sstConnVars,
                               EndPoint <EndPointType> localEndPoint,
			       EndPoint <EndPointType> remoteEndPoint,
                               ConnectionReturnCallbackFunction cb,
			       StreamReturnCallbackFunction scb)

  {
    boost::mutex::scoped_lock lock(sstConnVars->sStaticMembersLock.getMutex());

    ConnectionMap& connectionMap = sstConnVars->sConnectionMap;
    if (connectionMap.find(localEndPoint) != connectionMap.end()) {
      SST_LOG(warn, "sConnectionMap.find failed for " << localEndPoint.endPoint.toString() << "\n");

      return false;
    }

    uint32 availableChannel = sstConnVars->getAvailableChannel(localEndPoint.endPoint);

    if (availableChannel == 0)
      return false;

    std::tr1::shared_ptr<Connection>  conn =  std::tr1::shared_ptr<Connection> (
                       new Connection(sstConnVars, localEndPoint, remoteEndPoint));

    connectionMap[localEndPoint] = conn;
    sstConnVars->sConnectionReturnCallbackMap[localEndPoint] = cb;

    lock.unlock();

    conn->setWeakThis(conn);
    conn->setState(CONNECTION_PENDING_CONNECT);

    uint32 payload[1];
    payload[0] = htonl(availableChannel);

    conn->setLocalChannelID(availableChannel);
    conn->sendData(payload, sizeof(payload), false);

    return true;
  }

  static bool listen(ConnectionVariables<EndPointType>* sstConnVars, StreamReturnCallbackFunction cb, EndPoint<EndPointType> listeningEndPoint) {
      sstConnVars->getDatagramLayer(listeningEndPoint.endPoint)->listenOn(listeningEndPoint);

    boost::mutex::scoped_lock lock(sstConnVars->sStaticMembersLock.getMutex());

    StreamReturnCallbackMap& listeningConnectionsCallbackMap = sstConnVars->sListeningConnectionsCallbackMap;

    if (listeningConnectionsCallbackMap.find(listeningEndPoint) != listeningConnectionsCallbackMap.end()){
      return false;
    }

    listeningConnectionsCallbackMap[listeningEndPoint] = cb;

    return true;
  }

  static bool unlisten(ConnectionVariables<EndPointType>* sstConnVars, EndPoint<EndPointType> listeningEndPoint) {
    BaseDatagramLayer<EndPointType>::stopListening(sstConnVars, listeningEndPoint);

    boost::mutex::scoped_lock lock(sstConnVars->sStaticMembersLock.getMutex());

    sstConnVars->sListeningConnectionsCallbackMap.erase(listeningEndPoint);

    return true;
  }

  void listenStream(uint32 port, StreamReturnCallbackFunction scb) {
    mListeningStreamsCallbackMap[port] = scb;
  }

  void unlistenStream(uint32 port) {
    mListeningStreamsCallbackMap.erase(port);
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

    @return the number of bytes queued from the initial data buffer, or -1 if there was an error.
  */
  virtual int stream(StreamReturnCallbackFunction cb, void* initial_data, int length,
		      uint32 local_port, uint32 remote_port)
  {
    return stream(cb, initial_data, length, local_port, remote_port, 0);
  }

  virtual int stream(StreamReturnCallbackFunction cb, void* initial_data, int length,
                      uint32 local_port, uint32 remote_port, LSID parentLSID)
  {
    USID usid = createNewUSID();
    LSID lsid = ++mNumStreams;

    std::tr1::shared_ptr<Stream<EndPointType> > stream =
      std::tr1::shared_ptr<Stream<EndPointType> >
      ( new Stream<EndPointType>(parentLSID, mWeakThis, local_port, remote_port,  usid, lsid, cb, mSSTConnVars) );
    stream->mWeakThis = stream;
    int numBytesBuffered = stream->init(initial_data, length, false, 0);

    mOutgoingSubstreamMap[lsid]=stream;

    return numBytesBuffered;
  }

  uint64 sendData(const void* data, uint32 length, bool isAck) {
    boost::mutex::scoped_lock lock(mQueueMutex);

    assert(length <= MAX_PAYLOAD_SIZE);

    Sirikata::Protocol::SST::SSTStreamHeader* stream_msg =
                       new Sirikata::Protocol::SST::SSTStreamHeader();

    std::string str = std::string( (char*)data, length);

    bool parsed = parsePBJMessage(stream_msg, str);

    uint64 transmitSequenceNumber =  mTransmitSequenceNumber;

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

        if (mInSendingMode) {
          getContext()->mainStrand->post(Duration::milliseconds(1.0),
              std::tr1::bind(&Connection::serviceConnectionNoReturn, this, mWeakThis.lock()),
              "Connection::serviceConnectionNoReturn"
          );
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

  void setLocalChannelID(uint32 channelID) {
    this->mLocalChannelID = channelID;
  }

  void setRemoteChannelID(uint32 channelID) {
    this->mRemoteChannelID = channelID;
  }

  void setWeakThis( std::tr1::shared_ptr<Connection>  conn) {
    mWeakThis = conn;

    getContext()->mainStrand->post(Duration::seconds(300),
        std::tr1::bind(&Connection<EndPointType>::checkIfAlive, this, conn),
        "Connection<EndPointType>::checkIfAlive"
    );
  }

  USID createNewUSID() {
    uint8 raw_uuid[UUID::static_size];
    for(uint32 ui = 0; ui < UUID::static_size; ui++)
        raw_uuid[ui] = (uint8)rand() % 256;
    UUID id(raw_uuid, UUID::static_size);
    return id;
  }

  void markAcknowledgedPacket(uint64 receivedAckNum) {
    boost::mutex::scoped_lock lock(mOutstandingSegmentsMutex);

    for (std::deque< std::tr1::shared_ptr<ChannelSegment> >::iterator it = mOutstandingSegments.begin();
         it != mOutstandingSegments.end(); it++)
    {
        std::tr1::shared_ptr<ChannelSegment> segment = *it;

        if (!segment) {
          mOutstandingSegments.erase(it);
          it = mOutstandingSegments.begin();
          continue;
        }

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

          mInSendingMode = true;

          std::tr1::shared_ptr<Connection<EndPointType> > conn = mWeakThis.lock();
          if (conn) {
            getContext()->mainStrand->post(
                std::tr1::bind(&Connection<EndPointType>::serviceConnectionNoReturn, this, conn),
                "Connection<EndPointType>::serviceConnectionNoReturn"
            );
          }

          if (rand() % mCwnd == 0)  {
            mCwnd += 1;
          }

          mOutstandingSegments.erase(it);
          break;
        }
    }
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
				     NULL, mSSTConnVars));
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
	SST_LOG(warn, mLocalEndPoint.endPoint.toString()  << " not listening to streams at: " << received_stream_msg->dest_port() << "\n");
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
	SST_LOG(detailed, "Received reply packet for unknown stream: " <<  initiatingLSID  <<"\n");
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
          uint32 dest_port = received_stream_msg->dest_port();
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

      uint32* received_payload = (uint32*) received_msg->payload().data();

      setRemoteChannelID( ntohl(received_payload[0]));
      mRemoteEndPoint.port = ntohl(received_payload[1]);

      sendData( received_payload, 0, false );

      boost::mutex::scoped_lock lock(mSSTConnVars->sStaticMembersLock.getMutex());

      ConnectionReturnCallbackMap& connectionReturnCallbackMap = mSSTConnVars->sConnectionReturnCallbackMap;
      ConnectionMap& connectionMap = mSSTConnVars->sConnectionMap;

      if (connectionReturnCallbackMap.find(mLocalEndPoint) != connectionReturnCallbackMap.end())
      {
        if (connectionMap.find(mLocalEndPoint) != connectionMap.end()) {
          std::tr1::shared_ptr<Connection> conn = connectionMap[mLocalEndPoint];

          connectionReturnCallbackMap[mLocalEndPoint] (SST_IMPL_SUCCESS, conn);
        }
        connectionReturnCallbackMap.erase(mLocalEndPoint);
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

    if (mOutgoingSubstreamMap.size() == 0 && mIncomingSubstreamMap.size() == 0) {
      close(true);
    }
  }


  // This is the version of cleanup is used from all the normal methods in Connection
  static void cleanup(std::tr1::shared_ptr<Connection<EndPointType> > conn) {
    conn->mDatagramLayer->unlisten(conn->mLocalEndPoint);

    int connState = conn->mState;

    if (connState == CONNECTION_PENDING_CONNECT || connState == CONNECTION_DISCONNECTED) {
      //Deal with the connection not getting connected with the remote endpoint.
      //This is in contrast to the case where the connection got connected, but
      //the connection's root stream was unable to do so.

       boost::mutex::scoped_lock lock(conn->mSSTConnVars->sStaticMembersLock.getMutex());
       ConnectionReturnCallbackFunction cb = NULL;

       ConnectionReturnCallbackMap& connectionReturnCallbackMap = conn->mSSTConnVars->sConnectionReturnCallbackMap;
       if (connectionReturnCallbackMap.find(conn->localEndPoint()) != connectionReturnCallbackMap.end()) {
         cb = connectionReturnCallbackMap[conn->localEndPoint()];
       }

       std::tr1::shared_ptr<Connection>  failed_conn = conn;

       connectionReturnCallbackMap.erase(conn->localEndPoint());
       conn->mSSTConnVars->sConnectionMap.erase(conn->localEndPoint());

       lock.unlock();


       if (connState == CONNECTION_PENDING_CONNECT && cb ) {
         cb(SST_IMPL_FAILURE, failed_conn);
       }

       conn->mState = CONNECTION_DISCONNECTED;
     }
   }

   // This version should only be called by the destructor!
   void finalCleanup() {
     mDatagramLayer->unlisten(mLocalEndPoint);

     if (mState != CONNECTION_DISCONNECTED) {
         close(true);
         mState = CONNECTION_DISCONNECTED;
     }

     mSSTConnVars->releaseChannel(mLocalEndPoint.endPoint, mLocalChannelID);
   }

   static void closeConnections(ConnectionVariables<EndPointType>* sstConnVars) {
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
       while(!sstConnVars->sConnectionMap.empty()) {
           ConnectionMap& connectionMap = sstConnVars->sConnectionMap;

           ConnectionPtr saved = connectionMap.begin()->second;
           connectionMap.erase(connectionMap.begin());
           saved.reset();
       }
   }

   static void handleReceive(ConnectionVariables<EndPointType>* sstConnVars,
                             EndPoint<EndPointType> remoteEndPoint,
                             EndPoint<EndPointType> localEndPoint, void* recv_buffer, int len)
   {
     char* data = (char*) recv_buffer;
     std::string str = std::string(data, len);

     Sirikata::Protocol::SST::SSTChannelHeader* received_msg = new Sirikata::Protocol::SST::SSTChannelHeader();
     bool parsed = parsePBJMessage(received_msg, str);

     uint8 channelID = received_msg->channel_id();

     boost::mutex::scoped_lock lock(sstConnVars->sStaticMembersLock.getMutex());

     ConnectionMap& connectionMap = sstConnVars->sConnectionMap;
     if (connectionMap.find(localEndPoint) != connectionMap.end()) {
       if (channelID == 0) {
 	/*Someone's already connected at this port. Either don't reply or
 	  send back a request rejected message. */

        SST_LOG(info, "Someone's already connected at this port on object " << localEndPoint.endPoint.toString() << "\n");
 	return;
       }
       std::tr1::shared_ptr<Connection<EndPointType> > conn = connectionMap[localEndPoint];

       conn->receiveMessage(data, len);
     }
     else if (channelID == 0) {
       /* it's a new channel request negotiation protocol
 	        packet ; allocate a new channel.*/

       StreamReturnCallbackMap& listeningConnectionsCallbackMap = sstConnVars->sListeningConnectionsCallbackMap;
       if (listeningConnectionsCallbackMap.find(localEndPoint) != listeningConnectionsCallbackMap.end()) {
         uint32* received_payload = (uint32*) received_msg->payload().data();

         uint32 payload[2];

         uint32 availableChannel = sstConnVars->getAvailableChannel(localEndPoint.endPoint);
         payload[0] = htonl(availableChannel);
         uint32 availablePort = availableChannel; //availableChannel is picked from the same 16-bit
                                                  //address space and has to be unique. So why not use
                                                  //use it to identify the port as well...
         payload[1] = htonl(availablePort);

         EndPoint<EndPointType> newLocalEndPoint(localEndPoint.endPoint, availablePort);
         std::tr1::shared_ptr<Connection>  conn =
                    std::tr1::shared_ptr<Connection>(
                         new Connection(sstConnVars, newLocalEndPoint, remoteEndPoint));


         conn->listenStream(newLocalEndPoint.port, listeningConnectionsCallbackMap[localEndPoint]);
         conn->setWeakThis(conn);
         connectionMap[newLocalEndPoint] = conn;

         conn->setLocalChannelID(availableChannel);
         conn->setRemoteChannelID(ntohl(received_payload[0]));
         conn->setState(CONNECTION_PENDING_RECEIVE_CONNECT);

         conn->sendData(payload, sizeof(payload), false);
       }
       else {
         SST_LOG(warn, "No one listening on this connection\n");
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
  virtual bool datagram( void* data, int length, uint32 local_port, uint32 remote_port,
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
  virtual bool registerReadDatagramCallback(uint32 port, ReadDatagramCallback cb) {
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
      boost::mutex::scoped_lock lock(mSSTConnVars->sStaticMembersLock.getMutex());
      mSSTConnVars->sConnectionMap.erase(mLocalEndPoint);
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
    mBuffer = new uint8[len+1];

    if (len > 0) {
      memcpy(mBuffer,data,len);
    }

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

    typedef CallbackTypes<EndPointType> CBTypes;
    typedef typename CBTypes::StreamReturnCallbackFunction StreamReturnCallbackFunction;
    typedef typename CBTypes::ReadCallback ReadCallback;

    typedef std::map<EndPoint<EndPointType>, StreamReturnCallbackFunction> StreamReturnCallbackMap;

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

  bool connected() { return mConnected; }

  static bool connectStream(ConnectionVariables<EndPointType>* sstConnVars,
                            EndPoint <EndPointType> localEndPoint,
			    EndPoint <EndPointType> remoteEndPoint,
			    StreamReturnCallbackFunction cb)
  {
      if (localEndPoint.port == 0) {
          typename BaseDatagramLayer<EndPointType>::Ptr bdl = sstConnVars->getDatagramLayer(localEndPoint.endPoint);
          if (!bdl) {
              SST_LOG(error,"Tried to connect stream without calling createDatagramLayer for the endpoint.");
              return false;
          }
          localEndPoint.port = bdl->getUnusedPort(localEndPoint.endPoint);
      }

      StreamReturnCallbackMap& streamReturnCallbackMap = sstConnVars->mStreamReturnCallbackMap;
      if (streamReturnCallbackMap.find(localEndPoint) != streamReturnCallbackMap.end()) {
        return false;
      }

      streamReturnCallbackMap[localEndPoint] = cb;

      bool result = Connection<EndPointType>::createConnection(sstConnVars,
                                                               localEndPoint,
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
  static bool listen(ConnectionVariables<EndPointType>* sstConnVars, StreamReturnCallbackFunction cb, EndPoint <EndPointType> listeningEndPoint) {
    return Connection<EndPointType>::listen(sstConnVars, cb, listeningEndPoint);
  }

  static bool unlisten(ConnectionVariables<EndPointType>* sstConnVars, EndPoint <EndPointType> listeningEndPoint) {
    return Connection<EndPointType>::unlisten(sstConnVars, listeningEndPoint);
  }

  /*
    Start listening for child streams on the specified port. A remote stream
    can only create child streams under this stream if this stream is listening
    on the port specified for the child stream.

    @param scb the callback function invoked when a new stream is created
    @param port the endpoint where SST will accept new incoming
           streams.
  */
  void listenSubstream(uint32 port, StreamReturnCallbackFunction scb) {
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    if (!conn) {
      scb(SST_IMPL_FAILURE, StreamPtr() );
      return;
    }

    conn->listenStream(port, scb);
  }

  void unlistenSubstream(uint32 port) {
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();

    if (conn)
      conn->unlistenStream(port);
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
            std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
            "Stream<EndPointType>::serviceStreamNoReturn"
        );

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
            std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
            "Stream<EndPointType>::serviceStreamNoReturn"
        );

      return currOffset;
    }

    return -1;
  }

#if SIRIKATA_PLATFORM != SIRIKATA_PLATFORM_WINDOWS
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

      if (conn) {
        conn->eraseDisconnectedStream(this);
      }

      return true;
    }
    else {
      mState = PENDING_DISCONNECT;
      if (conn) {
          getContext()->mainStrand->post(
              std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
              "Stream<EndPointType>::serviceStreamNoReturn"
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

     @return the number of bytes actually buffered from the initial data buffer specified, or
     -1 if an error occurred.
  */
  virtual int createChildStream(StreamReturnCallbackFunction cb, void* data, int length,
				 uint32 local_port, uint32 remote_port)
  {
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    if (conn) {
      return conn->stream(cb, data, length, local_port, remote_port, mLSID);
    }

    return -1;
  }

  /*
    Returns the local endpoint to which this connection is bound.

    @return the local endpoint.
  */
  virtual EndPoint <EndPointType> localEndPoint()  {
    return mLocalEndPoint;
  }

  /*
    Returns the remote endpoint to which this connection is bound.

    @return the remote endpoint.
  */
  virtual EndPoint <EndPointType> remoteEndPoint()  {
    return mRemoteEndPoint;
  }

  virtual uint8 getState() {
    return mState;
  }

  const Context* getContext() {
    if (mContext == NULL) {
      std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
      assert(conn);

      mContext = conn->getContext();
    }

    return mContext;
  }

private:
  Stream(LSID parentLSID, std::tr1::weak_ptr<Connection<EndPointType> > conn,
	 uint32 local_port, uint32 remote_port,
	 USID usid, LSID lsid, StreamReturnCallbackFunction cb, ConnectionVariables<EndPointType>* sstConnVars)
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
    mStreamRTOMicroseconds(2000000),
    FL_ALPHA(0.8),
    mTransmitWindowSize(MAX_RECEIVE_WINDOW),
    mReceiveWindowSize(MAX_RECEIVE_WINDOW),
    mNumOutstandingBytes(0),
    mNextByteExpected(0),
    mLastContiguousByteReceived(-1),
    mLastSendTime(Time::null()),
    mLastReceiveTime(Time::null()),
    mStreamReturnCallback(cb),
    mConnected (false),
    MAX_INIT_RETRANSMISSIONS(5),
    mSSTConnVars(sstConnVars)
  {
    mInitialData = NULL;
    mInitialDataLength = 0;

    mReceiveBuffer = NULL;
    mReceiveBitmap = NULL;

    mQueuedBuffers.clear();
    mCurrentQueueLength = 0;

    std::tr1::shared_ptr<Connection<EndPointType> > locked_conn = mConnection.lock();
    mRemoteEndPoint = EndPoint<EndPointType> (locked_conn->remoteEndPoint().endPoint, mRemotePort);
    mLocalEndPoint = EndPoint<EndPointType> (locked_conn->localEndPoint().endPoint, mLocalPort);

    // Continues in init, when we have mWeakThis set
  }

  int init(void* initial_data, uint32 length, bool remotelyInitiated, LSID remoteLSID) {
    mNumInitRetransmissions = 1;
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
    int numBytesBuffered = mInitialDataLength;

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

    mNumBytesSent = mInitialDataLength;

    if (length > mInitialDataLength) {
      int writeval = write( ((uint8*)initial_data) + mInitialDataLength, length - mInitialDataLength);

      if (writeval >= 0) {
        numBytesBuffered += writeval;
      }
    }

    /** Post a keep-alive task...  **/
    std::tr1::shared_ptr<Connection<EndPointType> > conn = mConnection.lock();
    if (conn) {
      getContext()->mainStrand->post(Duration::seconds(60),
          std::tr1::bind(&Stream<EndPointType>::sendKeepAlive, this, mWeakThis, conn),
          "Stream<EndPointType>::sendKeepAlive"
      );
    }

    return numBytesBuffered;
  }

  uint8* receiveBuffer() {
      if (mReceiveBuffer == NULL)
          mReceiveBuffer = new uint8[MAX_RECEIVE_WINDOW];
      return mReceiveBuffer;
  }

  uint8* receiveBitmap() {
      if (mReceiveBitmap == NULL) {
          mReceiveBitmap = new uint8[MAX_RECEIVE_WINDOW];
          memset(mReceiveBitmap, 0, MAX_RECEIVE_WINDOW);
      }
      return mReceiveBitmap;
  }

  void initRemoteLSID(LSID remoteLSID) {
      mRemoteLSID = remoteLSID;
  }

  void sendKeepAlive(std::tr1::weak_ptr<Stream<EndPointType> > wstrm, std::tr1::shared_ptr<Connection<EndPointType> > conn) {
      std::tr1::shared_ptr<Stream<EndPointType> > strm = wstrm.lock();
      if (!strm) return;

    if (mState == DISCONNECTED || mState == PENDING_DISCONNECT) {
      close(true);
      return;
    }

    uint8 buf[1];

    write(buf, 0);

    getContext()->mainStrand->post(Duration::seconds(60),
        std::tr1::bind(&Stream<EndPointType>::sendKeepAlive, this, wstrm, conn),
        "Stream<EndPointType>::sendKeepAlive"
    );
  }

  static void connectionCreated( int errCode, std::tr1::shared_ptr<Connection<EndPointType> > c) {
    StreamReturnCallbackMap& streamReturnCallbackMap = c->mSSTConnVars->mStreamReturnCallbackMap;
    assert(streamReturnCallbackMap.find(c->localEndPoint()) != streamReturnCallbackMap.end());

    if (errCode != SST_IMPL_SUCCESS) {

      StreamReturnCallbackFunction cb = streamReturnCallbackMap[c->localEndPoint()];
      streamReturnCallbackMap.erase(c->localEndPoint());

      cb(SST_IMPL_FAILURE, StreamPtr() );

      return;
    }

    c->stream(streamReturnCallbackMap[c->localEndPoint()], NULL , 0,
	      c->localEndPoint().port, c->remoteEndPoint().port);

    streamReturnCallbackMap.erase(c->localEndPoint());
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

    if ( (curTime - mLastReceiveTime).toSeconds() > 300 && mLastReceiveTime != Time::null())
    {
      close(true);
      return true;
    }

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

        mSSTConnVars->mStreamReturnCallbackMap.erase(conn->localEndPoint());

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


        conn->eraseDisconnectedStream(this);
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
            getContext()->mainStrand->post(
                std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
                "Stream<EndPointType>::serviceStreamNoReturn"
            );
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
                std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
                "Stream<EndPointType>::serviceStreamNoReturn"
            );
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
          std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
          "Stream<EndPointType>::serviceStreamNoReturn"
      );

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
      // Special case: if we're not marking any data as skipped and we
      // haven't allocated the receive bitmap yet, then we're not
      // going to send anything anyway. Just ignore this call.
      if (mReceiveBitmap == NULL && skipLength == 0)
          return;

    uint32 readyBufferSize = skipLength;
    uint8* recv_bmap = receiveBitmap();
    for (uint32 i=skipLength; i < MAX_RECEIVE_WINDOW; i++) {
        if (recv_bmap[i] == 1) {
	readyBufferSize++;
      }
      else if (recv_bmap[i] == 0) {
	break;
      }
    }

    //pass data up to the app from 0 to readyBufferSize;
    //
    if (mReadCallback != NULL && readyBufferSize > 0) {
        uint8* recv_buf = receiveBuffer();
        mReadCallback(recv_buf, readyBufferSize);

      //now move the window forward...
      mLastContiguousByteReceived = mLastContiguousByteReceived + readyBufferSize;
      mNextByteExpected = mLastContiguousByteReceived + 1;

      uint8* recv_bmap = receiveBitmap();
      memmove(recv_bmap, recv_bmap + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);
      memset(recv_bmap + (MAX_RECEIVE_WINDOW - readyBufferSize), 0, readyBufferSize);

      memmove(recv_buf, recv_buf + readyBufferSize, MAX_RECEIVE_WINDOW - readyBufferSize);

      mReceiveWindowSize += readyBufferSize;
    }
  }

  void receiveData( Sirikata::Protocol::SST::SSTStreamHeader* streamMsg,
		    const void* buffer, uint64 offset, uint32 len )
  {
    const Time curTime = Timer::now();
    mLastReceiveTime = curTime;

    if (streamMsg->type() == streamMsg->REPLY) {
      mConnected = true;
    }
    else if (streamMsg->type() == streamMsg->DATA || streamMsg->type() == streamMsg->INIT) {
      boost::recursive_mutex::scoped_lock lock(mReceiveBufferMutex);

      int transmitWindowSize = pow(2.0, streamMsg->window()) - mNumOutstandingBytes;
      if (transmitWindowSize >= 0) {
        mTransmitWindowSize = transmitWindowSize;
      }
      else {
        mTransmitWindowSize = 0;
      }


      /*std::cout << "offset=" << offset << " , mLastContiguousByteReceived=" << mLastContiguousByteReceived
        << " , mNextByteExpected=" << mNextByteExpected <<"\n";*/

      int64 offsetInBuffer = offset - mLastContiguousByteReceived - 1;
      if ( len > 0 &&  (int64)(offset) == mNextByteExpected) {
        if (offsetInBuffer + len <= MAX_RECEIVE_WINDOW) {
	  mReceiveWindowSize -= len;

	  memcpy(receiveBuffer()+offsetInBuffer, buffer, len);
	  memset(receiveBitmap()+offsetInBuffer, 1, len);

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

   	  memcpy(receiveBuffer()+offsetInBuffer, buffer, len);
	  memset(receiveBitmap()+offsetInBuffer, 1, len);

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

      mChannelToBufferMap[offset]->mAckTime = curTime;

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
      SST_LOG(insane, "Bad sample\n");
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

    getContext()->mainStrand->post(
        Duration::microseconds(pow(2.0,mNumInitRetransmissions)*mStreamRTOMicroseconds),
        std::tr1::bind(&Stream<EndPointType>::serviceStreamNoReturn, this, mWeakThis.lock(), conn),
        "Stream<EndPointType>::serviceStreamNoReturn"
    );

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

  uint32 mLocalPort;
  uint32 mRemotePort;

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
  Time mLastReceiveTime;

  uint8* mReceiveBuffer;
  uint8* mReceiveBitmap;
  boost::recursive_mutex mReceiveBufferMutex;

  ReadCallback mReadCallback;
  StreamReturnCallbackFunction mStreamReturnCallback;

  friend class Connection<EndPointType>;

  /* Variables required for the initial connection */
  bool mConnected;
  uint8* mInitialData;
  uint16 mInitialDataLength;
  uint8 mNumInitRetransmissions;
  uint8 MAX_INIT_RETRANSMISSIONS;

  ConnectionVariables<EndPointType>* mSSTConnVars;

  std::tr1::weak_ptr<Stream<EndPointType> > mWeakThis;

  /** Store the endpoints here to avoid talking to mConnection. It's ok
   to do this because the endpoints never change for an SST Stream.**/
  EndPoint <EndPointType> mLocalEndPoint;
  EndPoint <EndPointType> mRemoteEndPoint;

};


/**
 Manages SST Connections. All calls creating new top-level streams, listening on endpoints,
 or creating the underlying datagram layer go through here. This class maintains the data structures
 needed by every new SST Stream or Connection.

 This class is only instantiated once per process (usually in main()) and is then
 accessible through SpaceContext and ObjectHostContext.
 */
template <class EndPointType>
class ConnectionManager : public Service {
public:
  typedef std::tr1::shared_ptr<BaseDatagramLayer<EndPointType> > BaseDatagramLayerPtr;

    typedef CallbackTypes<EndPointType> CBTypes;
    typedef typename CBTypes::StreamReturnCallbackFunction StreamReturnCallbackFunction;

  virtual void start() {
  }

  virtual void stop() {
    Connection<EndPointType>::closeConnections(&mSSTConnVars);
  }

  ~ConnectionManager() {
    Connection<EndPointType>::closeConnections(&mSSTConnVars);
  }

  bool connectStream(EndPoint <EndPointType> localEndPoint,
                     EndPoint <EndPointType> remoteEndPoint,
                     StreamReturnCallbackFunction cb)
  {
    return Stream<EndPointType>::connectStream(&mSSTConnVars, localEndPoint, remoteEndPoint, cb);
  }

    // The BaseDatagramLayer is really where the interaction with the underlying
    // system happens, and different underlying protocols may require different
    // parameters. These need to be instantiated by the client code anyway (to
    // generate the interface), so we provide some templatized versions to allow
    // a variable number of arguments.
    template<typename A1>
    BaseDatagramLayerPtr createDatagramLayer(EndPointType endPoint, Context* ctx, A1 a1) {
        return BaseDatagramLayer<EndPointType>::createDatagramLayer(&mSSTConnVars, endPoint, ctx, a1);
    }
    template<typename A1, typename A2>
    BaseDatagramLayerPtr createDatagramLayer(EndPointType endPoint, Context* ctx, A1 a1, A2 a2) {
        return BaseDatagramLayer<EndPointType>::createDatagramLayer(&mSSTConnVars, endPoint, ctx, a1, a2);
    }
    template<typename A1, typename A2, typename A3>
    BaseDatagramLayerPtr createDatagramLayer(EndPointType endPoint, Context* ctx, A1 a1, A2 a2, A3 a3) {
        return BaseDatagramLayer<EndPointType>::createDatagramLayer(&mSSTConnVars, endPoint, ctx, a1, a2, a3);
    }

  BaseDatagramLayerPtr getDatagramLayer(EndPointType endPoint) {
    return mSSTConnVars.getDatagramLayer(endPoint);
  }

  bool listen(StreamReturnCallbackFunction cb, EndPoint <EndPointType> listeningEndPoint) {
    return Stream<EndPointType>::listen(&mSSTConnVars, cb, listeningEndPoint);
  }

  bool unlisten( EndPoint <EndPointType> listeningEndPoint) {
    return Stream<EndPointType>::unlisten(&mSSTConnVars, listeningEndPoint);
  }

  //Storage class for SST's global variables.
  ConnectionVariables<EndPointType> mSSTConnVars;
};



} // namespace SST
} // namespace Sirikata

#endif
