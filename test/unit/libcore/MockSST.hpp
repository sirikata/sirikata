// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_TEST_LIBCORE_MOCK_SST_HPP_
#define _SIRIKATA_TEST_LIBCORE_MOCK_SST_HPP_

// Typedefs and specializations for a mock SST implementation. The
// BaseDatagramLayer supports controlling behavior (drops, delay) to test under
// different network conditions

#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/util/Random.hpp>

namespace Sirikata {

// Convenience typedefs in a separate namespace
namespace Mock {

// Create a special ID type. It's just a string (easy for debugging), but we
// need to provide the basic interface for endpoint IDs.
class ID {
public:
    ID()
     : mID()
    {}
    ID(const ID& rhs)
     : mID(rhs.mID)
    {}
    ID(const String& s)
     : mID(s)
    {}

    const String& toString() const { return mID; }


    bool operator<(const ID& rhs) const { return mID < rhs.mID; }
    bool operator==(const ID& rhs) const { return mID == rhs.mID; }
    bool operator!=(const ID& rhs) const { return mID != rhs.mID; }
    class Hasher {
    public:
        size_t operator()(const ID& objr) const {
            return std::tr1::hash<std::string>()(objr.mID);
        }
    };

private:
    String mID;
};

// SST Typedefs
typedef Sirikata::SST::EndPoint<ID> Endpoint;
typedef Sirikata::SST::BaseDatagramLayer<ID> BaseDatagramLayer;
typedef Sirikata::SST::Connection<ID> Connection;
typedef Sirikata::SST::Stream<ID> Stream;
typedef Sirikata::SST::ConnectionManager<ID> ConnectionManager;

// And this is the real implementation of the datagram layer.
class Service {
public:
    Service(Context* ctx)
     : mContext(ctx),
       mDelay(Duration::zero()),
       mDropRate(0)
    {}

    // src, src port, dst, dst port, data*, data size
    typedef std::tr1::function<void(const ID&, const ObjectMessagePort, const ID, const ObjectMessagePort, void* , uint32)> DatagramCallback;

    void setDelay(Duration d) { mDelay = d; }
    void setDropRate(float32 d) { mDropRate = d; }

    void listen(const ID& ep, ObjectMessagePort port, DatagramCallback cb) {
        // Just record this in our map, but also make sure we haven't already
        // recorded it
        PortHandlerMap& handlers = mHandlers[ep];
        TS_ASSERT(handlers.find(port) == handlers.end());
        handlers[port] = cb;
    }
    void unlisten(const ID& ep, ObjectMessagePort port) {
        PortHandlerMap& handlers = mHandlers[ep];
        // FIXME we get duplicate calls for this on cleanup, causing it to
        //fail. It's not really unsafe, but we really shouldn't get multiple
        //calls. Someone should fix this.
        //TS_ASSERT(handlers.find(port) != handlers.end());
        handlers.erase(port);
    }

    void send(const ID& src, const ObjectMessagePort src_port, const ID dst, const ObjectMessagePort dst_port, void* payload, uint32 payload_size) {
        if (mDropRate > 0 && randFloat() < mDropRate)
            return;

        // Simple deferment so we don't have recursive handling/sending here
        if (mDelay != Duration::zero())
            mContext->mainStrand->post(
                mDelay,
                std::tr1::bind(&Service::deliver, this,
                    src, src_port,
                    dst, dst_port,
                    String((char*)payload, payload_size)
                )
            );
        else
            mContext->mainStrand->post(
                std::tr1::bind(&Service::deliver, this,
                    src, src_port,
                    dst, dst_port,
                    String((char*)payload, payload_size)
                )
            );
    }

    ObjectMessagePort unused(const ID& ep) {
        PortHandlerMap& handlers = mHandlers[ep];
        ObjectMessagePort idx = 1;
        while(handlers.find(idx) != handlers.end())
            idx++;
        return (ObjectMessagePort)idx;
    }

private:
    void deliver(const ID& src, const ObjectMessagePort src_port, const ID dst, const ObjectMessagePort dst_port, String payload) {
        if (mHandlers.find(dst) == mHandlers.end()) return;
        PortHandlerMap& handlers = mHandlers[dst];
        if (handlers.find(dst_port) == handlers.end()) return;
        handlers[dst_port](src, src_port, dst, dst_port, (void*)payload.c_str(), payload.size());
    }

    Context *mContext;

    typedef std::map<ObjectMessagePort, DatagramCallback> PortHandlerMap;
    typedef std::map<ID, PortHandlerMap> EndpointMap;
    EndpointMap mHandlers;

    // Channel conditions
    Duration mDelay;
    float32 mDropRate;
};

} // namespace Mock

// Mock-specific implementation
namespace SST {

template <>
class SIRIKATA_EXPORT BaseDatagramLayer<Mock::ID>
{
  private:
    typedef Mock::ID EndPointType;

