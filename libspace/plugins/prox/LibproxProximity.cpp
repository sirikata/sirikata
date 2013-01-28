/*  Sirikata
 *  LibproxProximity.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "LibproxProximity.hpp"
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include <algorithm>

#include <sirikata/pintoloc/QueryHandlerFactory.hpp>

#include <sirikata/core/odp/SST.hpp>

#include "Protocol_Prox.pbj.hpp"
#include "Protocol_ServerProx.pbj.hpp"

#include <sirikata/space/AggregateManager.hpp>

#include <sirikata/core/command/Commander.hpp>
#include <json_spirit/json_spirit.h>

#include <boost/lexical_cast.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,msg)

namespace Sirikata {

static SolidAngle NoUpdateSolidAngle = SolidAngle(0.f);
// Note that this is different from sentinals indicating infinite
// results. Instead, this indicates that we shouldn't even request the update,
// leaving it as is, because we don't actually have a new value.
static uint32 NoUpdateMaxResults = ((uint32)INT_MAX)+1;

namespace {

bool parseQueryRequest(const String& query, SolidAngle* qangle_out, uint32* max_results_out) {
    if (query.empty())
        return false;

    namespace json = json_spirit;
    json::Value parsed;
    if (!json::read(query, parsed))
        return false;

    *qangle_out = SolidAngle( parsed.getReal("angle", SolidAngle::Max.asFloat()) );
    *max_results_out = parsed.getInt("max_result", 0);

    return true;
}

}

LibproxProximity::LibproxProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : LibproxProximityBase(ctx, locservice, cseg, net, aggmgr),
   mDistanceQueryDistance(0.f),
   mMinObjectQueryAngle(SolidAngle::Max),
   mMaxMaxCount(1),
   mServerQueries(),
   mServerDistance(false),
   mServerHandlerPoller(mProxStrand, std::tr1::bind(&LibproxProximity::tickQueryHandler, this, mServerQueryHandler), "LibproxProximity ServerHandler Poll", Duration::milliseconds((int64)100)),
   mServerQueryBoundsPoller(mProxStrand, std::tr1::bind(&LibproxProximity::recomputeAggregateQueryBounds, this), "LibproxProximity Aggregate Query Bounds Poll", Duration::seconds((int64)1)),
   mObjectQueries(),
   mObjectDistance(false),
   mObjectHandlerPoller(mProxStrand, std::tr1::bind(&LibproxProximity::tickQueryHandler, this, mObjectQueryHandler), "LibproxProximity ObjectHandler Poll", Duration::milliseconds((int64)100)),
   mStaticRebuilderPoller(mProxStrand, std::tr1::bind(&LibproxProximity::rebuildHandler, this, OBJECT_CLASS_STATIC), "LibproxProximity Static Rebuilder Poll", Duration::seconds(172800.f)),
   mDynamicRebuilderPoller(mProxStrand, std::tr1::bind(&LibproxProximity::rebuildHandler, this, OBJECT_CLASS_DYNAMIC), "LibproxProximity Dynamic Rebuilder Poll", Duration::seconds(172800.f))
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    using std::tr1::placeholders::_4;
    using std::tr1::placeholders::_5;

    // Generic query parameters
    mDistanceQueryDistance = GetOptionValue<float32>(OPT_PROX_QUERY_RANGE);

    // Server Queries
    String server_handler_type = GetOptionValue<String>(OPT_PROX_SERVER_QUERY_HANDLER_TYPE);
    String server_handler_options = GetOptionValue<String>(OPT_PROX_SERVER_QUERY_HANDLER_OPTIONS);
    String server_handler_node_data = GetOptionValue<String>(OPT_PROX_SERVER_QUERY_HANDLER_NODE_DATA);
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mServerQueryHandler[i].handler = NULL;
            continue;
        }
        mServerQueryHandler[i].handler = ObjectProxGeomQueryHandlerFactory.getConstructor(server_handler_type, server_handler_node_data)(server_handler_options, true);
        mServerQueryHandler[i].handler->setAggregateListener(this); // *Must* be before handler->initialize
        bool server_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mServerQueryHandler[i].handler->initialize(
            mLocCache, mLocCache,
            server_static_objects, false /* not replicated */,
            std::tr1::bind(&LibproxProximity::handlerShouldHandleObject, this, server_static_objects, false, _1, _2, _3, _4, _5, _6)
        );
    }
    if (server_handler_type == "dist" || server_handler_type == "rtreedist") mServerDistance = true;

    // Object Queries
    String object_handler_type = GetOptionValue<String>(OPT_PROX_OBJECT_QUERY_HANDLER_TYPE);
    String object_handler_options = GetOptionValue<String>(OPT_PROX_OBJECT_QUERY_HANDLER_OPTIONS);
    String object_handler_node_data = GetOptionValue<String>(OPT_PROX_OBJECT_QUERY_HANDLER_NODE_DATA);
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mObjectQueryHandler[i].handler = NULL;
            continue;
        }
        mObjectQueryHandler[i].handler = ObjectProxGeomQueryHandlerFactory.getConstructor(object_handler_type, object_handler_node_data)(object_handler_options, true);
        mObjectQueryHandler[i].handler->setAggregateListener(this); // *Must* be before handler->initialize
        bool object_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mObjectQueryHandler[i].handler->initialize(
            mLocCache, mLocCache,
            object_static_objects, false /* not replicated */,
            std::tr1::bind(&LibproxProximity::handlerShouldHandleObject, this, object_static_objects, true, _1, _2, _3, _4, _5, _6)
        );
    }
    if (object_handler_type == "dist" || object_handler_type == "rtreedist") mObjectDistance = true;
}

LibproxProximity::~LibproxProximity() {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        delete mObjectQueryHandler[i].handler;
        delete mServerQueryHandler[i].handler;
    }
}



// BOTH Threads - methods using thread safe data

// Setup all known servers for a server query update
void LibproxProximity::addAllServersForUpdate() {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    for(ServerSet::const_iterator it = mServersQueried.begin(); it != mServersQueried.end(); it++)
        mNeedServerQueryUpdate.insert(*it);
}

void LibproxProximity::getServersForAggregateQueryUpdate(ServerSet* servers_out) {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    servers_out->swap(mNeedServerQueryUpdate);
}

void LibproxProximity::addServerForAggregateQueryUpdate(ServerID sid) {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    mNeedServerQueryUpdate.insert(sid);
}

void LibproxProximity::updateAggregateQuery() {
    PROXLOG(debug,"Query addition initiated server query request.");
    addAllServersForUpdate();

    // This is a bit ridiculous since we're just passing this data to another
    // local service that will decode it, but this keeps the PintoServerQuerier
    // interface generic
    namespace json = json_spirit;
    json::Object update_params_obj;
    json::Value update_params(update_params_obj);
    update_params.put("angle", mMinObjectQueryAngle.asFloat());
    update_params.put("max_result", mMaxMaxCount);
    mServerQuerier->updateQuery(json::write(update_params));
}

uint32 LibproxProximity::numServersQueried() {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    return mServersQueried.size();
}




// MAIN Thread Methods: The following should only be called from the main thread.

// Service Interface

void LibproxProximity::start() {
    LibproxProximityBase::start();

    mContext->add(&mServerHandlerPoller);
    mContext->add(&mObjectHandlerPoller);
    mContext->add(&mStaticRebuilderPoller);
    mContext->add(&mDynamicRebuilderPoller);
    mContext->add(&mServerQueryBoundsPoller);
}


// ObjectSessionListener Interface

