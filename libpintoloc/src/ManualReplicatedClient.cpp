// Copyright (c) 2012 Sirikata Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include <sirikata/pintoloc/ManualReplicatedClient.hpp>

#include <sirikata/core/service/Context.hpp>

#include <sirikata/core/network/Message.hpp> // parse/serializePBJMessage
#include "Protocol_Prox.pbj.hpp"
#include "Protocol_Loc.pbj.hpp"

#include <json_spirit/json_spirit.h>
#include <boost/lexical_cast.hpp>

#define RCLOG(lvl, msg) SILOG(replicated-client, lvl, "(Server " << mServerID << ") " << msg)

namespace Sirikata {
namespace Pinto {
namespace Manual {



namespace {

// Helpers for encoding requests
String initRequest() {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "init");
    return json::write(req);
}

String destroyRequest() {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "destroy");
    return json::write(req);
}

String refineRequest(const ProxIndexID proxid, const std::vector<ObjectReference>& aggs) {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "refine");
    req.put("index", proxid);
    req.put("nodes", json::Array());
    json::Array& nodes = req.getArray("nodes");
    for(uint32 i = 0; i < aggs.size(); i++)
        nodes.push_back( json::Value(aggs[i].toString()) );
    return json::write(req);
}

String coarsenRequest(const ProxIndexID proxid, const std::vector<ObjectReference>& aggs) {
    namespace json = json_spirit;
    json::Value req = json::Object();
    req.put("action", "coarsen");
    req.put("index", proxid);
    req.put("nodes", json::Array());
    json::Array& nodes = req.getArray("nodes");
    for(uint32 i = 0; i < aggs.size(); i++)
        nodes.push_back( json::Value(aggs[i].toString()) );
    return json::write(req);
}

} // namespace


using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;
using std::tr1::placeholders::_3;


ReplicatedClient::ReplicatedClient(Context* ctx, Network::IOStrandPtr strand, TimeSynced* sync, const String& server_id)
 : mContext(ctx),
   doNotUse___mStrand(strand),
   mStrand(strand.get()),
   mSync(sync),
   mServerID(server_id),
   mObjects(),
   mOrphans(),
   mUnobservedTimeouts(),
   mUnobservedTimer(
       Network::IOTimer::create(
           strand.get(),
           std::tr1::bind(&ReplicatedClient::processExpiredNodes, this)
       )
   ),
   mCleanupOrphansPoller(
       strand.get(),
       std::tr1::bind(&ReplicatedClient::cleanupOrphans, this),
       "ReplicatedClient::cleanupOrphans",
       Duration::minutes(1)
   )
{
}

ReplicatedClient::ReplicatedClient(Context* ctx, Network::IOStrand* strand, TimeSynced* sync, const String& server_id)
 : mContext(ctx),
   doNotUse___mStrand(),
   mStrand(strand),
   mSync(sync),
   mServerID(server_id),
   mObjects(),
   mOrphans(),
   mUnobservedTimeouts(),
   mUnobservedTimer(
       Network::IOTimer::create(
           strand,
           std::tr1::bind(&ReplicatedClient::processExpiredNodes, this)
       )
   ),
   mCleanupOrphansPoller(
       strand,
       std::tr1::bind(&ReplicatedClient::cleanupOrphans, this),
       "ReplicatedClient::cleanupOrphans",
       Duration::minutes(1)
   )
{
}

ReplicatedClient::~ReplicatedClient() {
    for(IndexOrphanLocUpdateMap::iterator it = mOrphans.begin(); it != mOrphans.end(); it++)
        it->second->stop();
    mUnobservedTimer->cancel();

    delete mSync;
}


void ReplicatedClient::start() {
    mCleanupOrphansPoller.start();
}

void ReplicatedClient::stop() {
    mCleanupOrphansPoller.stop();
}


void ReplicatedClient::initQuery() {
    sendProxMessage(initRequest());
}

void ReplicatedClient::destroyQuery() {
    sendProxMessage(destroyRequest());
}

namespace {
String ObjectReferenceListString(const std::vector<ObjectReference>& aggs) {
    String result = "[ ";
    for(uint32 i = 0; i < aggs.size(); i++) {
        result += aggs[i].toString() + ", ";
    }
    result += ']';
    return result;
}
}

void ReplicatedClient::sendRefineRequest(const ProxIndexID proxid, const ObjectReference& agg) {
    std::vector<ObjectReference> aggs;
    aggs.push_back(agg);
    sendRefineRequest(proxid, aggs);
}

void ReplicatedClient::sendRefineRequest(const ProxIndexID proxid, const std::vector<ObjectReference>& aggs) {
    RCLOG(detailed, "Requesting refinement in " << proxid << " of nodes " << ObjectReferenceListString(aggs));
    sendProxMessage(refineRequest(proxid, aggs));
}

