// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#ifndef _SIRIKATA_LIBCORE_ODP_SST_HPP_
#define _SIRIKATA_LIBCORE_ODP_SST_HPP_

#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/odp/Service.hpp>

namespace Sirikata {

// Convenience typedefs in a separate namespace
namespace ODPSST {
typedef Sirikata::SST::EndPoint<SpaceObjectReference> Endpoint;
typedef Sirikata::SST::BaseDatagramLayer<SpaceObjectReference> BaseDatagramLayer;
typedef Sirikata::SST::Connection<SpaceObjectReference> Connection;
typedef Sirikata::SST::Stream<SpaceObjectReference> Stream;
typedef Sirikata::SST::ConnectionManager<SpaceObjectReference> ConnectionManager;
} // namespace ODPSST

// SpaceObjectReference/ODP-specific implementation
namespace SST {

template <>
class SIRIKATA_EXPORT BaseDatagramLayer<SpaceObjectReference>
{
  private:
    typedef SpaceObjectReference EndPointType;

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
        ODP::Service* odp)
    {
        BaseDatagramLayerPtr datagramLayer = getDatagramLayer(sstConnVars, endPoint);
        if (datagramLayer) return datagramLayer;

        datagramLayer = BaseDatagramLayerPtr(
            new BaseDatagramLayer(sstConnVars, ctx, odp, endPoint)
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
        ODP::Port* port = allocatePort(listeningEndPoint);
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

    void unlisten(EndPoint<EndPointType>& ep) {
        // To stop listening, just destroy the corresponding port
        PortMap::iterator it = mAllocatedPorts.find(ep);
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

    void invalidate() {
        mODP = NULL;
        mSSTConnVars->removeDatagramLayer(mEndpoint, true);
    }

  private:
    BaseDatagramLayer(ConnectionVariables<EndPointType>* sstConnVars, const Context* ctx, ODP::Service* odpservice, const EndPointType&ep)
        : mContext(ctx),
          mODP(odpservice),
          mSSTConnVars(sstConnVars),
          mEndpoint(ep)
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
        PortMap::iterator it = mAllocatedPorts.find(ep);
        if (it == mAllocatedPorts.end()) return NULL;
        return it->second;
    }

    ODP::Port* getOrAllocatePort(const EndPoint<EndPointType>& ep) {
        ODP::Port* result = getPort(ep);
        if (result != NULL) return result;
        result = allocatePort(ep);
        return result;
    }

    void receiveMessage(const ODP::Endpoint &src, const ODP::Endpoint &dst, MemoryReference payload) {
        Connection<EndPointType>::handleReceive(
            mSSTConnVars,
            EndPoint<EndPointType> (SpaceObjectReference(src.space(), src.object()), src.port()),
            EndPoint<EndPointType> (SpaceObjectReference(dst.space(), dst.object()), dst.port()),
            (void*) payload.data(), payload.size()
        );
    }

    void receiveMessageToCallback(const ODP::Endpoint &src, const ODP::Endpoint &dst, MemoryReference payload, DataCallback cb) {
        cb((void*) payload.data(), payload.size() );
    }



    const Context* mContext;
    ODP::Service* mODP;

    typedef std::map<EndPoint<EndPointType>, ODP::Port*> PortMap;
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
  //SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT BaseDatagramLayer<SpaceObjectReference>;
  SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Connection<SpaceObjectReference>;
  SIRIKATA_EXPORT_TEMPLATE template class SIRIKATA_EXPORT Stream<SpaceObjectReference>;
#endif

} // namespace SST

} // namespace Sirikata

#endif //_SIRIKATA_LIBCORE_ODP_SST_HPP_
