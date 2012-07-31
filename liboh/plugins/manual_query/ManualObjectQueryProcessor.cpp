// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "ManualObjectQueryProcessor.hpp"
#include <sirikata/core/network/IOStrandImpl.hpp>

#define QPLOG(lvl, msg) SILOG(manual-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Manual {


ManualObjectQueryProcessor* ManualObjectQueryProcessor::create(ObjectHostContext* ctx, const String& args) {
    return new ManualObjectQueryProcessor(ctx);
}


ManualObjectQueryProcessor::ManualObjectQueryProcessor(ObjectHostContext* ctx)
 : ObjectQueryProcessor(ctx),
   mContext(ctx),
   mStrand(mContext->ioService->createStrand("ManualObjectQueryProcessor Strand")),
   mServerQueryHandler(ctx, this, mStrand)
{
}

ManualObjectQueryProcessor::~ManualObjectQueryProcessor() {
}

void ManualObjectQueryProcessor::start() {
    mServerQueryHandler.start();
    mContext->objectHost->ObjectNodeSessionProvider::addListener(static_cast<ObjectNodeSessionListener*>(this));
}

void ManualObjectQueryProcessor::stop() {
    mServerQueryHandler.stop();
    mContext->objectHost->ObjectNodeSessionProvider::removeListener(static_cast<ObjectNodeSessionListener*>(this));

    for(QueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++)
        it->second->stop();
    mObjectQueryHandlers.clear();

    mObjectState.clear();
}


void ManualObjectQueryProcessor::onObjectNodeSession(const SpaceID& space, const ObjectReference& oref, const OHDP::NodeID& id) {
    QPLOG(detailed, "New object-space node session: " << oref << " connected to " << space << "-" << id);

    OHDP::SpaceNodeID snid(space, id);
    SpaceObjectReference sporef(space, oref);

    ObjectStateMap::iterator obj_it = mObjectState.find(sporef);

    // First, clear out old state. This needs to happen before any changes are
    // made so it can clear out all the old state
    if (obj_it != mObjectState.end() && obj_it->second.node != OHDP::NodeID::null()) {
        OHDP::SpaceNodeID old_snid(obj_it->first.space(), obj_it->second.node);
        mServerQueryHandler.decrementServerQuery(old_snid);
        if (obj_it->second.registered)
            unregisterObjectQuery(sporef);
    }

    // Update state
    // Object no longer has a session, it's no longer relevant
    if (id == OHDP::NodeID::null()) {
        if (obj_it != mObjectState.end()) mObjectState.erase(obj_it);
        return;
    }
    // Otherwise it's new/migrating
    if (obj_it == mObjectState.end())
        obj_it = mObjectState.insert( ObjectStateMap::value_type(sporef, ObjectState()) ).first;
    obj_it->second.node = id;

    // Figure out if we need to do anything for registration. At a minimum,
    // hold onto the server query handler.
    mServerQueryHandler.incrementServerQuery(snid);
    // *Don't* try to register. Instead, we need to wait until we're really
    // fully connected, i.e. until we get an SST stream callback
}



void ManualObjectQueryProcessor::presenceConnected(HostedObjectPtr ho, const SpaceObjectReference& sporef) {
}

void ManualObjectQueryProcessor::presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, SSTStreamPtr strm) {
    // And possibly register the query
    ObjectStateMap::iterator obj_it = mObjectState.find(sporef);
    if (obj_it == mObjectState.end())
        return;

    if (obj_it->second.needsRegistration())
        registerOrUpdateObjectQuery(sporef);
}

void ManualObjectQueryProcessor::presenceDisconnected(HostedObjectPtr ho, const SpaceObjectReference& sporef) {
}

String ManualObjectQueryProcessor::connectRequest(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& query) {
    if (!query.empty()) {
        ObjectStateMap::iterator obj_it = mObjectState.find(sporef);
        // Very likely brand new here
        if (obj_it == mObjectState.end())
            obj_it = mObjectState.insert( ObjectStateMap::value_type(sporef, ObjectState()) ).first;
        obj_it->second.who = ho;
        obj_it->second.query = query;
    }

    // Return an empty query -- we don't want any query passed on to the
    // serve. We aggregate and register the OH query separately.
    return "";
}