void ReplicatedClient::sendCoarsenRequest(const ProxIndexID proxid, const ObjectReference& agg) {
    std::vector<ObjectReference> aggs;
    aggs.push_back(agg);
    sendCoarsenRequest(proxid, aggs);
}

void ReplicatedClient::sendCoarsenRequest(const ProxIndexID proxid, const std::vector<ObjectReference>& aggs) {
    RCLOG(detailed, "Requesting coarsening in " << proxid << " of nodes " << ObjectReferenceListString(aggs));
    sendProxMessage(coarsenRequest(proxid, aggs));
}



// Proximity tracking updates in local queries to trigger adjustments to server query
void ReplicatedClient::queriersAreObserving(ProxIndexID indexid, const ObjectReference& objid) {
    // Someone is observing this node, we should try to refine it
    sendRefineRequest(indexid, objid);

    // And make sure we don't have it lined up for coarsening
    UnobservedNodesByID& by_id = mUnobservedTimeouts.get<objid_tag>();
    UnobservedNodesByID::iterator it = by_id.find(IndexObjectReference(indexid, objid));
    if (it == by_id.end()) return;
    by_id.erase(it);
}

void ReplicatedClient::queriersStoppedObserving(ProxIndexID indexid, const ObjectReference& objid) {
    // Nobody is observing this node, we should try to coarsen it after awhile,
    // but only if it doesn't have children.
    mUnobservedTimeouts.insert(UnobservedNodeTimeout(IndexObjectReference(indexid, objid), mContext->recentSimTime() + Duration::seconds(15)));
    // And restart the timeout for the most recent
    Duration next_timeout = mUnobservedTimeouts.get<expires_tag>().begin()->expires - mContext->recentSimTime();
    mUnobservedTimer->cancel();
    mUnobservedTimer->wait(next_timeout);
}

void ReplicatedClient::processExpiredNodes() {
    Time curt = mContext->recentSimTime();
    UnobservedNodesByExpiration& by_expires = mUnobservedTimeouts.get<expires_tag>();
    while(!by_expires.empty() &&
        by_expires.begin()->expires < curt)
    {
        sendCoarsenRequest(by_expires.begin()->objid.indexid, by_expires.begin()->objid.objid);
        by_expires.erase(by_expires.begin());
    }

    if (!by_expires.empty()) {
        Duration next_timeout = mUnobservedTimeouts.get<expires_tag>().begin()->expires - mContext->recentSimTime();
        mUnobservedTimer->wait(next_timeout);
    }
}

