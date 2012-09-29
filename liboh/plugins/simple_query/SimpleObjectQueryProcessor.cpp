// Copyright (c) 2011 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "SimpleObjectQueryProcessor.hpp"
#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Frame.pbj.hpp"
#include <sirikata/pintoloc/ProtocolLocUpdate.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/oh/OHSpaceTimeSynced.hpp>
#include <sirikata/core/odp/SST.hpp>

#define SOQP_LOG(lvl, msg) SILOG(simple-object-query-processor, lvl, msg)

namespace Sirikata {
namespace OH {
namespace Simple {

SimpleObjectQueryProcessor* SimpleObjectQueryProcessor::create(ObjectHostContext* ctx, const String& args) {
    return new SimpleObjectQueryProcessor(ctx);
}


SimpleObjectQueryProcessor::SimpleObjectQueryProcessor(ObjectHostContext* ctx)
 : ObjectQueryProcessor(ctx),
   mContext(ctx)
{
}

SimpleObjectQueryProcessor::~SimpleObjectQueryProcessor() {
}

void SimpleObjectQueryProcessor::start() {
}

void SimpleObjectQueryProcessor::stop() {
    mContext->mainStrand->post(std::tr1::bind(&SimpleObjectQueryProcessor::handleStop, this));
}


void SimpleObjectQueryProcessor::handleStop() {
    for(ObjectStateMap::iterator it = mObjectStateMap.begin(); it != mObjectStateMap.end(); it++)
        it->second->stop();
}

void SimpleObjectQueryProcessor::presenceConnectedStream(HostedObjectPtr ho, const SpaceObjectReference& sporef, HostedObject::SSTStreamPtr strm) {
    // Setup tracking state for this query
    mObjectStateMap[sporef] = ObjectStatePtr(new ObjectState(mContext, ho));;

    // And setup listeners for new data from the server
    strm->listenSubstream(OBJECT_PORT_PROXIMITY,
        std::tr1::bind(&SimpleObjectQueryProcessor::handleProximitySubstream, this,
            HostedObjectWPtr(ho), sporef, _1, _2
        )
    );

    strm->listenSubstream(OBJECT_PORT_LOCATION,
        std::tr1::bind(&SimpleObjectQueryProcessor::handleLocationSubstream, this,
            HostedObjectWPtr(ho), sporef, _1, _2
        )
    );
}

void SimpleObjectQueryProcessor::presenceDisconnected(HostedObjectPtr ho, const SpaceObjectReference& sporef) {
    mObjectStateMap.erase(sporef);
}

// Proximity

void SimpleObjectQueryProcessor::handleProximitySubstream(const HostedObjectWPtr& weakHO, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    String* prevdata = new String();
    s->registerReadCallback(
        std::tr1::bind(&SimpleObjectQueryProcessor::handleProximitySubstreamRead, this,
            weakHO, spaceobj, s, prevdata, _1, _2
        )
    );
}

void SimpleObjectQueryProcessor::handleProximitySubstreamRead(const HostedObjectWPtr& weakHO, const SpaceObjectReference& spaceobj, SSTStreamPtr s, String* prevdata, uint8* buffer, int length) {
    HostedObjectPtr self(weakHO.lock());
    if (!self)
        return;
    if (self->stopped()) {
        SOQP_LOG(detailed,"Ignoring proximity update after system stop requested.");
        return;
    }

    prevdata->append((const char*)buffer, length);

    while(true) {
        std::string msg = Network::Frame::parse(*prevdata);

        // If we don't have a full message, just wait for more
        if (msg.empty()) return;

        // Otherwise, try to handle it
        handleProximityMessage(self, spaceobj, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //s->registerReadCallback(0);
}

bool SimpleObjectQueryProcessor::handleProximityMessage(HostedObjectPtr self, const SpaceObjectReference& spaceobj, const std::string& payload)
{
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success)
        return false;

    ObjectStatePtr obj_state = mObjectStateMap[spaceobj];

    ProxyManagerPtr proxy_manager = self->getProxyManager(spaceobj.space(), spaceobj.object());
    if (!proxy_manager) {
        SOQP_LOG(warn,"Hosted Object received a message for a presence without a proxy manager.");
        return true;
    }

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        // We need to convert times to local time
        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            update.addition(aidx).location().set_t(
                self->getObjectHost()->localTime(spaceobj.space(), update.addition(aidx).location().t())
            );
        }

        // To take care of tracking orphans for the HostedObject, we need to
        // take take a pass through the results ourselves. We backup data for
        // objects that are going to be removed before delivering the
        // results...
        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

            SpaceObjectReference observed(spaceobj.space(), ObjectReference(removal.object()));
            ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);

            // Somehow, it's possible we don't have a proxy for this object...
            if (proxy_obj)
                obj_state->orphans.addUpdateFromExisting(proxy_obj);
        }