void LibproxProximity::newSession(ObjectSession* session) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    SST::Stream<SpaceObjectReference>::Ptr strm = session->getStream();
    SST::Connection<SpaceObjectReference>::Ptr conn = strm->connection().lock();
    assert(conn);

    SpaceObjectReference sourceObject = conn->remoteEndPoint().endPoint;

    conn->registerReadDatagramCallback( OBJECT_PORT_PROXIMITY,
        std::tr1::bind(
            &LibproxProximity::handleObjectProximityMessage, this,
            sourceObject.object().getAsUUID(),
            _1, _2
        )
    );
}

void LibproxProximity::sessionClosed(ObjectSession* session) {
    // Prox strand may  have some state to clean up
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleDisconnectedObject, this, session->id().getAsUUID()),
        "LibproxProximity::handleDisconnectedObject"
    );

    ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.find(session->id().getAsUUID());
    if (prox_stream_it != mObjectProxStreams.end()) {
        prox_stream_it->second->disable();
        mObjectProxStreams.erase(prox_stream_it);
    }
}


void LibproxProximity::handleObjectProximityMessage(const UUID& objid, void* buffer, uint32 length) {
    Sirikata::Protocol::Prox::QueryRequest prox_update;
    bool parse_success = prox_update.ParseFromString( String((char*) buffer, length) );
    if (!parse_success) {
        LOG_INVALID_MESSAGE_BUFFER(prox, error, ((char*)buffer), length);
        return;
    }

    mStats.objectReceivedBytes += length;
    mStats.objectReceivedMessages++;

    // Need to support old style (angle + max count) and new style.
    if (prox_update.has_query_parameters()) {
        SolidAngle sa;
        uint32 max_results;
        if (parseQueryRequest(prox_update.query_parameters(), &sa, &max_results))
            updateQuery(objid, mLocService->location(objid), mLocService->bounds(objid).fullBounds(), sa, max_results);
    }
    else {
        if (!prox_update.has_query_angle()) return;

        SolidAngle query_angle(prox_update.query_angle());
        uint32 query_max_results = (prox_update.has_query_max_count() ? prox_update.query_max_count() : NoUpdateMaxResults);
        updateQuery(objid, mLocService->location(objid), mLocService->bounds(objid).fullBounds(), query_angle, query_max_results);
    }
}


void LibproxProximity::sendQueryRequests() {
    TimedMotionVector3f loc;

    ServerSet sub_servers;
    getServersForAggregateQueryUpdate(&sub_servers);
    for(ServerSet::const_iterator it = sub_servers.begin(); it != sub_servers.end(); it++) {
        ServerID sid = *it;
        Sirikata::Protocol::Prox::Container container;
        Sirikata::Protocol::Prox::IServerQuery msg = container.mutable_query();
        msg.set_action(Sirikata::Protocol::Prox::ServerQuery::AddOrUpdate);
        Sirikata::Protocol::ITimedMotionVector msg_loc = msg.mutable_location();
        msg_loc.set_t(loc.updateTime());
        msg_loc.set_position(loc.position());
        msg_loc.set_position(loc.velocity());
        // This protocol only had bounds, so we just drop some
        // information, which for this querying shouldn't matter
        // anyway.
        msg.set_bounds(mLastAggregateQuerierBounds.fullBounds());
        msg.set_min_angle(mMinObjectQueryAngle.asFloat());
        msg.set_max_count(mMaxMaxCount);

        Message* smsg = new Message(
            mContext->id(),
            SERVER_PORT_PROX,
            sid,
            SERVER_PORT_PROX,
            serializePBJMessage(container)
        );
        bool sent = sendServerMessage(smsg);
        if (!sent) {
            delete smsg;
            addServerForAggregateQueryUpdate(sid);
        }
    }
}


// PintoServerQuerierListener Interface

void LibproxProximity::onPintoServerResult(const Sirikata::Protocol::Prox::ProximityUpdate& update) {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);

    for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
        ServerID sid = addition.object().asUInt32();
        // Ignore results that are for ourselves or for internal nodes
        if (sid == mContext->id()) continue;
        if (addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate) continue;

        PROXLOG(detailed, "Top-level pinto added server " << sid);
        mServersQueried.insert(sid);
        mNeedServerQueryUpdate.insert(sid);
    }

    // We're not replicating the pinto tree, just using results, so we can
    // ignore reparent events

    for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
        Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);
        ServerID sid = removal.object().asUInt32();
        // Ignore results that are for ourselves or for internal nodes
        if (sid == mContext->id()) continue;
        // Can't check for aggregate (internal) nodes, so we have to rely on
        // checking whether we've added it as a queried server

        PROXLOG(detailed, "Top-level pinto removed server " << sid);
        if (mServersQueried.find(sid) != mServersQueried.end()) mServersQueried.erase(sid);
    }
}

void LibproxProximity::onPintoServerLocUpdate(const LocUpdate& update) {
    // Nothing to do here. We don't actually save the resulting data about the
    // servers, we just care about their membership in the result set from top
    // level pinto so we can query/update them.
}

// MessageRecipient Interface

void LibproxProximity::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_PROX);

    LibproxProximityBase::serverMessageReceived(msg);

    Sirikata::Protocol::Prox::Container prox_container;
    bool parsed = parsePBJMessage(&prox_container, msg->payload());
    if (!parsed) {
        PROXLOG(warn,"Couldn't parse message, ID=" << msg->id());
        delete msg;
        return;
    }

    ServerID source_server = msg->source_server();

    if (prox_container.has_query()) {
        Sirikata::Protocol::Prox::ServerQuery prox_query_msg = prox_container.query();

        if (prox_query_msg.action() == Sirikata::Protocol::Prox::ServerQuery::AddOrUpdate) {
            assert(
                prox_query_msg.has_location() &&
                prox_query_msg.has_bounds() &&
                prox_query_msg.has_min_angle()
            );

            Sirikata::Protocol::TimedMotionVector msg_loc = prox_query_msg.location();
            TimedMotionVector3f qloc(msg_loc.t(), MotionVector3f(msg_loc.position(), msg_loc.velocity()));
            SolidAngle minangle(prox_query_msg.min_angle());
            uint32 query_max_results = (prox_query_msg.has_max_count() ? prox_query_msg.max_count() : NoUpdateMaxResults);

            // Figure out whether this is a useful query. To keep the management
            // of server queries simple, we might get a request for a query but
            // with angle == SolidAngle::Max. That's an indicator that we
            // shouldn't return any results, but because objects within the
            // bounds of the querier will be returned, which can be very large
            // for a server, we'll actually return stuff if we register these
            // queries. Instead, here, we just figure out if we should actually
            // be ignoring/removing a query based on these new values.
            if (minangle > (SolidAngle::Max/2)) // Conservative to avoid small FP errors
                removeQuery(source_server);
            else
                updateQuery(source_server, qloc, prox_query_msg.bounds(), minangle, query_max_results);
        }
        else if (prox_query_msg.action() == Sirikata::Protocol::Prox::ServerQuery::Remove) {
            removeQuery(source_server);
        }
        else {
            assert(false);
        }
    }

    if (prox_container.has_result()) {
        if (!mServerQueryResults[source_server])
            mServerQueryResults[source_server] = ObjectSetPtr(new ObjectSet());

        Sirikata::Protocol::Prox::ProximityResults prox_result_msg = prox_container.result();

        assert( prox_result_msg.has_t() );
        Time t = prox_result_msg.t();

        for(int32 idx = 0; idx < prox_result_msg.update_size(); idx++) {
            Sirikata::Protocol::Prox::ProximityUpdate update = prox_result_msg.update(idx);

            for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
                Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
                mServerQueryResults[source_server]->insert(addition.object());

                assert(addition.has_aggregate_bounds());
                Vector3f center = addition.aggregate_bounds().has_center_offset() ? addition.aggregate_bounds().center_offset() : Vector3f(0,0,0);
                float32 center_rad = addition.aggregate_bounds().has_center_bounds_radius() ? addition.aggregate_bounds().center_bounds_radius() : 0.f;
                float32 max_object_size = addition.aggregate_bounds().has_max_object_size() ? addition.aggregate_bounds().max_object_size() : 0.f;

                mLocService->addReplicaObject(
                    t,
                    addition.object(),
                    (addition.type() == Sirikata::Protocol::Prox::ObjectAddition::Aggregate),
                    TimedMotionVector3f( addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()) ),
                    TimedMotionQuaternion( addition.orientation().t(), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()) ),
                    AggregateBoundingInfo(center, center_rad, max_object_size),
                    (addition.has_mesh() ? addition.mesh() : ""),
                    (addition.has_physics() ? addition.physics() : ""),
                     ""  //no Zernike descriptor in Proximity message. is this ok to do? TAHIR.
                );
            }

            // This implementation just uses the cut or leaf results from other
            // servers, it doesn't perform replication of the tree, so reparent
            // events can be ignored.

            for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
                Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);
                mLocService->removeReplicaObject(t, removal.object());
                mServerQueryResults[source_server]->erase(removal.object());
            }
        }
    }

    delete msg;
}

