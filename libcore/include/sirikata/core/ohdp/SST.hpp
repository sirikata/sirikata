// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_OHDP_SST_HPP_
#define _SIRIKATA_LIBCORE_OHDP_SST_HPP_

#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/ohdp/Service.hpp>

namespace Sirikata {

// Convenience typedefs in a separate namespace
namespace OHDPSST {
typedef Sirikata::SST::EndPoint<OHDP::SpaceNodeID> Endpoint;
typedef Sirikata::SST::BaseDatagramLayer<OHDP::SpaceNodeID> BaseDatagramLayer;
typedef Sirikata::SST::Connection<OHDP::SpaceNodeID> Connection;
typedef Sirikata::SST::Stream<OHDP::SpaceNodeID> Stream;
typedef Sirikata::SST::ConnectionManager<OHDP::SpaceNodeID> ConnectionManager;
} // namespace OHDPSST

// OHDP::SpaceNodeID/OHDP-specific implementation
namespace SST {

template <>
class SIRIKATA_EXPORT BaseDatagramLayer<OHDP::SpaceNodeID>
{
  private:
    typedef OHDP::SpaceNodeID EndPointType;

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
        OHDP::Service* ohdp)
    {
        BaseDatagramLayerPtr datagramLayer = getDatagramLayer(sstConnVars, endPoint);
        if (datagramLayer) return datagramLayer;

        datagramLayer = BaseDatagramLayerPtr(
            new BaseDatagramLayer(sstConnVars, ctx, ohdp, endPoint)
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
      OHDP::Port* port = allocatePort(listeningEndPoint);
        port->receive(
            std::tr1::bind(&BaseDatagramLayer<EndPointType>::receiveMessageToCallback, this,
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3,
                cb
            )
        );
    }

    void listenOn(const EndPoint<EndPointType>& listeningEndPoint) {
        OHDP::Port* port = allocatePort(listeningEndPoint);
        port->receive(
            std::tr1::bind(
                &BaseDatagramLayer::receiveMessage, this,
                std::tr1::placeholders::_1,
                std::tr1::placeholders::_2,
                std::tr1::placeholders::_3
            )
        );
    }

    void unlisten(EndPoint<EndPointType>& ep) {
        // To stop listening, just destroy the corresponding port
        PortMap::iterator it = mAllocatedPorts.find(ep);
        if (it == mAllocatedPorts.end()) return;
        delete it->second;
        mAllocatedPorts.erase(it);
    }

    void send(EndPoint<EndPointType>* src, EndPoint<EndPointType>* dest, void* data, int len) {
        boost::mutex::scoped_lock lock(mMutex);

        OHDP::Port* port = getOrAllocatePort(*src);

        port->send(
            OHDP::Endpoint(dest->endPoint, dest->port),
            MemoryReference(data, len)
        );
    }

    const Context* context() {
        return mContext;
    }

    uint32 getUnusedPort(const EndPointType& ep) {
        return mOHDP->unusedOHDPPort(ep.space(), ep.node());
    }

    void invalidate() {
        mOHDP = NULL;
        mSSTConnVars->removeDatagramLayer(mEndpoint, true);
    }

  private:
    BaseDatagramLayer(ConnectionVariables<EndPointType>* sstConnVars, const Context* ctx, OHDP::Service* ohdpservice, const EndPointType&ep)
        : mContext(ctx),
          mOHDP(ohdpservice),
          mSSTConnVars(sstConnVars),
          mEndpoint(ep)
        {

        }

    OHDP::Port* allocatePort(const EndPoint<EndPointType>& ep) {
        OHDP::Port* port = mOHDP->bindOHDPPort(
            ep.endPoint.space(), ep.endPoint.node(), ep.port
        );
        mAllocatedPorts[ep] = port;
        return port;
    }

    OHDP::Port* getPort(const EndPoint<EndPointType>& ep) {
        PortMap::iterator it = mAllocatedPorts.find(ep);
        if (it == mAllocatedPorts.end()) return NULL;
        return it->second;
    }

    OHDP::Port* getOrAllocatePort(const EndPoint<EndPointType>& ep) {
        OHDP::Port* result = getPort(ep);
        if (result != NULL) return result;
        result = allocatePort(ep);
        return result;
    }

    void receiveMessage(const OHDP::Endpoint &src, const OHDP::Endpoint &dst, MemoryReference payload) {
        Connection<EndPointType>::handleReceive(
            mSSTConnVars,
            EndPoint<EndPointType> (OHDP::SpaceNodeID(src.space(), src.node()), src.port()),
            EndPoint<EndPointType> (OHDP::SpaceNodeID(dst.space(), dst.node()), dst.port()),
            (void*) payload.data(), payload.size()
        );
    }

    void receiveMessageToCallback(const OHDP::Endpoint &src, const OHDP::Endpoint &dst, MemoryReference payload, DataCallback cb) {
      cb((void*) payload.data(), payload.size() );
    }



    const Context* mContext;
    OHDP::Service* mOHDP;

    typedef std::map<EndPoint<EndPointType>, OHDP::Port*> PortMap;
    PortMap mAllocatedPorts;

    boost::mutex mMutex;

    ConnectionVariables<EndPointType>* mSSTConnVars;
    EndPointType mEndpoint;
};

#if SIRIKATA_PLATFORM == SIRIKATA_PLATFORM_WINDOWS
  // These exports keep Windows happy by forcing the export of these
  // types. BaseDatagramLayer is now excluded because it is explicitly
  // specialized, which, for some reason, keeps things working
  // properly.
  //SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT BaseDatagramLayer<OHDP::SpaceNodeID>;
  SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Connection<OHDP::SpaceNodeID>;
  SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Stream<OHDP::SpaceNodeID>;
#endif

} // namespace SST

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_OHDP_SST_HPP_