        // Then deliver the results....
        deliverProximityUpdate(self, spaceobj, update);

        // And work through the additions, processing orphaned updates.
        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);

            SpaceObjectReference observed(spaceobj.space(), ObjectReference(addition.object()));

            obj_state->orphans.invokeOrphanUpdates1(observed, this, spaceobj);
        }
    }

    return true;
}


void SimpleObjectQueryProcessor::updateQuery(HostedObjectPtr ho, const SpaceObjectReference& sporef, const String& new_query) {
    Protocol::Prox::QueryRequest request;
    request.set_query_parameters(new_query);
    std::string payload = serializePBJMessage(request);

    SSTStreamPtr spaceStream = mContext->objectHost->getSpaceStream(sporef.space(), sporef.object());
    if (spaceStream != SSTStreamPtr()) {
        SSTConnectionPtr conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram(
            (void*)payload.data(), payload.size(),
            OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY,
            NULL
        );
    }
}


// Location

void SimpleObjectQueryProcessor::handleLocationSubstream(const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s) {
    s->registerReadCallback(
        std::tr1::bind(
            &SimpleObjectQueryProcessor::handleLocationSubstreamRead, this,
            weakSelf, spaceobj, s, new std::stringstream(), _1, _2
        )
    );
}

void SimpleObjectQueryProcessor::handleLocationSubstreamRead(const HostedObjectWPtr& weakSelf, const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length) {
    HostedObjectPtr self(weakSelf.lock());
    if (!self)
        return;
    if (self->stopped()) {
        SOQP_LOG(detailed,"Ignoring location update after system stop requested.");
        return;
    }

    prevdata->write((const char*)buffer, length);
    if (handleLocationMessage(self, spaceobj, prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s, and close the stream
        s->registerReadCallback(0);
        s->close(false);
    }
}

bool SimpleObjectQueryProcessor::handleLocationMessage(const HostedObjectPtr& self, const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    // Each update is checked against the current proximity results (in this
    // implementation's case, that's just the object's ProxyObjects) and
    // either goes into orphan tracking or is delivered.

    // We can sanity check for all updates on this presence, as well as reuse
    // the proxy manager lookup.
    ProxyManagerPtr proxy_manager = self->getProxyManager(spaceobj.space(), spaceobj.object());
    if (!proxy_manager) {
        SOQP_LOG(warn,"Hosted Object received a message for a presence without a proxy manager.");
        return true;
    }
    // As well as looking up object state (orhpan manager) only once
    ObjectStatePtr obj_state = mObjectStateMap[spaceobj];

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);
        SpaceObjectReference observed(spaceobj.space(), ObjectReference(update.object()));
        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);

        OHSpaceTimeSynced sync(mContext->objectHost, spaceobj.space());
        LocProtocolLocUpdate llu(update, sync);
        if (!proxy_obj) {
            obj_state->orphans.addOrphanUpdate(observed, llu);
        }
        else {
            deliverLocationUpdate(self, spaceobj, llu);
        }
    }

    return true;
}

void SimpleObjectQueryProcessor::onOrphanLocUpdate(const LocUpdate& update, const SpaceObjectReference& spaceobj) {
    // This is similar to processing a location message except that we know
    // we're ready for these updates -- the proxy will be there and we
    // definitely never have to add it as an orphan

    ObjectStatePtr obj_state = mObjectStateMap[spaceobj];
    HostedObjectPtr ho = obj_state->ho.lock();
    if (!ho) return;

    ProxyManagerPtr proxy_manager = ho->getProxyManager(spaceobj.space(), spaceobj.object());
    assert(proxy_manager);
    SpaceObjectReference observed(spaceobj.space(), ObjectReference(update.object()));
    ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(observed);
    assert(proxy_obj);
    deliverLocationUpdate(ho, spaceobj, update);
}




// BaseProxCommandable
void SimpleObjectQueryProcessor::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put("name", "simple");
    result.put("settings", Command::Object());
    cmdr->result(cmdid, result);
}
void SimpleObjectQueryProcessor::commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put("handlers", Command::Object());
    cmdr->result(cmdid, result);
}
void SimpleObjectQueryProcessor::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put( String("nodes"), Command::Array());
    cmdr->result(cmdid, result);
}
void SimpleObjectQueryProcessor::commandListQueriers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put( String("queriers"), Command::Array());
    cmdr->result(cmdid, result);
}
void SimpleObjectQueryProcessor::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put("error", "Simple processors have nothing to rebuild.");
    cmdr->result(cmdid, result);
}
void SimpleObjectQueryProcessor::commandStats(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    result.put( String("stats"), Command::EmptyResult());
    cmdr->result(cmdid, result);
}

} // namespace Simple
} // namespace OH
} // namespace Sirikata