void ManualObjectQueryProcessor::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) {
    ObjectStateMap::iterator it = mObjectState.find(sporef);

    if (new_query.empty()) { // Cancellation
        // Fill in the new query, we may still keep this around so we
        // want an accurate record.
        it->second.query = new_query;
        // Clear object state if possible
        if (it->second.registered)
            unregisterObjectQuery(sporef);
        if (it->second.canRemove())
            mObjectState.erase(it);
    }
    else { // Init or update
        // Track state
        it->second.who = ho;
        it->second.query = new_query;
        // (New query and can register) or (already registered, needs
        // update). If we have a new query but can't yet register, it'll be
        // checked when the connection succeeds
        if ( (it->second.needsRegistration() && it->second.canRegister()) || (it->second.registered) )
            registerOrUpdateObjectQuery(sporef);
    }
}




void ManualObjectQueryProcessor::createdServerQuery(const OHDP::SpaceNodeID& snid) {
    // We get this when the first object session with a space server starts. We
    // can use this to setup the query processor for objects connected to that
    // space server.
    ObjectQueryHandlerPtr obj_query_handler(
        new ObjectQueryHandler(mContext, this, snid, mStrand)
    );
    mObjectQueryHandlers.insert(
        QueryHandlerMap::value_type(
            snid,
            obj_query_handler
        )
    );
    obj_query_handler->start();
}

void ManualObjectQueryProcessor::removedServerQuery(const OHDP::SpaceNodeID& snid) {
    QueryHandlerMap::iterator it = mObjectQueryHandlers.find(snid);
    assert( it != mObjectQueryHandlers.end() );
    ObjectQueryHandlerPtr obj_query_handler = it->second;
    mObjectQueryHandlers.erase(it);
    obj_query_handler->stop();
}

void ManualObjectQueryProcessor::createdReplicatedIndex(const OHDP::SpaceNodeID& snid, ProxIndexID iid, ReplicatedLocationServiceCachePtr loc_cache, ServerID objects_from_server, bool dynamic_objects) {
    QueryHandlerMap::iterator it = mObjectQueryHandlers.find(snid);
    assert( it != mObjectQueryHandlers.end() );
    it->second->createdReplicatedIndex(iid, loc_cache, objects_from_server, dynamic_objects);
}

void ManualObjectQueryProcessor::removedReplicatedIndex(const OHDP::SpaceNodeID& snid, ProxIndexID iid) {
    QueryHandlerMap::iterator it = mObjectQueryHandlers.find(snid);
    assert( it != mObjectQueryHandlers.end() );
    it->second->removedReplicatedIndex(iid);
}



void ManualObjectQueryProcessor::queriersAreObserving(const OHDP::SpaceNodeID& snid, const ProxIndexID indexid, const ObjectReference& objid) {
    mServerQueryHandler.queriersAreObserving(snid, indexid, objid);
}

void ManualObjectQueryProcessor::queriersStoppedObserving(const OHDP::SpaceNodeID& snid, const ProxIndexID indexid, const ObjectReference& objid) {
    mServerQueryHandler.queriersStoppedObserving(snid, indexid, objid);
}

void ManualObjectQueryProcessor::replicatedNodeRemoved(const OHDP::SpaceNodeID& snid, ProxIndexID indexid, const ObjectReference& objid) {
    mServerQueryHandler.replicatedNodeRemoved(snid, indexid, objid);
}




void ManualObjectQueryProcessor::registerOrUpdateObjectQuery(const SpaceObjectReference& sporef) {
    // Get query info
    ObjectStateMap::iterator it = mObjectState.find(sporef);
    assert(it != mObjectState.end());
    ObjectState& state = it->second;
    HostedObjectPtr ho = state.who.lock();
    assert( ho && !state.query.empty() && state.node != OHDP::NodeID::null() );

    // Get the appropriate handler
    QueryHandlerMap::iterator handler_it = mObjectQueryHandlers.find(OHDP::SpaceNodeID(sporef.space(), state.node));
    assert(handler_it != mObjectQueryHandlers.end());
    ObjectQueryHandlerPtr handler = handler_it->second;

    // And register
    handler->updateQuery(ho, sporef, state.query);
    state.registered = true;
}