// MigrationDataClient Interface

std::string LibproxProximity::migrationClientTag() {
    return "prox";
}

std::string LibproxProximity::generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) {
    ObjectQueryAngleMap::iterator it = mObjectQueryAngles.find(obj);
    if (it == mObjectQueryAngles.end()) // no query registered, return nothing
        return std::string();
    else {
        SolidAngle query_angle = it->second;
        removeQuery(obj);

        Sirikata::Protocol::Prox::ObjectMigrationData migr_data;
        migr_data.set_min_angle( query_angle.asFloat() );
        if (mObjectQueryMaxCounts.find(obj) != mObjectQueryMaxCounts.end())
            migr_data.set_max_count( mObjectQueryMaxCounts[obj] );
        return serializePBJMessage(migr_data);
    }
}

void LibproxProximity::receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) {
    Sirikata::Protocol::Prox::ObjectMigrationData migr_data;
    bool parse_success = migr_data.ParseFromString(data);
    if (!parse_success) {
        LOG_INVALID_MESSAGE(prox, error, data);
        return;
    }

    SolidAngle obj_query_angle(migr_data.min_angle());
    uint32 obj_query_max_results = (migr_data.has_max_count() ? migr_data.max_count() : ObjectProxSimulationTraits::InfiniteResults);
    addQuery(obj, obj_query_angle, obj_query_max_results);
}


// AggregateListener Interface

void LibproxProximity::aggregateObjectCreated(ProxAggregator* handler, const ObjectReference& objid) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateObjectCreated(objid);
}

void LibproxProximity::aggregateObjectDestroyed(ProxAggregator* handler, const ObjectReference& objid) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateObjectDestroyed(objid);
}

void LibproxProximity::aggregateCreated(ProxAggregator* handler, const ObjectReference& objid) {
    // We ignore aggregates built of dynamic objects, they aren't useful for
    // creating aggregate meshes
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateCreated(objid);
}

void LibproxProximity::aggregateChildAdded(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateChildAdded(objid, child, bnds_center, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void LibproxProximity::aggregateChildRemoved(ProxAggregator* handler, const ObjectReference& objid, const ObjectReference& child, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateChildRemoved(objid, child, bnds_center, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void LibproxProximity::aggregateBoundsUpdated(ProxAggregator* handler, const ObjectReference& objid, const Vector3f& bnds_center, const float32 bnds_center_radius, const float32 max_obj_size) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateBoundsUpdated(objid, bnds_center, AggregateBoundingInfo(Vector3f::zero(), bnds_center_radius, max_obj_size));
}

void LibproxProximity::aggregateQueryDataUpdated(ProxAggregator* handler, const ObjectReference& objid, const String& qd) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateQueryDataUpdated(objid, qd, (handler->rootAggregateID() == objid));
}

void LibproxProximity::aggregateDestroyed(ProxAggregator* handler, const ObjectReference& objid) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateDestroyed(objid);
}

void LibproxProximity::aggregateObserved(ProxAggregator* handler, const ObjectReference& objid, uint32 nobservers, uint32 nchildren) {
    if (!static_cast<ProxQueryHandler*>(handler)->staticOnly()) return;
    LibproxProximityBase::aggregateObserved(objid, nobservers, nchildren);
}




void LibproxProximity::updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa, uint32 max_results) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleUpdateServerQuery, this, sid, loc, bounds, sa, max_results),
        "LibproxProximity::handleUpdateServerQuery"
    );
}

void LibproxProximity::removeQuery(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleRemoveServerQuery, this, sid),
        "LibproxProximity::handleRemoveServerQuery"
    );
}

void LibproxProximity::addQuery(UUID obj, SolidAngle sa, uint32 max_results) {
    updateQuery(obj, mLocService->location(obj), mLocService->bounds(obj).fullBounds(), sa, max_results);
}

void LibproxProximity::addQuery(UUID obj, const String& params) {
    SolidAngle sa;
    uint32 max_results;
    if (parseQueryRequest(params, &sa, &max_results))
        updateQuery(obj, mLocService->location(obj), mLocService->bounds(obj).fullBounds(), sa, max_results);
}

void LibproxProximity::updateQuery(UUID obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results) {
    SeqNoPtr obj_seqno = mContext->objectSessionManager()->getSession(ObjectReference(obj))->getSeqNoPtr();

    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleUpdateObjectQuery, this, obj, loc, bounds, sa, max_results, obj_seqno),
        "LibproxProximity::handleUpdateObjectQuery"
    );

    bool update_remote_queries = false;
    if (sa != NoUpdateSolidAngle) {
        // Update the main thread's record
        mObjectQueryAngles[obj] = sa;

        // Update min query angle, and update remote queries if necessary
        if (sa < mMinObjectQueryAngle) {
            mMinObjectQueryAngle = sa;
            update_remote_queries = true;
        }
    }

    if (max_results != NoUpdateMaxResults) {
        // Update the main thread's record
        mObjectQueryMaxCounts[obj] = max_results;

        // Update min query angle, and update remote queries if necessary
        if (max_results > mMaxMaxCount) {
            mMaxMaxCount = max_results;
            update_remote_queries = true;
        }
    }

    if (update_remote_queries)
        updateAggregateQuery();
}

void LibproxProximity::removeQuery(UUID obj) {
    // Update the main thread's record
    SolidAngle sa = mObjectQueryAngles[obj];
    mObjectQueryAngles.erase(obj);
    uint32 max_count = mObjectQueryMaxCounts[obj];
    mObjectQueryMaxCounts.erase(obj);

    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleRemoveObjectQuery, this, obj, true),
        "LibproxProximity::handleRemoveObjectQuery"
    );

    // Update min query angle, and update remote queries if necessary
    if (sa == mMinObjectQueryAngle || max_count == mMaxMaxCount) {
        PROXLOG(debug,"Query removal initiated server query request.");
        SolidAngle minangle(SolidAngle::Max);
        for(ObjectQueryAngleMap::iterator it = mObjectQueryAngles.begin(); it != mObjectQueryAngles.end(); it++)
            if (it->second < minangle) minangle = it->second;
        uint32 maxcount = 1;
        for(ObjectQueryMaxCountMap::iterator it = mObjectQueryMaxCounts.begin(); it != mObjectQueryMaxCounts.end(); it++)
            if (it->second == ObjectProxSimulationTraits::InfiniteResults || (maxcount > ObjectProxSimulationTraits::InfiniteResults && it->second > maxcount))
                maxcount = it->second;

        // NOTE: Even if this condition is satisfied, we could only be increasing
        // the minimum angle, so we don't *strictly* need to update the query.
        // Some buffer timing might be in order here to avoid excessive updates
        // while still getting the benefit from reducing the query angle.
        if (minangle != mMinObjectQueryAngle || maxcount != mMaxMaxCount) {
            mMinObjectQueryAngle = minangle;
            mMaxMaxCount = maxcount;
            updateAggregateQuery();
        }
    }
}



