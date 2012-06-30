// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "PintoManagerBase.hpp"
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_MasterPinto.pbj.hpp"

#include <sirikata/core/command/Commander.hpp>

using namespace Sirikata::Network;

#define PINTO_LOG(lvl, msg) SILOG(pinto,lvl,msg)

namespace Sirikata {

PintoManagerBase::PintoManagerBase(PintoContext* ctx)
 : Pinto::BaseProxCommandable(),
   mContext(ctx),
   mStrand(ctx->ioService->createStrand("PintoManager"))
{
    registerBaseProxCommands(ctx, "pinto.prox");

    String listener_protocol = GetOptionValue<String>(OPT_PINTO_PROTOCOL);
    String listener_protocol_options = GetOptionValue<String>(OPT_PINTO_PROTOCOL_OPTIONS);

    OptionSet* listener_protocol_optionset = StreamListenerFactory::getSingleton().getOptionParser(listener_protocol)(listener_protocol_options);

    mListener = StreamListenerFactory::getSingleton().getConstructor(listener_protocol)(mStrand, listener_protocol_optionset);

    String listener_host = GetOptionValue<String>(OPT_PINTO_HOST);
    String listener_port = GetOptionValue<String>(OPT_PINTO_PORT);
    Address listenAddress(listener_host, listener_port);
    PINTO_LOG(debug, "Listening on " << listenAddress.toString());
    mListener->listen(
        listenAddress,
        std::tr1::bind(&PintoManagerBase::newStreamCallback,this,_1,_2)
    );

    mLocCache = new PintoManagerLocationServiceCache();
}

PintoManagerBase::~PintoManagerBase() {
    delete mListener;
    delete mStrand;
}

void PintoManagerBase::start() {
}

void PintoManagerBase::stop() {
    mListener->close();
}

void PintoManagerBase::newStreamCallback(Stream* newStream, Stream::SetCallbacks& setCallbacks) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    PINTO_LOG(debug,"New space server connection.");

    setCallbacks(
        std::tr1::bind(&PintoManagerBase::handleClientConnection, this, newStream, _1, _2),
        std::tr1::bind(&PintoManagerBase::handleClientReceived, this, newStream, _1, _2),
        std::tr1::bind(&PintoManagerBase::handleClientReadySend, this, newStream)
    );

    onConnected(newStream);
}

void PintoManagerBase::handleClientConnection(Sirikata::Network::Stream* stream, Network::Stream::ConnectionStatus status, const std::string &reason) {
    if (status == Network::Stream::Disconnected) {
        // Unregister region and query, remove from clients list.
        ServerID sid = streamServerID(stream);
        if (sid != NullServerID)
            mLocCache->removeSpaceServer(sid);
        onDisconnected(stream);
    }
}

void PintoManagerBase::handleClientReceived(Sirikata::Network::Stream* stream, Chunk& data, const Network::Stream::PauseReceiveCallback& pause) {
    Sirikata::Protocol::MasterPinto::PintoMessage msg;
    bool parsed = parsePBJMessage(&msg, data);

    if (!parsed) {
        PINTO_LOG(error, "Couldn't parse message from client.");
        return;
    }

    if (msg.has_server()) {
        PINTO_LOG(debug, "Associated connection with space server " << msg.server().server());
        mStreamServers[stream] = msg.server().server();

        TimedMotionVector3f default_loc( Time::null(), MotionVector3f( Vector3f::zero(), Vector3f::zero() ) );
        BoundingSphere3f default_region(BoundingSphere3f::null());
        float32 default_max = 0.f;
        mLocCache->addSpaceServer(msg.server().server(), default_loc, default_region, default_max);
        onInitialMessage(stream);
    }

    ServerID server = streamServerID(stream);
    if (server == NullServerID) {
        PINTO_LOG(error, "Received message without ServerID and don't already have one, killing the connection.");
        stream->close();
        onDisconnected(stream);
        return;
    }

    if (msg.has_region()) {
        PINTO_LOG(debug, "Received region update from " << server << ": " << msg.region().bounds());
        mLocCache->updateSpaceServerRegion(server, msg.region().bounds());
        onRegionUpdate(stream, msg.region().bounds());
    }

    if (msg.has_largest()) {
        PINTO_LOG(debug, "Received largest object update from " << server << ": " << msg.largest().radius());
        mLocCache->updateSpaceServerMaxSize(server, msg.largest().radius());
        onMaxSizeUpdate(stream, msg.largest().radius());
    }

    if (msg.has_query()) {
        PINTO_LOG(debug, "Received query update from " << server);
        onQueryUpdate(stream, msg.query());
    }
}

void PintoManagerBase::handleClientReadySend(Sirikata::Network::Stream* stream) {
}



// Dummy implementations for events
void PintoManagerBase::onConnected(Stream* newStream) {
}

void PintoManagerBase::onInitialMessage(Stream* stream) {
}

void PintoManagerBase::onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds) {
}

void PintoManagerBase::onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms) {
}

void PintoManagerBase::onQueryUpdate(Stream* stream, const String& update) {
}

void PintoManagerBase::onDisconnected(Stream* stream) {
}




// AggregateListener Interface
void PintoManagerBase::aggregateCreated(ProxAggregator* handler, const ServerID& objid) {
    mLocCache->addAggregate(objid);
}

void PintoManagerBase::aggregateChildAdded(ProxAggregator* handler, const ServerID& objid, const ServerID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    mLocCache->updateAggregateLocation(objid, TimedMotionVector3f(Time::null(), MotionVector3f(bnds_center, Vector3f(0,0,0))));
    mLocCache->updateAggregateBounds(objid, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void PintoManagerBase::aggregateChildRemoved(ProxAggregator* handler, const ServerID& objid, const ServerID& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    mLocCache->updateAggregateLocation(objid, TimedMotionVector3f(Time::null(), MotionVector3f(bnds_center, Vector3f(0,0,0))));
    mLocCache->updateAggregateBounds(objid, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void PintoManagerBase::aggregateBoundsUpdated(ProxAggregator* handler, const ServerID& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    mLocCache->updateAggregateLocation(objid, TimedMotionVector3f(Time::null(), MotionVector3f(bnds_center, Vector3f(0,0,0))));
    mLocCache->updateAggregateBounds(objid, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void PintoManagerBase::aggregateDestroyed(ProxAggregator* handler, const ServerID& objid) {
    mLocCache->removeAggregate(objid);
}

void PintoManagerBase::aggregateObserved(ProxAggregator* handler, const ServerID& objid, uint32 nobservers) {
}


ServerID PintoManagerBase::streamServerID(Sirikata::Network::Stream* stream) const {
    StreamServerIDMap::const_iterator it = mStreamServers.find(stream);
    if (it == mStreamServers.end()) return NullServerID;
    return it->second;
}

} // namespace Sirikata