void ManualObjectQueryProcessor::unregisterObjectQuery(const SpaceObjectReference& sporef) {
    // Get query info
    ObjectStateMap::iterator it = mObjectState.find(sporef);
    assert(it != mObjectState.end());
    ObjectState& state = it->second;
    HostedObjectPtr ho = state.who.lock();
    // unregisterObjectQuery can happen due to disconnection where we
    // want to preserve the query info, so we can't assert an empty
    // query like we assert a non-empty one in registerOrUpdateObjectQuery
    assert( ho && state.node != OHDP::NodeID::null() );

    // Get the appropriate handler
    QueryHandlerMap::iterator handler_it = mObjectQueryHandlers.find(OHDP::SpaceNodeID(sporef.space(), state.node));
    assert(handler_it != mObjectQueryHandlers.end());
    ObjectQueryHandlerPtr handler = handler_it->second;

    // And unregister
    handler->removeQuery(ho, sporef);
    state.registered = false;
}


void ManualObjectQueryProcessor::deliverProximityResult(const SpaceObjectReference& sporef, const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    ObjectStateMap::iterator it = mObjectState.find(sporef);
    // Object may no longer exist since events cross strands
    if (it == mObjectState.end()) return;

    HostedObjectPtr ho = it->second.who.lock();
    if (!ho) return;

    // Should already be in local time, so we can deliver directly
    deliverProximityUpdate(ho, sporef, update);
}

void ManualObjectQueryProcessor::deliverLocationResult(const SpaceObjectReference& sporef, const LocUpdate& lu) {
    ObjectStateMap::iterator it = mObjectState.find(sporef);
    // Object may no longer exist since events cross strands
    if (it == mObjectState.end()) return;

    HostedObjectPtr ho = it->second.who.lock();
    if (!ho) return;

    deliverLocationUpdate(ho, sporef, lu);
}



void ManualObjectQueryProcessor::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put("name", "libprox-manual");
    result.put("settings.handlers", mObjectQueryHandlers.size());
    cmdr->result(cmdid, result);
}

void ManualObjectQueryProcessor::commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    for (QueryHandlerMap::iterator it = mObjectQueryHandlers.begin(); it != mObjectQueryHandlers.end(); it++) {
        ObjectQueryHandlerPtr hdlr = it->second;
        hdlr->commandListInfo(it->first, result);
    }

    cmdr->result(cmdid, result);
}

ManualObjectQueryProcessor::ObjectQueryHandlerPtr ManualObjectQueryProcessor::lookupCommandHandler(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) const {
    ObjectQueryHandlerPtr result;

    if (cmd.contains("handler")) {
        String handler_name = cmd.getString("handler");
        // Only the first part of the handler name is the SpaceNodeID we need to
        // look up the ObjectQueryHandler
        handler_name = handler_name.substr(0, handler_name.find('.'));
        OHDP::SpaceNodeID snid(handler_name);
        QueryHandlerMap::const_iterator it = mObjectQueryHandlers.find(snid);
        if (it != mObjectQueryHandlers.end())
            result = it->second;
    }

    if (!result) {
        Command::Result result = Command::EmptyResult();
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
    }

    return result;
}

void ManualObjectQueryProcessor::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    // Look up or trigger error message
    ObjectQueryHandlerPtr handler = lookupCommandHandler(cmd, cmdr, cmdid);
    if (!handler) return;

    handler->commandListNodes(cmd, cmdr, cmdid);
}

void ManualObjectQueryProcessor::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    // Look up or trigger error message
    ObjectQueryHandlerPtr handler = lookupCommandHandler(cmd, cmdr, cmdid);
    if (!handler) return;

    handler->commandForceRebuild(cmd, cmdr, cmdid);
}

} // namespace Manual
} // namespace OH
} // namespace Sirikata