int32 LibproxProximity::objectQueries() const {
    return mObjectQueries[OBJECT_CLASS_STATIC].size();
}

int32 LibproxProximity::serverQueries() const {
    return mServerQueries[OBJECT_CLASS_STATIC].size();
}

void LibproxProximity::poll() {
    // Update server-to-server angles if necessary
    sendQueryRequests();

    // Get and ship server results
    std::deque<Message*> server_results_copy;
    mServerResults.swap(server_results_copy);
    mServerResultsToSend.insert(mServerResultsToSend.end(), server_results_copy.begin(), server_results_copy.end());

    bool server_sent = true;
    while(server_sent && !mServerResultsToSend.empty()) {
        Message* msg_front = mServerResultsToSend.front();
        server_sent = sendServerMessage(msg_front);
        if (server_sent)
            mServerResultsToSend.pop_front();
    }

    // Get and ship object results
    std::deque<Sirikata::Protocol::Object::ObjectMessage*> object_results_copy;
    mObjectResults.swap(object_results_copy);
    mObjectResultsToSend.insert(mObjectResultsToSend.end(), object_results_copy.begin(), object_results_copy.end());

    while(!mObjectResultsToSend.empty()) {
        Sirikata::Protocol::Object::ObjectMessage* msg_front = mObjectResultsToSend.front();
        sendObjectResult(msg_front);
        delete msg_front;
        mObjectResultsToSend.pop_front();
    }
}



void LibproxProximity::queryHasEvents(Query* query) {
    InstanceMethodNotReentrant nr(mQueryHasEventsNotRentrant);

    if (
        query->handler() == mServerQueryHandler[OBJECT_CLASS_STATIC].handler ||
        query->handler() == mServerQueryHandler[OBJECT_CLASS_DYNAMIC].handler
    )
        generateServerQueryEvents(query);
    else
        generateObjectQueryEvents(query);
}


// Note: LocationServiceListener interface is only used in order to get updates on objects which have
// registered queries, allowing us to update those queries as appropriate.  All updating of objects
// in the prox data structure happens via the LocationServiceCache
void LibproxProximity::localObjectRemoved(const UUID& uuid, bool agg) {
    LibproxProximityBase::localObjectRemoved(uuid, agg);

    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::removeStaticObjectTimeout, this, ObjectReference(uuid)),
        "LibproxProximity::removeStaticObjectTimeout"
    );
}
void LibproxProximity::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    updateQuery(uuid, newval, mLocService->bounds(uuid).fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults);
    if (mSeparateDynamicObjects)
        checkObjectClass(true, uuid, newval);
}
void LibproxProximity::localBoundsUpdated(const UUID& uuid, bool agg, const AggregateBoundingInfo& newval) {
    LibproxProximityBase::localBoundsUpdated(uuid, agg, newval);
    updateQuery(uuid, mLocService->location(uuid), newval.fullBounds(), NoUpdateSolidAngle, NoUpdateMaxResults);
}
void LibproxProximity::replicaObjectRemoved(const UUID& uuid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::removeStaticObjectTimeout, this, ObjectReference(uuid)),
        "LibproxProximity::removeStaticObjectTimeout"
    );
}
void LibproxProximity::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    if (mSeparateDynamicObjects)
        checkObjectClass(false, uuid, newval);
}


// PROX Thread: Everything after this should only be called from within the prox thread.

void LibproxProximity::tickQueryHandler(ProxQueryHandlerData qh[NUM_OBJECT_CLASSES]) {
    // Not really any better place to do this. We'll call this more frequently
    // than necessary by putting it here, but hopefully it doesn't matter since
    // most of the time nothing will be done.
    processExpiredStaticObjectTimeouts();

    // We need to actually swap any objects that the previous step
    // found. However, we need to be careful because just performing
    // the addObject() and removeObject() can result in incorrect
    // results: because each class is ticked separately we could do
    // the addition and removal, then tick the handlers in the wrong
    // order such that querier q which already has object o in the
    // result set gets messages [add o, remove o] when they really
    // needed to get [remove o, add o].
    //
    // To handle this, we just do all the removals, perform a tick,
    // then do all the additions. This forces this step to only
    // generate removals, then lets the next tick generate the
    // additions.

    Time simT = mContext->simTime();
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (qh[i].handler != NULL) {
            for(ObjectIDSet::iterator it = qh[i].removals.begin(); it != qh[i].removals.end(); it++)
                qh[i].handler->removeObject(*it, true);
            qh[i].removals.clear();

            qh[i].handler->tick(simT);

            for(ObjectIDSet::iterator it = qh[i].additions.begin(); it != qh[i].additions.end(); it++)
                qh[i].handler->addObject(*it);
            qh[i].additions.clear();
        }
    }

    // We wait until the first full iteration is done for queries so we can
    // coalesce their initial results, skipping intermediate refinement. Now's
    // the time to mark them as having completed their first iteration and
    // performing the coalescing.

    // copied for safe iteration
    FirstIterationObjectSet copied_first_its = mObjectQueriesFirstIteration;
    for(FirstIterationObjectSet::const_iterator it = copied_first_its.begin(); it != copied_first_its.end(); it++)
        generateObjectQueryEvents(*it, true);
    mObjectQueriesFirstIteration.clear();
}

void LibproxProximity::rebuildHandlerType(ProxQueryHandlerData* handler, ObjectClass objtype) {
    if (handler[objtype].handler != NULL)
        handler[objtype].handler->rebuild();
}

void LibproxProximity::rebuildHandler(ObjectClass objtype) {
    rebuildHandlerType(mServerQueryHandler, objtype);
    rebuildHandlerType(mObjectQueryHandler, objtype);
}

void LibproxProximity::recomputeAggregateQueryBounds() {
    Time t = mContext->simTime();
    AggregateBoundingInfo new_bnds;

    for(ObjectQueryMap::iterator it = mObjectQueries[OBJECT_CLASS_STATIC].begin(); it != mObjectQueries[OBJECT_CLASS_STATIC].end(); it++) {
        // We know that the query registration makes these individual objects,
        // gives 0 size bounds and puts the object size in maxSize, so we can
        // ignore query->region() and give 0 for the center bounds radius.
        AggregateBoundingInfo querier_bnds(it->second->position(t), 0, it->second->maxSize());
        new_bnds.mergeIn(querier_bnds);
    }

    // Since our merging process isn't perfect, we might actually end up with
    // the entire server bounding box being better.
    // FIXME avoid computing this so much, could be event driven from CSeg
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = aggregateBBoxes(bboxes);
    BoundingSphere3f server_bounds = bbox.toBoundingSphere();
    if (server_bounds.radius() < new_bnds.centerBoundsRadius) {
        new_bnds.centerOffset = server_bounds.center();
        new_bnds.centerBoundsRadius = server_bounds.radius();
    }

    if (new_bnds == mLastAggregateQuerierBounds)
        return;

    PROXLOG(detailed, "Updated aggregate querier bounds info " << new_bnds);
    mLastAggregateQuerierBounds = new_bnds;
    updateAggregateQuery();
}

