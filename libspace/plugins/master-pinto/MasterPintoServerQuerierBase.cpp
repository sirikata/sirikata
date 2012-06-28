// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "MasterPintoServerQuerierBase.hpp"

#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/options/Options.hpp>

#include "Protocol_MasterPinto.pbj.hpp"

using namespace Sirikata::Network;

#define MP_LOG(lvl,msg) SILOG(masterpinto,lvl, msg);

namespace Sirikata {

MasterPintoServerQuerierBase::MasterPintoServerQuerierBase(SpaceContext* ctx, const String& params)
 : mContext(ctx),
   mIOStrand(ctx->ioService->createStrand("MasterPintoServerQuerier")),
   mConnecting(false),
   mConnected(false),
   mGaveID(false),
   mRegion(),
   mRegionDirty(true),
   mMaxRadius(0),
   mMaxRadiusDirty(true)
{
    OptionSet* optionsSet = OptionSet::getOptions("space_master_pinto",NULL);
    optionsSet->parse(params);

    mHost = optionsSet->referenceOption(OPT_MASTER_PINTO_HOST)->as<String>();
    mPort = optionsSet->referenceOption(OPT_MASTER_PINTO_PORT)->as<String>();
}

MasterPintoServerQuerierBase::~MasterPintoServerQuerierBase() {
    delete mServerStream;
    delete mIOStrand;
}

void MasterPintoServerQuerierBase::start() {
    connect();
}

void MasterPintoServerQuerierBase::stop() {
    mServerStream->close();
    mConnected = false;
}

void MasterPintoServerQuerierBase::connect() {
    if (mConnecting) return;

    mConnecting = true;

    OptionSet* optionsSet = OptionSet::getOptions("space_master_pinto",NULL);
    String server_protocol = optionsSet->referenceOption(OPT_MASTER_PINTO_PROTOCOL)->as<String>();
    String server_protocol_options = optionsSet->referenceOption(OPT_MASTER_PINTO_PROTOCOL_OPTIONS)->as<String>();
    OptionSet* server_protocol_optionset = StreamFactory::getSingleton().getOptionParser(server_protocol)(server_protocol_options);
    mServerStream = StreamFactory::getSingleton().getConstructor(server_protocol)(mIOStrand, server_protocol_optionset);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    Address serverAddress(mHost, mPort);
    MP_LOG(debug, "Trying to connect to " << serverAddress.toString());
    mServerStream->connect(
        serverAddress,
        &Network::Stream::ignoreSubstreamCallback,
        std::tr1::bind(&MasterPintoServerQuerierBase::handleServerConnection, this, _1, _2),
        std::tr1::bind(&MasterPintoServerQuerierBase::handleServerReceived, this, _1, _2),
        std::tr1::bind(&MasterPintoServerQuerierBase::handleServerReadySend, this)
    );
}

void MasterPintoServerQuerierBase::updateRegion(const BoundingBox3f& region) {
    MP_LOG(debug, "Updating region " << region);
    mRegion = region;
    mRegionDirty = true;
    mIOStrand->post(
        std::tr1::bind(&MasterPintoServerQuerierBase::tryServerUpdate, this),
        "MasterPintoServerQuerierBase::tryServerUpdate"
    );
}

void MasterPintoServerQuerierBase::updateLargestObject(float max_radius) {
    MP_LOG(debug, "Updating largest object " << max_radius);
    mMaxRadius = max_radius;
    mMaxRadiusDirty = true;
    mIOStrand->post(
        std::tr1::bind(&MasterPintoServerQuerierBase::tryServerUpdate, this),
        "MasterPintoServerQuerierBase::tryServerUpdate"
    );
}

void MasterPintoServerQuerierBase::tryServerUpdate() {
    if (!mRegionDirty && !mMaxRadiusDirty)
        return;

    if (!mConnected) {
        connect();
        return;
    }

    Sirikata::Protocol::MasterPinto::PintoMessage msg;

    if (!mGaveID) {
        mGaveID = true;
        Sirikata::Protocol::MasterPinto::IRegionIDUpdate update = msg.mutable_server();
        update.set_server(mContext->id());
    }

    if (mRegionDirty) {
        mRegionDirty = false;
        Sirikata::Protocol::MasterPinto::IRegionUpdate update = msg.mutable_region();
        update.set_bounds(mRegion.toBoundingSphere());
    }

    if (mMaxRadiusDirty) {
        mMaxRadiusDirty = false;
        Sirikata::Protocol::MasterPinto::ILargestObjectUpdate update = msg.mutable_largest();
        update.set_radius(mMaxRadius);
    }

    String serialized = serializePBJMessage(msg);
    mServerStream->send( MemoryReference(serialized), ReliableOrdered );
}

void MasterPintoServerQuerierBase::sendQueryUpdate(const String& data, QueueUpdateType queue) {
    if (!connected()) {
        if (queue == QueueUpdate)
            mQueuedQueryUpdates.push_back(data);
        connect();
        return;
    }

    Sirikata::Protocol::MasterPinto::PintoMessage msg;
    msg.set_query(data);
    String serialized = serializePBJMessage(msg);
    mServerStream->send( MemoryReference(serialized), ReliableOrdered );
}

void MasterPintoServerQuerierBase::handleServerConnection(Network::Stream::ConnectionStatus status, const std::string &reason) {
    mConnecting = false;

    if (status == Network::Stream::Connected) {
        MP_LOG(debug, "Connected to master pinto server.");
        mConnected = true;
        tryServerUpdate();
        // Process any queued messages
        std::vector<String> queued_updates;
        mQueuedQueryUpdates.swap(queued_updates);
        for(uint32 i = 0; i < queued_updates.size(); i++)
            sendQueryUpdate(queued_updates[i]);
        // Allow implementations to do some work with the connection
        onConnected();
    }
    else if (status == Network::Stream::ConnectionFailed) {
        MP_LOG(debug, "Connection to master pinto server failed.");
    }
    else if (status == Network::Stream::Disconnected) {
        MP_LOG(debug, "Disconnected from pinto server.");
        mConnected = false;
        mGaveID = false;
        // In case the implementation needs to clean up
        onDisconnected();
    }
}

void MasterPintoServerQuerierBase::handleServerReceived(Chunk& data, const Network::Stream::PauseReceiveCallback& pause) {
    onPintoData(String((char*)&data[0], data.size()));
}

void MasterPintoServerQuerierBase::handleServerReadySend() {
}

} // namespace Sirikata