  public:
    typedef std::tr1::shared_ptr<BaseDatagramLayer<EndPointType> > Ptr;
    typedef Ptr BaseDatagramLayerPtr;

    typedef std::tr1::function<void(void*, int)> DataCallback;

    static BaseDatagramLayerPtr getDatagramLayer(ConnectionVariables<EndPointType>* sstConnVars,
                                                 EndPointType endPoint)
    {
        return sstConnVars->getDatagramLayer(endPoint);
    }

    static BaseDatagramLayerPtr createDatagramLayer(
        ConnectionVariables<EndPointType>* sstConnVars,
        EndPointType endPoint,
        const Context* ctx,
        Mock::Service* mockdp)
    {
        BaseDatagramLayerPtr datagramLayer = getDatagramLayer(sstConnVars, endPoint);
        if (datagramLayer) return datagramLayer;

        datagramLayer = BaseDatagramLayerPtr(
            new BaseDatagramLayer(sstConnVars, ctx, mockdp, endPoint)
        );
        sstConnVars->addDatagramLayer(endPoint, datagramLayer);

        return datagramLayer;
    }

    static void stopListening(ConnectionVariables<EndPointType>* sstConnVars, EndPoint<EndPointType>& listeningEndPoint) {
        EndPointType endPointID = listeningEndPoint.endPoint;

        BaseDatagramLayerPtr bdl = sstConnVars->getDatagramLayer(endPointID);
        if (!bdl) return;
        sstConnVars->removeDatagramLayer(endPointID, true);
        bdl->unlisten(listeningEndPoint);
    }

    void listenOn(EndPoint<EndPointType>& listeningEndPoint, DataCallback cb) {
        mMock->listen(
            listeningEndPoint.endPoint, listeningEndPoint.port,
            std::tr1::bind(
                &BaseDatagramLayer::receiveMessageToCallback, this,
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3,
                std::tr1::placeholders::_4,
                std::tr1::placeholders::_5,
                std::tr1::placeholders::_6,
                cb
            )
        );
    }

    void listenOn(const EndPoint<EndPointType>& listeningEndPoint) {
        mMock->listen(
            listeningEndPoint.endPoint, listeningEndPoint.port,
            std::tr1::bind(
                &BaseDatagramLayer::receiveMessage, this,
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3,
                std::tr1::placeholders::_4,
                std::tr1::placeholders::_5,
                std::tr1::placeholders::_6
            )
        );
    }

    void unlisten(EndPoint<EndPointType>& ep) {
        mMock->unlisten(ep.endPoint, ep.port);
    }

    void send(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
        mMock->send(
            src->endPoint, src->port,
            dest->endPoint, dest->port,
            data, len
        );
    }

    const Context* context() {
        return mContext;
    }

    uint32 getUnusedPort(const EndPointType& ep) {
        return mMock->unused(ep);
    }

    void invalidate() {
        mMock = NULL;
        mSSTConnVars->removeDatagramLayer(mEndpoint, true);
    }

  private:
    BaseDatagramLayer(ConnectionVariables<EndPointType>* sstConnVars, const Context* ctx, Mock::Service* mockdp, const EndPointType&ep)
        : mContext(ctx),
          mMock(mockdp),
          mSSTConnVars(sstConnVars),
          mEndpoint(ep)
        {

        }

    void receiveMessage(const Mock::ID& src, const ObjectMessagePort src_port, const Mock::ID dst, const ObjectMessagePort dst_port, void* payload, uint32 payload_size) {
        Connection<EndPointType>::handleReceive(
            mSSTConnVars,
            EndPoint<EndPointType> (src, src_port),
            EndPoint<EndPointType> (dst, dst_port),
            payload, payload_size
        );
    }

    void receiveMessageToCallback(const Mock::ID& src, const ObjectMessagePort src_port, const Mock::ID dst, const ObjectMessagePort dst_port, void* payload, uint32 payload_size, DataCallback cb) {
        cb(payload, payload_size );
    }



    const Context* mContext;
    Mock::Service* mMock;

    ConnectionVariables<EndPointType>* mSSTConnVars;
    EndPointType mEndpoint;
};

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
  // These exports keep Windows happy by forcing the export of these
  // types. BaseDatagramLayer is now excluded because it is explicitly
  // specialized, which, for some reason, keeps things working
  // properly.
  //SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT BaseDatagramLayer<Mock::ID>;
  SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Connection<Mock::ID>;
  SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Stream<Mock::ID>;
#endif

} // namespace SST

} // namespace Sirikata

#endif //_SIRIKATA_TEST_LIBCORE_MOCK_SST_HPP_