// Command handlers
void LibproxProximity::commandProperties(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    // Properties/settings
    result.put("name", "libprox");
    result.put("settings.handlers", mNumQueryHandlers * 2);
    result.put("settings.dynamic_separate", mSeparateDynamicObjects);
    if (mSeparateDynamicObjects)
        result.put("settings.static_heuristic", mMoveToStaticDelay.toString());

    // Current state. Split into two high level parts, objects and servers, and
    // further split by properties of connected objects/servers and queries
    // by them.

    // Properties of objects
    // We don't get this info from loc, we just figure it out based on what the
    // query processors report: server queries only have local objects, object
    // queries have both
    int32 server_query_objects = (mNumQueryHandlers == 2 ? (mServerQueryHandler[0].handler->numObjects() + mServerQueryHandler[1].handler->numObjects()) : mServerQueryHandler[0].handler->numObjects());
    int32 object_query_objects = (mNumQueryHandlers == 2 ? (mObjectQueryHandler[0].handler->numObjects() + mObjectQueryHandler[1].handler->numObjects()) : mObjectQueryHandler[0].handler->numObjects());
    result.put("objects.properties.local_count", server_query_objects);
    result.put("objects.properties.remote_count", object_query_objects - server_query_objects);
    result.put("objects.properties.count", object_query_objects);
    result.put("objects.properties.max_size", mMaxObject);

    // Properties of queries from objects
    result.put("queries.objects.count", mObjectQueries[0].size());
    result.put("queries.objects.min_solid_angle", mMinObjectQueryAngle.asFloat());
    result.put("queries.objects.max_max_count", mMaxMaxCount);
    if (mObjectDistance)
        result.put("queries.objects.distance", mDistanceQueryDistance);
    // Technically not thread safe, but these should be simple
    // read-only accesses.
    uint32 obj_messages = 0;
    for(ObjectProxStreamMap::iterator prox_stream_it = mObjectProxStreams.begin(); prox_stream_it != mObjectProxStreams.end(); prox_stream_it++)
        obj_messages += prox_stream_it->second->outstanding.size();
    result.put("queries.objects.messages", mObjectResults.size() + mObjectResultsToSend.size() + obj_messages);


    // Properties of servers
    result.put("servers.num_queried", numServersQueried());

    // Properties of queries from servers
    result.put("queries.servers.count", mServerQueries[0].size());
    if (mServerDistance)
        result.put("queries.servers.distance", mDistanceQueryDistance);
    result.put("queries.servers.messages", mServerResults.size() + mServerResultsToSend.size());

    cmdr->result(cmdid, result);
}

void LibproxProximity::commandListHandlers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i].handler != NULL) {
            String key = String("handlers.object.") + ObjectClassToString((ObjectClass)i) + ".";
            result.put(key + "name", String("object-queries.") + ObjectClassToString((ObjectClass)i) + "-objects");
            result.put(key + "queries", mObjectQueryHandler[i].handler->numQueries());
            result.put(key + "objects", mObjectQueryHandler[i].handler->numObjects());
            result.put(key + "nodes", mObjectQueryHandler[i].handler->numNodes());
        }
        if (mServerQueryHandler[i].handler != NULL) {
            String key = String("handlers.server.") + ObjectClassToString((ObjectClass)i) + ".";
            result.put(key + "name", String("server-queries.") + ObjectClassToString((ObjectClass)i) + "-objects");
            result.put(key + "queries", mServerQueryHandler[i].handler->numQueries());
            result.put(key + "objects", mServerQueryHandler[i].handler->numObjects());
            result.put(key + "nodes", mServerQueryHandler[i].handler->numNodes());
        }
    }
    cmdr->result(cmdid, result);
}

void LibproxProximity::commandListQueriers(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();
    // Organized as lists under queriers.type, each querier being a dict of query handlers -> stats
    result.put("queriers.object", Command::Object());
    result.put("queriers.oh", Command::Object());
    result.put("queriers.server", Command::Object());
    Command::Result& object_queriers = result.get("queriers.object");
    Command::Result& oh_queriers = result.get("queriers.oh");
    Command::Result& server_queriers = result.get("queriers.server");

    // Outer loops get our list of queriers
    for(ObjectQueryMap::iterator qit = mObjectQueries[OBJECT_CLASS_STATIC].begin(); qit != mObjectQueries[OBJECT_CLASS_STATIC].end(); qit++) {
        Command::Result data = Command::EmptyResult();
        for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
            if (mObjectQueryHandler[i].handler == NULL) continue;
            // Then we need to look up the per-object-class query for the querier
            ObjectQueryMap::iterator qcit = mObjectQueries[i].find(qit->first);
            if (qcit == mObjectQueries[i].end()) continue;

            String path = String("object-queries_") + ObjectClassToString((ObjectClass)i) + "-objects";
            data.put(path + ".results", qcit->second->numResults());
            data.put(path + ".size", qcit->second->size());
        }
        object_queriers.put(qit->first.toString(), data);
    }
    for(ServerQueryMap::iterator qit = mServerQueries[OBJECT_CLASS_STATIC].begin(); qit != mServerQueries[OBJECT_CLASS_STATIC].end(); qit++) {
        Command::Result data = Command::EmptyResult();
        for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
            if (mServerQueryHandler[i].handler == NULL) continue;
            ServerQueryMap::iterator qcit = mServerQueries[i].find(qit->first);
            if (qcit == mServerQueries[i].end()) continue;

            String path = String("server-queries_") + ObjectClassToString((ObjectClass)i) + "-objects";
            data.put(path + ".results", qcit->second->numResults());
            data.put(path + ".size", qcit->second->size());
        }
        server_queriers.put(boost::lexical_cast<String>(qit->first), data);
    }
    cmdr->result(cmdid, result);
}

bool LibproxProximity::parseHandlerName(const String& name, ProxQueryHandlerData** handlers_out, ObjectClass* class_out) {
    // Should be of the form xxx-queries.yyy-objects, containing only 1 .
    std::size_t dot_pos = name.find('.');
    if (dot_pos == String::npos || name.rfind('.') != dot_pos)
        return false;

    String handler_part = name.substr(0, dot_pos);
    if (handler_part == "server-queries")
        *handlers_out = mServerQueryHandler;
    else if (handler_part == "object-queries")
        *handlers_out = mObjectQueryHandler;
    else
        return false;

    String class_part = name.substr(dot_pos+1);
    if (class_part == "dynamic-objects")
        *class_out = OBJECT_CLASS_DYNAMIC;
    else if (class_part == "static-objects")
        *class_out = OBJECT_CLASS_STATIC;
    else
        return false;

    return true;
}

void LibproxProximity::commandForceRebuild(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    ProxQueryHandlerData* handlers = NULL;
    ObjectClass klass;
    if (!cmd.contains("handler") ||
        !parseHandlerName(cmd.getString("handler"), &handlers, &klass))
    {
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
        return;
    }

    rebuildHandlerType(handlers, klass);
    result.put("success", true);
    cmdr->result(cmdid, result);
}

