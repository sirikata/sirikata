// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "PintoManager.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_MasterPinto.pbj.hpp"

#include <sirikata/core/command/Commander.hpp>
#include <boost/lexical_cast.hpp>

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
    String handler_node_data = GetOptionValue<String>(OPT_PINTO_HANDLER_NODE_DATA);
    mQueryHandler = ServerProxGeomQueryHandlerFactory.getConstructor(handler_type, handler_node_data)(handler_options, true);
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

    tick();
}

void PintoManager::onRegionUpdate(Sirikata::Network::Stream* stream, BoundingSphere3f bounds) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->region(bounds);

    tick();
    // Note that we don't have this trigger loc updates. This isn't necessary
    // since clients of this version of pinto just trust that they should
    // contact the returned servers.
}

void PintoManager::onMaxSizeUpdate(Sirikata::Network::Stream* stream, float32 ms) {
    assert(mClients.find(stream) != mClients.end());
    ClientData& cdata = mClients[stream];
    cdata.query->maxSize(ms);

    tick();
    // Note that we don't have this trigger loc updates. This isn't necessary
    // since clients of this version of pinto just trust that they should
    // contact the returned servers.
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

    tick();
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
        // This doesn't support reparenting, we'd need to update this
        // protocol to look more like the normal Prox result protocol
        // (or just use that protocol...)
        assert(evt.reparents().size() == 0);
        for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
            Sirikata::Protocol::MasterPinto::IPintoResult result = update.add_change();
            result.set_addition(false);
            result.set_server(evt.removals()[ridx].id());
        }

        evts.pop_front();
    }

    String serialized = serializePBJMessage(msg);
    bool success = stream->send( MemoryReference(serialized), ReliableOrdered );
    assert(success);
}




// BaseProxCommandable
void PintoManager::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    result.put("name", "pinto-manager");
    result.put("settings.handlers", 1);

    result.put("servers.properties.count", mClients.size());
    result.put("queries.servers.count", mClients.size());

    cmdr->result(cmdid, result);
}

void PintoManager::commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    String key = String("handlers.servers.servers.");
    result.put(key + "name", String("server-queries"));
    result.put(key + "queries", mQueryHandler->numQueries());
    result.put(key + "objects", mQueryHandler->numObjects());
    result.put(key + "nodes", mQueryHandler->numNodes());
    cmdr->result(cmdid, result);
}

void PintoManager::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    result.put( String("nodes"), Command::Array());
    Command::Array& nodes_ary = result.getArray("nodes");
    for(ProxQueryHandler::NodeIterator nit = mQueryHandler->nodesBegin(); nit != mQueryHandler->nodesEnd(); nit++) {
        nodes_ary.push_back( Command::Object() );
        nodes_ary.back().put("id", boost::lexical_cast<String>(nit.id()));
        nodes_ary.back().put("parent", boost::lexical_cast<String>(nit.parentId()));
        BoundingSphere3f bounds = nit.bounds(mContext->simTime());
        nodes_ary.back().put("bounds.center.x", bounds.center().x);
        nodes_ary.back().put("bounds.center.y", bounds.center().y);
        nodes_ary.back().put("bounds.center.z", bounds.center().z);
        nodes_ary.back().put("bounds.radius", bounds.radius());
        nodes_ary.back().put("cuts", nit.cuts());
    }

    cmdr->result(cmdid, result);
}


void PintoManager::commandListQueriers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    // Organized as lists under queriers.type, each querier being a dict of query handlers -> stats
    result.put("queriers.server", Command::Object());
    Command::Result& queriers = result.get("queriers.server");
    for(ClientDataMap::const_iterator it = mClients.begin(); it != mClients.end(); it++) {
        // If a node hasn't associated a Server ID yet, ignore it's query
        ServerID sid = streamServerID(it->first);
        if (sid == NullServerID) continue;

        Command::Result data = Command::EmptyResult();
        data.put("server-queries.results", it->second.query->numResults());
        data.put("server-queries.size", it->second.query->size());
        queriers.put(boost::lexical_cast<String>(sid), data);
    }
    cmdr->result(cmdid, result);
}

void PintoManager::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    mQueryHandler->rebuild();
    cmdr->result(cmdid, result);
}

void PintoManager::commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put( String("stats"), Command::EmptyResult());
    cmdr->result(cmdid, result);
}

} // namespace Sirikata
