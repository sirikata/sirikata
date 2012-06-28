// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "PintoManager.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_MasterPinto.pbj.hpp"

#include <sirikata/core/prox/QueryHandlerFactory.hpp>

using namespace Sirikata::Network;

#define PINTO_LOG(lvl, msg) SILOG(pinto,lvl,msg)

namespace Sirikata {

PintoManager::PintoManager(PintoContext* ctx)
 : PintoManagerBase(ctx),
   mLastTime(Time::null()),
   mDt(Duration::milliseconds((int64)1))
{
    String handler_type = GetOptionValue<String>(OPT_PINTO_HANDLER_TYPE);
    String handler_options = GetOptionValue<String>(OPT_PINTO_HANDLER_OPTIONS);
    mQueryHandler = QueryHandlerFactory<ServerProxSimulationTraits>(handler_type, handler_options);
    bool static_objects = false;
    mQueryHandler->initialize(
        mLocCache, mLocCache,
        static_objects, false /* not replicated */
    );
}

PintoManager::~PintoManager() {
    delete mQueryHandler;
}

void PintoManager::onConnected(Stream* newStream) {
    mClients[newStream] = ClientData();
}

void PintoManager::onInitialMessage(Stream* stream) {
    TimedMotionVector3f default_loc( Time::null(), MotionVector3f( Vector3f::zero(), Vector3f::zero() ) );
    BoundingSphere3f default_region(BoundingSphere3f::null());
    float32 default_max = 0.f;
    SolidAngle default_min_angle(SolidAngle::Max);
    // FIXME max_results
    Query* query = mQueryHandler->registerQuery(default_loc, default_region, default_max, default_min_angle);
    ClientData& cdata = mClients[stream];
    cdata.query = query;
    mClientsByQuery[query] = stream;
    query->setEventListener(this);
}

void PintoManager::onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->region(bounds);
}

void PintoManager::onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->maxSize(ms);
}

void PintoManager::onQueryUpdate(Stream* stream, const String& data) {
    Sirikata::Protocol::MasterPinto::QueryUpdate update;
    bool parsed = parsePBJMessage(&update, data);

    if (!parsed) {
        PINTO_LOG(error, "Couldn't parse solid angle query update from client.");
        return;
    }

    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->angle( SolidAngle(update.min_angle()) );
    // FIXME max results
}

void PintoManager::onDisconnected(Stream* stream) {
    mClientsByQuery.erase( mClients[stream].query );
    delete mClients[stream].query;
    mClients.erase(stream);

    tick();
}

void PintoManager::tick() {
    mLastTime += mDt;
    mQueryHandler->tick(mLastTime);
}

void PintoManager::queryHasEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    Network::Stream* stream = mClientsByQuery[query];
    ClientData& cdata = mClients[stream];

    QueryEventList evts;
    query->popEvents(evts);

    Sirikata::Protocol::MasterPinto::PintoResponse msg;

    while(!evts.empty()) {
        const QueryEvent& evt = evts.front();
        //PINTO_LOG(debug, "Event generated for server " << cdata.server << ": " << evt.id() << (evt.type() == QueryEvent::Added ? " added" : " removed"));

        Sirikata::Protocol::MasterPinto::IPintoUpdate update = msg.add_update();
        for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
            Sirikata::Protocol::MasterPinto::IPintoResult result = update.add_change();
            result.set_addition(true);
            result.set_server(evt.additions()[aidx].id());
        }
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            Sirikata::Protocol::MasterPinto::IPintoResult result = update.add_change();
            result.set_addition(false);
            result.set_server(evt.removals()[ridx].id());
        }

        evts.pop_front();
    }

    String serialized = serializePBJMessage(msg);
    stream->send( MemoryReference(serialized), ReliableOrdered );
}

} // namespace Sirikata