void LibproxProximity::commandListNodes(const Command::Command& cmd, Command::Commander* cmdr, Command::CommandID cmdid) {
    Command::Result result = Command::EmptyResult();

    ProxQueryHandlerData* handlers = NULL;
    ObjectClass klass;
    if (!cmd.contains("handler") ||
        !parseHandlerName(cmd.getString("handler"), &handlers, &klass))
    {
        result.put("error", "Ill-formatted request: handler not specified or invalid.");
        cmdr->result(cmdid, result);
        return;
    }

    result.put( String("nodes"), Command::Array());
    Command::Array& nodes_ary = result.getArray("nodes");
    for(ProxQueryHandler::NodeIterator nit = handlers[klass].handler->nodesBegin(); nit != handlers[klass].handler->nodesEnd(); nit++) {
        nodes_ary.push_back( Command::Object() );
        nodes_ary.back().put("id", nit.id().toString());
        nodes_ary.back().put("parent", nit.parentId().toString());
        BoundingSphere3f bounds = nit.bounds(mContext->simTime());
        nodes_ary.back().put("bounds.center.x", bounds.center().x);
        nodes_ary.back().put("bounds.center.y", bounds.center().y);
        nodes_ary.back().put("bounds.center.z", bounds.center().z);
        nodes_ary.back().put("bounds.radius", bounds.radius());
        nodes_ary.back().put("cuts", nit.cuts());
    }

    cmdr->result(cmdid, result);
}


void LibproxProximity::generateServerQueryEvents(Query* query) {
    Time t = mContext->simTime();
    uint32 max_count = GetOptionValue<uint32>(PROX_MAX_PER_RESULT);

    assert(mInvertedServerQueries.find(query) != mInvertedServerQueries.end());
    ServerID sid = mInvertedServerQueries[query];
    SeqNoPtr seqNoPtr = getOrCreateSeqNoInfo(sid);

    QueryEventList evts;
    query->popEvents(evts);

    while(!evts.empty()) {
        Sirikata::Protocol::Prox::Container container;
        Sirikata::Protocol::Prox::IProximityResults contents = container.mutable_result();
        contents.set_t(t);
        uint32 count = 0;
        while(count < max_count && !evts.empty()) {
            const QueryEvent& evt = evts.front();
            Sirikata::Protocol::Prox::IProximityUpdate event_results = contents.add_update();
            // Each QueryEvent is made up of additions and
            // removals
            for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
                ObjectReference oobjid = evt.additions()[aidx].id();
                UUID objid = oobjid.getAsUUID();
                assert(mLocCache->tracking(oobjid));
                count++;

                mContext->mainStrand->post(
                    std::tr1::bind(&LibproxProximity::handleAddServerLocSubscription, this, sid, objid, seqNoPtr),
                    "LibproxProximity::handleAddServerLocSubscription"
                );

                Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
                addition.set_object( objid );

                uint64 seqNo = (*seqNoPtr)++;
                addition.set_seqno (seqNo);

                TimedMotionVector3f loc = mLocCache->location(oobjid);
                Sirikata::Protocol::ITimedMotionVector msg_loc = addition.mutable_location();
                msg_loc.set_t(loc.updateTime());
                msg_loc.set_position(loc.position());
                msg_loc.set_velocity(loc.velocity());

                TimedMotionQuaternion orient = mLocCache->orientation(oobjid);
                Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                msg_orient.set_t(orient.updateTime());
                msg_orient.set_position(orient.position());
                msg_orient.set_velocity(orient.velocity());

                Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
                AggregateBoundingInfo bnds = mLocCache->bounds(oobjid);
                msg_bounds.set_center_offset(bnds.centerOffset);
                msg_bounds.set_center_bounds_radius(bnds.centerBoundsRadius);
                msg_bounds.set_max_object_size(bnds.maxObjectRadius);

                String mesh = mLocCache->mesh(oobjid).toString();
                if (mesh.size() > 0)
                    addition.set_mesh(mesh);
                const String& phy = mLocCache->physics(oobjid);
                if (phy.size() > 0)
                    addition.set_physics(phy);
                String qd = mLocCache->queryData(oobjid);
                if (qd.size() > 0)
                    addition.set_query_data(qd);
            }
            for(uint32 pidx = 0; pidx < evt.reparents().size(); pidx++) {
                Sirikata::Protocol::Prox::INodeReparent reparent = event_results.add_reparent();
                reparent.set_object( evt.reparents()[pidx].id().getAsUUID() );
                uint64 seqNo = (*seqNoPtr)++;
                reparent.set_seqno (seqNo);
                reparent.set_old_parent(evt.reparents()[pidx].oldParent().getAsUUID());
                reparent.set_new_parent(evt.reparents()[pidx].newParent().getAsUUID());
                reparent.set_type(
                    (evt.reparents()[pidx].type() == QueryEvent::Normal) ?
                    Sirikata::Protocol::Prox::NodeReparent::Object :
                    Sirikata::Protocol::Prox::NodeReparent::Aggregate
                );
            }
            for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
                ObjectReference oobjid = evt.removals()[ridx].id();
                UUID objid = oobjid.getAsUUID();
                count++;

                mContext->mainStrand->post(
                    std::tr1::bind(&LibproxProximity::handleRemoveServerLocSubscription, this, sid, objid),
                    "LibproxProximity::handleRemoveServerLocSubscription"
                );

                Sirikata::Protocol::Prox::IObjectRemoval removal = event_results.add_removal();
                removal.set_object(objid);
                uint64 seqNo = (*seqNoPtr)++;
                removal.set_seqno (seqNo);
                removal.set_type(
                    (evt.removals()[ridx].permanent() == QueryEvent::Permanent)
                    ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                    : Sirikata::Protocol::Prox::ObjectRemoval::Transient
                );
            }

            evts.pop_front();
        }

        //PROXLOG(insane,"Reporting " << contents.addition_size() << " additions, " << contents.removal_size() << " removals to server " << sid);

        Message* msg = new Message(
            mContext->id(),
            SERVER_PORT_PROX,
            sid,
            SERVER_PORT_PROX,
            serializePBJMessage(container)
        );
        mServerResults.push(msg);
    }
}