void ReplicatedClient::proxUpdate(const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    mStrand->post(std::tr1::bind(&ReplicatedClient::handleProxUpdate, this, update));
}
void ReplicatedClient::handleProxUpdate(const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    SerializationCheck::Scoped sc(this);
    // We may have to trigger creation of a new replicated tree if this is a
    // new root. We should always extract the ID of the tree this update
    // applies to
    assert(update.has_index_properties());
    Sirikata::Protocol::Prox::IndexProperties index_props = update.index_properties();
    assert(index_props.has_id());
    uint32 index_unique_id = index_props.id();
    RCLOG(detailed, "-Update for tree index ID: " << index_unique_id);
    if (index_props.has_index_id() || index_props.has_dynamic_classification()) {
        RCLOG(detailed, " ID: " << index_props.id());
        ServerID index_from_server = NullServerID;
        if (index_props.has_index_id()) {
            RCLOG(detailed, " Tree ID: " << index_props.index_id());
            index_from_server = boost::lexical_cast<ServerID>(index_props.index_id());
        }
        bool dynamic_objects = true;
        if (index_props.has_dynamic_classification()) {
            dynamic_objects = !(index_props.dynamic_classification() == Sirikata::Protocol::Prox::IndexProperties::Static);
            RCLOG(detailed, " Class: " << (dynamic_objects ? "dynamic" : "static"));
        }

        // Create a new loccache for this new tree. We could be getting this
        bool is_new = createLocCache(index_unique_id);
        if (is_new) {
            RCLOG(detailed, " Replicated tree is NEW");
            onCreatedReplicatedIndex(index_unique_id, getLocCache(index_unique_id), index_from_server, dynamic_objects);
        }
    }

    ReplicatedLocationServiceCachePtr loccache = getLocCache(index_unique_id);
    OrphanLocUpdateManagerPtr orphan_manager = getOrphanLocUpdateManager(index_unique_id);
    for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
        // Convert to local time
        addition.location().set_t(
            mSync->localTime(addition.location().t())
        );

        ProxProtocolLocUpdate add(addition);

        ObjectReference observed_oref(addition.object());
        RCLOG(insane, " - Addition: " << observed_oref);
        // NOTE: We don't track the SpaceID here, but we also don't really
        // need it -- OrphanLocUpdateManager only uses it because it can be
        // used in places where a single instance covers multiple spaces,
        // but it doesn't here.
        SpaceObjectReference observed(SpaceID::null(), observed_oref);

        bool is_agg = (addition.has_type() && addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate);

        ObjectReference parent_oref(addition.parent());

        // Store the data
        loccache->objectAdded(
            observed_oref, is_agg,
            parent_oref,
            add.location(), add.location_seqno(),
            add.orientation(), add.orientation_seqno(),
            add.bounds(), add.bounds_seqno(),
            Transfer::URI(add.meshOrDefault()), add.mesh_seqno(),
            add.physicsOrDefault(), add.physics_seqno()
        );

        // Replay orphans
        orphan_manager->invokeOrphanUpdates1(*mSync, observed, this, index_unique_id);
    }

    for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
        Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);
        ObjectReference observed_oref(removal.object());
        RCLOG(insane, " - Removal: " << observed_oref);
        // NOTE: see above note about why SpaceID::null() is ok here
        SpaceObjectReference observed(SpaceID::null(), observed_oref);
        bool temporary_removal = (!removal.has_type() || (removal.type() == Sirikata::Protocol::Prox::ObjectRemoval::Transient));
        // Backup data for orphans and then destroy
        // TODO(ewencp) Seems like we shouldn't actually need this
        // since we shouldn't get a removal with having had an
        // addition first...
        if (loccache->startSimpleTracking(observed_oref)) {
            orphan_manager->addUpdateFromExisting(observed, loccache->properties(observed_oref));
            loccache->stopSimpleTracking(observed_oref);
        }
        loccache->objectRemoved(
            observed_oref, temporary_removal
        );
    }
    RCLOG(insane, " ----- Done");
    // We may have removed everything from the specified tree. We need to
    // check if we've hit that condition and clean out any associated
    // state. If objects from that server reappear, this state will be
    // created from scratch.
    if (loccache->empty()) {
        onDestroyedReplicatedIndex(index_unique_id);
        removeLocCache(index_unique_id);
    }

}

void ReplicatedClient::proxUpdate(const Sirikata::Protocol::Prox::ProximityResults& results) {
    mStrand->post(std::tr1::bind(&ReplicatedClient::handleProxUpdateResults, this, results));
}
void ReplicatedClient::handleProxUpdateResults(const Sirikata::Protocol::Prox::ProximityResults& results) {
    // Proximity messages are handled just by updating our state
    // tracking the objects, adding or removing the objects or
    // updating their properties.
    RCLOG(detailed, "Received proximity message with " << results.update_size() << " updates");
    for(int32 idx = 0; idx < results.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = results.update(idx);
        proxUpdate(update);
    }
}


// Location

namespace {
// Helper for applying an update to
void applyLocUpdate(const ObjectReference& objid, ReplicatedLocationServiceCachePtr loccache, const LocUpdate& lu) {
    // We can bail immediately if none of the updates are relevant since the
    // object isn't even being tracked
    if (!loccache->tracking(objid)) return;

    if (lu.has_epoch())
        loccache->epochUpdated(objid, lu.epoch());
    if (lu.has_parent())
        loccache->parentUpdated(objid, lu.parent(), lu.parent_seqno());
    if (lu.has_location())
        loccache->locationUpdated(objid, lu.location(), lu.location_seqno());
    if (lu.has_orientation())
        loccache->orientationUpdated(objid, lu.orientation(), lu.orientation_seqno());
    if (lu.has_bounds())
        loccache->boundsUpdated(objid, lu.bounds(), lu.bounds_seqno());
    if (lu.has_mesh())
        loccache->meshUpdated(objid, Transfer::URI(lu.meshOrDefault()), lu.mesh_seqno());
    if (lu.has_physics())
        loccache->physicsUpdated(objid, lu.physicsOrDefault(), lu.physics_seqno());
}
}