void LibproxProximity::generateObjectQueryEvents(Query* query, bool do_first) {
    // If we're waiting for the first iteration to finish, we ignore the
    // notification, waiting until we get out of the first tick to manually
    // trigger updates.
    bool is_first = (mObjectQueriesFirstIteration.find(query) != mObjectQueriesFirstIteration.end());
    if (!do_first && is_first) return;

    uint32 max_count = GetOptionValue<uint32>(PROX_MAX_PER_RESULT);

    assert(mInvertedObjectQueries.find(query) != mInvertedObjectQueries.end());
    UUID query_id = mInvertedObjectQueries[query];
    SeqNoPtr seqNoPtr = getSeqNoInfo(query_id);

    QueryEventList evts;
    query->popEvents(evts);

    if (!seqNoPtr) {
        PROXLOG(detailed, "Can't find seqno information due to disconnect, ignoring remaining proximity results for " << query_id << ".");
        return;
    }

    if (is_first) {
        coalesceEvents(evts, 10);
        mObjectQueriesFirstIteration.erase(query);
    }

    while(!evts.empty()) {
        Sirikata::Protocol::Prox::ProximityResults prox_results;
        prox_results.set_t(mContext->simTime());

        uint32 count = 0;
        while(count < max_count && !evts.empty()) {
            const QueryEvent& evt = evts.front();
            Sirikata::Protocol::Prox::IProximityUpdate event_results = prox_results.add_update();

            for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
                ObjectReference oobjid = evt.additions()[aidx].id();
                UUID objid = oobjid.getAsUUID();
                assert(mLocCache->tracking(oobjid));
                count++;

                mContext->mainStrand->post(
                    std::tr1::bind(&LibproxProximity::handleAddObjectLocSubscription, this, query_id, objid),
                    "LibproxProximity::handleAddObjectLocSubscription"
                );

                Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
                addition.set_object( objid );

                //query_id contains the uuid of the object that is receiving
                //the proximity message that obj_id has been added.
                uint64 seqNo = (*seqNoPtr)++;
                addition.set_seqno (seqNo);

                if (mLocCache->isAggregate(oobjid)) {
                    addition.set_type(Sirikata::Protocol::Prox::ObjectAddition::Aggregate);
                }
                else {
                    addition.set_type(Sirikata::Protocol::Prox::ObjectAddition::Object);
                }

                Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
                TimedMotionVector3f loc = mLocCache->location(oobjid);
                motion.set_t(loc.updateTime());
                motion.set_position(loc.position());
                motion.set_velocity(loc.velocity());

                TimedMotionQuaternion orient = mLocCache->orientation(oobjid);
                Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                msg_orient.set_t(orient.updateTime());
                msg_orient.set_position(orient.position());
                msg_orient.set_velocity(orient.velocity());

                Sirikata::Protocol::IAggregateBoundingInfo msg_bounds = addition.mutable_aggregate_bounds();
                AggregateBoundingInfo bnds = mLocCache->bounds(oobjid);
                msg_bounds.set_center_offset(bnds.centerOffset);
                msg_bounds.set_center_bounds_radius(bnds.centerBoundsRadius);
                msg_bounds.set_max_object_size(bnds.maxObjectRadius);

                String mesh = mLocCache->mesh(oobjid).toString();
                if (mesh.size() > 0)
                    addition.set_mesh(mesh);
                const String& phy = mLocCache->physics(oobjid);
                if (phy.size() > 0)
                    addition.set_physics(phy);
                // Do not include query_data for results going to objects
            }
            for(uint32 pidx = 0; pidx < evt.reparents().size(); pidx++) {
                Sirikata::Protocol::Prox::INodeReparent reparent = event_results.add_reparent();
                reparent.set_object( evt.reparents()[pidx].id().getAsUUID() );
                uint64 seqNo = (*seqNoPtr)++;
                reparent.set_seqno (seqNo);
                reparent.set_old_parent(evt.reparents()[pidx].oldParent().getAsUUID());
                reparent.set_new_parent(evt.reparents()[pidx].newParent().getAsUUID());
                reparent.set_type(
                    (evt.reparents()[pidx].type() == QueryEvent::Normal) ?
                    Sirikata::Protocol::Prox::NodeReparent::Object :
                    Sirikata::Protocol::Prox::NodeReparent::Aggregate
                );
            }
            for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
                ObjectReference oobjid = evt.removals()[ridx].id();
                UUID objid = oobjid.getAsUUID();
                count++;
                // Clear out seqno and let main strand remove loc
                // subcription
                mContext->mainStrand->post(
                    std::tr1::bind(&LibproxProximity::handleRemoveObjectLocSubscription, this, query_id, objid),
                    "LibproxProximity::handleRemoveObjectLocSubscription"
                );

                Sirikata::Protocol::Prox::IObjectRemoval removal = event_results.add_removal();
                removal.set_object( objid );
                uint64 seqNo = (*seqNoPtr)++;
                removal.set_seqno (seqNo);
                removal.set_type(
                    (evt.removals()[ridx].permanent() == QueryEvent::Permanent)
                    ? Sirikata::Protocol::Prox::ObjectRemoval::Permanent
                    : Sirikata::Protocol::Prox::ObjectRemoval::Transient
                );
            }
            evts.pop_front();
        }

        Sirikata::Protocol::Object::ObjectMessage* obj_msg = createObjectMessage(
            mContext->id(),
            UUID::null(), OBJECT_PORT_PROXIMITY,
            query_id, OBJECT_PORT_PROXIMITY,
            serializePBJMessage(prox_results)
        );
        mObjectResults.push(obj_msg);
    }
}


SeqNoPtr LibproxProximity::getOrCreateSeqNoInfo(const ServerID server_id)
{
    // server_id == querier
    ServerSeqNoInfoMap::iterator proxSeqNoIt = mServerSeqNos.find(server_id);
    if (proxSeqNoIt == mServerSeqNos.end())
        proxSeqNoIt = mServerSeqNos.insert( ServerSeqNoInfoMap::value_type(server_id, SeqNoPtr(new SeqNo())) ).first;
    return proxSeqNoIt->second;
}

void LibproxProximity::eraseSeqNoInfo(const ServerID server_id)
{
    // server_id == querier
    ServerSeqNoInfoMap::iterator proxSeqNoIt = mServerSeqNos.find(server_id);
    if (proxSeqNoIt == mServerSeqNos.end()) return;
    mServerSeqNos.erase(proxSeqNoIt);
}


SeqNoPtr LibproxProximity::getSeqNoInfo(const UUID& obj_id)
{
    // obj_id == querier
    ObjectSeqNoInfoMap::iterator proxSeqNoIt = mObjectSeqNos.find(obj_id);
    // If there is a forceful disconnection, we can end up having erased the
    // seqno ptr but still have some results to process. A null seqno ptr
    // indicates we should ignore the results.
    if (proxSeqNoIt == mObjectSeqNos.end())
        return SeqNoPtr();
    assert(proxSeqNoIt != mObjectSeqNos.end());
    return proxSeqNoIt->second;
}

void LibproxProximity::eraseSeqNoInfo(const UUID& obj_id)
{
    // obj_id == querier
    ObjectSeqNoInfoMap::iterator proxSeqNoIt = mObjectSeqNos.find(obj_id);
    if (proxSeqNoIt == mObjectSeqNos.end()) return;
    mObjectSeqNos.erase(proxSeqNoIt);
}


void LibproxProximity::handleUpdateServerQuery(const ServerID& server, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, const uint32 max_results) {
    TimedMotionVector3f adjusted_loc(loc.updateTime(), MotionVector3f(loc.position() + bounds.center(), loc.velocity()));
    BoundingSphere3f region(Vector3f(0,0,0), 0);
    float ms = bounds.radius();

    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mServerQueryHandler[i].handler == NULL) continue;

        ServerQueryMap::iterator it = mServerQueries[i].find(server);
        if (it == mServerQueries[i].end()) {
            PROXLOG(debug,"Add server query from " << server << ", min angle " << angle.asFloat() << ", object class " << ObjectClassToString((ObjectClass)i) << " (loc: " << adjusted_loc.position() << ", region: " << region << ", max size: " << ms << ")");

            // FIXME also support custom queries
            Query* q = mServerDistance ?
                mServerQueryHandler[i].handler->registerQuery(adjusted_loc, region, ms, SolidAngle::Min, mDistanceQueryDistance) :
                mServerQueryHandler[i].handler->registerQuery(adjusted_loc, region, ms, angle) ;
            if (max_results != NoUpdateMaxResults && max_results > 0)
                q->maxResults(max_results);
            mServerQueries[i][server] = q;
            mInvertedServerQueries[q] = server;
            q->setEventListener(this);
        }
        else {
            PROXLOG(debug,"Update server query from " << server << ", min angle " << angle.asFloat() << ", object class " << ObjectClassToString((ObjectClass)i) << " (loc: " << adjusted_loc.position() << ", region: " << region << ", max size: " << ms << ")");

            Query* q = it->second;
            q->position(adjusted_loc);
            q->region( region );
            q->maxSize( ms );
            q->angle(angle);
            if (max_results != NoUpdateMaxResults && max_results > 0)
                q->maxResults(max_results);
        }
    }
}

void LibproxProximity::handleRemoveServerQuery(const ServerID& server) {
    PROXLOG(debug,"Remove server query from " << server);

    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mServerQueryHandler[i].handler == NULL) continue;

        ServerQueryMap::iterator it = mServerQueries[i].find(server);
        if (it == mServerQueries[i].end()) continue;

        Query* q = it->second;
        mServerQueries[i].erase(it);
        mInvertedServerQueries.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }

    // Clear out sequence numbers
    eraseSeqNoInfo(server);

    mContext->mainStrand->post(
        std::tr1::bind(&LibproxProximity::handleRemoveAllServerLocSubscription, this, server),
        "LibproxProximity::handleRemoveAllServerLocSubscription"
    );
}


void LibproxProximity::handleConnectedServer(ServerID sid) {
    // Sometimes we may get forcefully disconnected from a server. To
    // reestablish our previous setup, if that server appears in our queried
    // set (we were told it was relevant to us by some higher level pinto
    // service), we mark it as needing another update. In the case that we
    // are just getting an initial connection, this shouldn't change anything
    // since it would already be markedas needing an update if we had been told
    // by pinto that it needed it.
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    if (mServersQueried.find(sid) != mServersQueried.end())
        mNeedServerQueryUpdate.insert(sid);
}

void LibproxProximity::handleDisconnectedServer(ServerID sid) {
    // When we lose a connection, we need to clear out everything related to
    // that server.
    PROXLOG(debug, "Handling unexpected disconnection from " << sid << " by clearing out proximity state.");

    // Clear out remote server's query to us
    handleRemoveServerQuery(sid);

    // And our query (and associated state) to them
    // They clear out our query (as we do for them above), but we need to also
    // remove the state we maintain locally, i.e. replicated objects.
    ObjectSetPtr objects = mServerQueryResults[sid];
    if (!objects) return;
    mServerQueryResults.erase(sid);
    Time t = mContext->simTime();
    for(ObjectSet::const_iterator it = objects->begin(); it != objects->end(); it++)
        mLocService->removeReplicaObject(t, *it);
}

void LibproxProximity::handleUpdateObjectQuery(const UUID& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle, uint32 max_results, SeqNoPtr seqno) {
    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();

    // In some cases (new queries) we only want to do any work here,
    // including reporting the update, if the user explicitly made a
    // change to query parameters since this will get called for all
    // objects triggering movement.
    bool explicit_query_params_update = ((angle != NoUpdateSolidAngle) || (max_results != NoUpdateMaxResults));

    if (mObjectSeqNos.find(object) == mObjectSeqNos.end()) {
        // If there's no existing query, so this was just because of a
        // location update -- don't record a query since it wouldn't
        // do anything anyway.
        if (!explicit_query_params_update) return;

        mObjectSeqNos.insert( ObjectSeqNoInfoMap::value_type(object, seqno) );
    }

    // Log, but only if this isn't just due to object movement
    if (explicit_query_params_update)
        PROXLOG(detailed,"Update object query from " << object.toString() << ", min angle " << angle.asFloat() << ", max results " << max_results);


    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i].handler == NULL) continue;

        ObjectQueryMap::iterator it = mObjectQueries[i].find(object);

        if (it == mObjectQueries[i].end()) {
            // We only add if we actually have all the necessary info, most importantly a real minimum angle.
            // This is necessary because we get this update for all location updates, even those for objects
            // which don't have subscriptions.
            if (angle != NoUpdateSolidAngle) {
                // FIXME also support custom queries
                Query* q = mObjectDistance ?
                    mObjectQueryHandler[i].handler->registerQuery(loc, region, ms, SolidAngle::Min, mDistanceQueryDistance) :
                    mObjectQueryHandler[i].handler->registerQuery(loc, region, ms, angle);
                if (max_results != NoUpdateMaxResults && max_results > 0)
                    q->maxResults(max_results);
                mObjectQueries[i][object] = q;
                mInvertedObjectQueries[q] = object;
                mObjectQueriesFirstIteration.insert(q);
                q->setEventListener(this);
            }
        }
        else {
            Query* query = it->second;
            query->position(loc);
            query->region( region );
            query->maxSize( ms );
            if (angle != NoUpdateSolidAngle)
                query->angle(angle);
            if (max_results != NoUpdateMaxResults && max_results > 0)
                query->maxResults(max_results);
        }
    }
}

void LibproxProximity::handleRemoveObjectQuery(const UUID& object, bool notify_main_thread) {
    // Clear out queries
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i].handler == NULL) continue;

        ObjectQueryMap::iterator it = mObjectQueries[i].find(object);
        if (it == mObjectQueries[i].end()) continue;

        Query* q = it->second;
        mObjectQueries[i].erase(it);
        mInvertedObjectQueries.erase(q);
        mObjectQueriesFirstIteration.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }

    // Clear out sequence numbers
    eraseSeqNoInfo(object);

    // Optionally let the main thread know to clear its communication state
    if (notify_main_thread) {
        // There's no corresponding removeAllSeqNoPtr because we
        // should have erased it above.
        mContext->mainStrand->post(
            std::tr1::bind(&LibproxProximity::handleRemoveAllObjectLocSubscription, this, object),
            "LibproxProximity::handleRemoveAllObjectLocSubscription"
        );
    }
}

void LibproxProximity::handleDisconnectedObject(const UUID& object) {
    // Clear out query state if it exists
    handleRemoveObjectQuery(object, false);
}

bool LibproxProximity::handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const ObjectReference& obj_id, bool is_local, bool is_aggregate, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize) {
    // We just need to decide whether the query handler should handle
    // the object. We need to consider local vs. replica and static
    // vs. dynamic.  All must 'vote' for handling the object for us to
    // say it should be handled, so as soon as we find a negative
    // response we can return false.

    // First classify by local vs. replica. Only say no on a local
    // handler looking at a replica.
    if (!is_local && !is_global_handler) return false;

    // If we're not doing the static/dynamic split, then this is a non-issue
    if (!mSeparateDynamicObjects) return true;

    // If we are splitting them, check velocity against is_static_handler. The
    // value here as arbitrary, just meant to indicate such small movement that
    // the object is effectively
    bool is_static = velocityIsStatic(pos.velocity());
    if ((is_static && is_static_handler) ||
        (!is_static && !is_static_handler))
        return true;
    else
        return false;
}

void LibproxProximity::handleCheckObjectClassForHandlers(const ObjectReference& objid, bool is_static, ProxQueryHandlerData handlers[NUM_OBJECT_CLASSES]) {
    if ( (is_static && handlers[OBJECT_CLASS_STATIC].handler->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_DYNAMIC].handler->containsObject(objid)) )
        return;

    // Validate that the other handler has the object.
    assert(
        (is_static && handlers[OBJECT_CLASS_DYNAMIC].handler->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_STATIC].handler->containsObject(objid))
    );

    // If it wasn't in the right place, switch it.
    int swap_out = is_static ? OBJECT_CLASS_DYNAMIC : OBJECT_CLASS_STATIC;
    int swap_in = is_static ? OBJECT_CLASS_STATIC : OBJECT_CLASS_DYNAMIC;
    PROXLOG(debug, "Swapping " << objid.toString() << " from " << ObjectClassToString((ObjectClass)swap_out) << " to " << ObjectClassToString((ObjectClass)swap_in));
    handlers[swap_out].removals.insert(objid);
    handlers[swap_in].additions.insert(objid);
}

void LibproxProximity::trySwapHandlers(bool is_local, const ObjectReference& objid, bool is_static) {
    handleCheckObjectClassForHandlers(objid, is_static, mObjectQueryHandler);
    if (is_local)
        handleCheckObjectClassForHandlers(objid, is_static, mServerQueryHandler);
}

} // namespace Sirikata