void ReplicatedClient::locUpdate(const Sirikata::Protocol::Loc::LocationUpdate& update) {
    mStrand->post(std::tr1::bind(&ReplicatedClient::handleLocUpdate, this, update));
}
void ReplicatedClient::handleLocUpdate(const Sirikata::Protocol::Loc::LocationUpdate& update) {
    SerializationCheck::Scoped sc(this);
    ObjectReference observed_oref(update.object());
    // NOTE: We don't track the SpaceID here, but we also don't really need it
    // -- OrphanLocUpdateManager only uses it because it can be used in places
    // where a single instance covers multiple spaces, but it doesn't here.
    SpaceObjectReference observed(SpaceID::null(), observed_oref);

    // Because of prox/loc ordering, we may or may not have a record of the
    // object yet. We need to send to either OrphanLocUpdateManager or, if
    // we do already have the object, we can apply it now.

    // However, in the case of replicated trees, we need to make sure we
    // apply to the right trees. We need the server to have told us which
    // ones to apply it to
    if (update.index_id_size() == 0) {
        RCLOG(error, "LocationUpdate didn't include index_id to indicate which replicated trees need it, this update will be ignored!");
        return;
    }

    for(int32 index_id_idx = 0; index_id_idx < update.index_id_size(); index_id_idx++) {
        ProxIndexID index_id = update.index_id(index_id_idx);
        IndexObjectCacheMap::iterator index_it = mObjects.find(index_id);
        // If we can't find an index, we may just be getting the update
        // earlier than we're getting the prox update, i.e. just what
        // OrphanLocUpdateManager handles. But we haven't necessarily
        // allocated one yet. We'll allocate the state, but *not* notify the
        // parent that we createdReplicatedIndex yet since we haven't really
        // yet and we need the info from the prox update's IndexProperties
        // to do that. See proxUpdate for how this is finally
        // resolved.
        if (index_it == mObjects.end()) {
            createOrphanLocUpdateManager(index_id);
            // Add it to a list so we can clean it out eventually
            // if we never get prox data for it
            mCachesForOrphans.push_back(index_id);
        }
        // Then we just continue as normal, sticking it in as an orphan if
        // necessary
        if (index_it == mObjects.end() || !getLocCache(index_id)->tracking(observed_oref)) {
            getOrphanLocUpdateManager(index_id)->addOrphanUpdate(observed, update);
        }
        else { // or actually applying it
            LocProtocolLocUpdate llu(update, *mSync);
            applyLocUpdate(observed_oref, getLocCache(index_id), llu);
        }
    }
}


void ReplicatedClient::onOrphanLocUpdate(const LocUpdate& lu, ProxIndexID iid) {
    SerializationCheck::Scoped sc(this);
    assert(getLocCache(iid)->tracking(lu.object()));
    applyLocUpdate(lu.object(), getLocCache(iid), lu);
}


bool ReplicatedClient::createLocCache(ProxIndexID iid) {
    bool is_new = false;
    if (mObjects.find(iid) == mObjects.end()) {
        mObjects[iid] = ReplicatedLocationServiceCachePtr(new ReplicatedLocationServiceCache(mStrand));
        is_new = true;
    }
    createOrphanLocUpdateManager(iid);
    return is_new;
}

void ReplicatedClient::createOrphanLocUpdateManager(ProxIndexID iid) {
    if (mOrphans.find(iid) == mOrphans.end())
        mOrphans[iid] = OrphanLocUpdateManagerPtr(new OrphanLocUpdateManager(mContext, mContext->mainStrand, Duration::seconds(10)));
}

ReplicatedLocationServiceCachePtr ReplicatedClient::getLocCache(ProxIndexID iid) {
    assert(mObjects.find(iid) != mObjects.end());
    return mObjects[iid];
}

OrphanLocUpdateManagerPtr ReplicatedClient::getOrphanLocUpdateManager(ProxIndexID iid) {
    assert(mOrphans.find(iid) != mOrphans.end());
    return mOrphans[iid];
}

void ReplicatedClient::removeLocCache(ProxIndexID iid) {
    assert(mObjects.find(iid) != mObjects.end());
    mObjects.erase(iid);
    assert(mOrphans.find(iid) != mOrphans.end());
    mOrphans.erase(iid);
}

void ReplicatedClient::cleanupOrphans() {
    SerializationCheck::Scoped sc(this);

    // Backwards so we can erase as we go
    for(int i = mCachesForOrphans.size()-1; i >= 0; i--) {
        // If we don't even have the data anymore, delete and move on
        if (mObjects.find(mCachesForOrphans[i]) == mObjects.end()) {
            mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
            continue;
        }
        // If the orphan manager still has entries, we need to wait longer
        if (!getOrphanLocUpdateManager(mCachesForOrphans[i])->empty())
            continue;
        // If the loccache has data, then we've gotten something back from it,
        // so we can just remove this entry from our list. It should get cleaned
        // up normally
        if (!getLocCache(mCachesForOrphans[i])->fullyEmpty()) {
            mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
            continue;
        }
        // And in any other case, we need to clean up.
        removeLocCache(mCachesForOrphans[i]);
        mCachesForOrphans.erase(mCachesForOrphans.begin() + i);
    }
}

} // namespace Manual
} // namespace Pinto
} // namespace Sirikata
