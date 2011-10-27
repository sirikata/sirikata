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

#include <sirikata/space/QueryHandlerFactory.hpp>

#include "Protocol_Prox.pbj.hpp"
#include "Protocol_ServerProx.pbj.hpp"

#include <sirikata/core/network/IOServiceFactory.hpp>

#include <sirikata/space/AggregateManager.hpp>

// Property tree for old API for queries
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

namespace Sirikata {

static SolidAngle NoUpdateSolidAngle = SolidAngle(0.f);
// Note that this is different from sentinals indicating infinite
// results. Instead, this indicates that we shouldn't even request the update,
// leaving it as is, because we don't actually have a new value.
static uint32 NoUpdateMaxResults = ((uint32)INT_MAX)+1;

static BoundingBox3f aggregateBBoxes(const BoundingBoxList& bboxes) {
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);
    return bbox;
}

static bool velocityIsStatic(const Vector3f& vel) {
    // These values are arbitrary, just meant to indicate that the object is,
    // for practical purposes, not moving.
    return (
        vel.x < .05f &&
        vel.y < .05f &&
        vel.z < .05f
    );
}

namespace {

bool parseQueryRequest(const String& query, SolidAngle* qangle_out, uint32* max_results_out) {
    if (query.empty())
        return false;

    using namespace boost::property_tree;
    ptree pt;
    try {
        std::stringstream data_json(query);
        read_json(data_json, pt);
    }
    catch(json_parser::json_parser_error exc) {
        return false;
    }


    if (pt.find("angle") != pt.not_found())
        *qangle_out = SolidAngle( pt.get<float32>("angle") );
    else
        *qangle_out = SolidAngle::Max;

    if (pt.find("max_results") != pt.not_found())
        *max_results_out = pt.get<uint32>("max_results");
    else
        *max_results_out = 0;

    return true;
}

}

const String& LibproxProximity::ObjectClassToString(ObjectClass c) {
    static String static_ = "static";
    static String dynamic_ = "dynamic";
    static String unknown_ = "unknown";

    switch(c) {
      case OBJECT_CLASS_STATIC: return static_; break;
      case OBJECT_CLASS_DYNAMIC: return dynamic_; break;
      default: return unknown_; break;
    }
}

LibproxProximity::LibproxProximity(SpaceContext* ctx, LocationService* locservice, CoordinateSegmentation* cseg, SpaceNetwork* net, AggregateManager* aggmgr)
 : LibproxProximityBase(ctx, locservice, cseg, net, aggmgr),
   mServerQuerier(NULL),
   mDistanceQueryDistance(0.f),
   mMaxObject(0.0f),
   mMinObjectQueryAngle(SolidAngle::Max),
   mMaxMaxCount(1),
   mServerQueries(),
   mServerDistance(false),
   mServerHandlerPoller(mProxStrand, std::tr1::bind(&LibproxProximity::tickQueryHandler, this, mServerQueryHandler), Duration::milliseconds((int64)100)),
   mObjectQueries(),
   mObjectDistance(false),
   mObjectHandlerPoller(mProxStrand, std::tr1::bind(&LibproxProximity::tickQueryHandler, this, mObjectQueryHandler), Duration::milliseconds((int64)100)),
   mStaticRebuilderPoller(mProxStrand, std::tr1::bind(&LibproxProximity::rebuildHandler, this, OBJECT_CLASS_STATIC), Duration::seconds(3600.f)),
   mDynamicRebuilderPoller(mProxStrand, std::tr1::bind(&LibproxProximity::rebuildHandler, this, OBJECT_CLASS_DYNAMIC), Duration::seconds(3600.f))
{
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    using std::tr1::placeholders::_4;
    using std::tr1::placeholders::_5;

    // Server Querier (discover other servers)
    String pinto_type = GetOptionValue<String>(OPT_PINTO);
    String pinto_options = GetOptionValue<String>(OPT_PINTO_OPTIONS);
    mServerQuerier = PintoServerQuerierFactory::getSingleton().getConstructor(pinto_type)(mContext, pinto_options);
    mServerQuerier->addListener(this);

    // Deal with static/dynamic split
    mSeparateDynamicObjects = GetOptionValue<bool>(OPT_PROX_SPLIT_DYNAMIC);
    mNumQueryHandlers = (mSeparateDynamicObjects ? 2 : 1);

    // Generic query parameters
    mDistanceQueryDistance = GetOptionValue<float32>(OPT_PROX_QUERY_RANGE);

    // Server Queries
    String server_handler_type = GetOptionValue<String>(OPT_PROX_SERVER_QUERY_HANDLER_TYPE);
    String server_handler_options = GetOptionValue<String>(OPT_PROX_SERVER_QUERY_HANDLER_OPTIONS);
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mServerQueryHandler[i] = NULL;
            continue;
        }
        mServerQueryHandler[i] = QueryHandlerFactory<ObjectProxSimulationTraits>(server_handler_type, server_handler_options);
        mServerQueryHandler[i]->setAggregateListener(this); // *Must* be before handler->initialize
        bool server_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mServerQueryHandler[i]->initialize(
            mLocCache, mLocCache, server_static_objects,
            std::tr1::bind(&LibproxProximity::handlerShouldHandleObject, this, server_static_objects, false, _1, _2, _3, _4, _5)
        );
    }
    if (server_handler_type == "dist" || server_handler_type == "rtreedist") mServerDistance = true;

    // Object Queries
    String object_handler_type = GetOptionValue<String>(OPT_PROX_OBJECT_QUERY_HANDLER_TYPE);
    String object_handler_options = GetOptionValue<String>(OPT_PROX_OBJECT_QUERY_HANDLER_OPTIONS);
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (i >= mNumQueryHandlers) {
            mObjectQueryHandler[i] = NULL;
            continue;
        }
        mObjectQueryHandler[i] = QueryHandlerFactory<ObjectProxSimulationTraits>(object_handler_type, object_handler_options);
        mObjectQueryHandler[i]->setAggregateListener(this); // *Must* be before handler->initialize
        bool object_static_objects = (mSeparateDynamicObjects && i == OBJECT_CLASS_STATIC);
        mObjectQueryHandler[i]->initialize(
            mLocCache, mLocCache, object_static_objects,
            std::tr1::bind(&LibproxProximity::handlerShouldHandleObject, this, object_static_objects, true, _1, _2, _3, _4, _5)
        );
    }
    if (object_handler_type == "dist" || object_handler_type == "rtreedist") mObjectDistance = true;
}

LibproxProximity::~LibproxProximity() {
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        delete mObjectQueryHandler[i];
        delete mServerQueryHandler[i];
    }

    delete mServerQuerier;
}


// MAIN Thread Methods: The following should only be called from the main thread.

void LibproxProximity::start() {
    Proximity::start();

    // Always initialize with CSeg's current size
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = aggregateBBoxes(bboxes);
    mServerQuerier->updateRegion(bbox);

    mContext->add(&mServerHandlerPoller);
    mContext->add(&mObjectHandlerPoller);
    mContext->add(&mStaticRebuilderPoller);
    mContext->add(&mDynamicRebuilderPoller);
}


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
        std::tr1::bind(&LibproxProximity::handleDisconnectedObject, this, session->id().getAsUUID())
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

    // Need to support old style (angle + max count) and new style.
    if (prox_update.has_query_parameters()) {
        SolidAngle sa;
        uint32 max_results;
        if (parseQueryRequest(prox_update.query_parameters(), &sa, &max_results))
            updateQuery(objid, mLocService->location(objid), mLocService->bounds(objid), sa, max_results);
    }
    else {
        if (!prox_update.has_query_angle()) return;

        SolidAngle query_angle(prox_update.query_angle());
        uint32 query_max_results = (prox_update.has_query_max_count() ? prox_update.query_max_count() : NoUpdateMaxResults);
        updateQuery(objid, mLocService->location(objid), mLocService->bounds(objid), query_angle, query_max_results);
    }
}

// Setup all known servers for a server query update
void LibproxProximity::addAllServersForUpdate() {
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    for(ServerSet::const_iterator it = mServersQueried.begin(); it != mServersQueried.end(); it++)
        mNeedServerQueryUpdate.insert(*it);
}

void LibproxProximity::sendQueryRequests() {
    TimedMotionVector3f loc;

    // FIXME avoid computing this so much
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = aggregateBBoxes(bboxes);
    BoundingSphere3f bounds = bbox.toBoundingSphere();

    ServerSet sub_servers;
    {
        boost::lock_guard<boost::mutex> lck(mServerSetMutex);
        sub_servers.swap(mNeedServerQueryUpdate);
    }
    for(ServerSet::const_iterator it = sub_servers.begin(); it != sub_servers.end(); it++) {
        ServerID sid = *it;
        Sirikata::Protocol::Prox::Container container;
        Sirikata::Protocol::Prox::IServerQuery msg = container.mutable_query();
        msg.set_action(Sirikata::Protocol::Prox::ServerQuery::AddOrUpdate);
        Sirikata::Protocol::ITimedMotionVector msg_loc = msg.mutable_location();
        msg_loc.set_t(loc.updateTime());
        msg_loc.set_position(loc.position());
        msg_loc.set_position(loc.velocity());
        msg.set_bounds(bounds);
        msg.set_min_angle(mMinObjectQueryAngle.asFloat());
        msg.set_max_count(mMaxMaxCount);

        Message* smsg = new Message(
            mContext->id(),
            SERVER_PORT_PROX,
            sid,
            SERVER_PORT_PROX,
            serializePBJMessage(container)
        );
        bool sent = mProxServerMessageService->route(smsg);
        if (!sent) {
            delete smsg;
            {
                boost::lock_guard<boost::mutex> lck(mServerSetMutex);
                mNeedServerQueryUpdate.insert(sid);
            }
        }
    }
}

void LibproxProximity::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_PROX);

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
                mLocService->addReplicaObject(
                    t,
                    addition.object(),
                    TimedMotionVector3f( addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()) ),
                    TimedMotionQuaternion( addition.orientation().t(), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()) ),
                    addition.bounds(),
                    (addition.has_mesh() ? addition.mesh() : ""),
                    (addition.has_physics() ? addition.physics() : "")
                );
            }

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

// PintoServerQuerierListener Interface

void LibproxProximity::addRelevantServer(ServerID sid) {
    if (sid == mContext->id()) return;

    // Potentially invoked from PintoServerQuerier IO thread
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    mServersQueried.insert(sid);
    mNeedServerQueryUpdate.insert(sid);
}

void LibproxProximity::removeRelevantServer(ServerID sid) {
    if (sid == mContext->id()) return;

    // Potentially invoked from PintoServerQuerier IO thread
    boost::lock_guard<boost::mutex> lck(mServerSetMutex);
    mServersQueried.erase(sid);
}

void LibproxProximity::aggregateCreated(ProxAggregator* handler, const UUID& objid) {
    // On addition, an "aggregate" will have no children, i.e. its zero sized.

    mContext->mainStrand->post(
        std::tr1::bind(
            &LocationService::addLocalAggregateObject, mLocService,
            objid,
            TimedMotionVector3f(mContext->simTime(), MotionVector3f()),
            TimedMotionQuaternion(mContext->simTime(), MotionQuaternion()),
            BoundingSphere3f(),
            "",
            ""
        )
    );

    mAggregateManager->addAggregate(objid);
}


void LibproxProximity::updateAggregateLoc(const UUID& objid, const BoundingSphere3f& bnds) {

  if (mLocService->contains(objid)) {
    mLocService->updateLocalAggregateLocation(
        objid,
        TimedMotionVector3f(mContext->simTime(), MotionVector3f(bnds.center(), Vector3f(0,0,0)))
    );
    mLocService->updateLocalAggregateBounds(
        objid,
        BoundingSphere3f(bnds.center(), bnds.radius())
    );
  }
}

void LibproxProximity::aggregateChildAdded(ProxAggregator* handler, const UUID& objid, const UUID& child, const BoundingSphere3f& bnds) {
    if (!mLocService->contains(objid) || mLocService->bounds(objid) != bnds) {
      // Loc cares only about this chance to update state of aggregate
      mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximity::updateAggregateLoc, this,
            objid, bnds
        )
      );
    }

    mAggregateManager->addChild(objid, child);
}

void LibproxProximity::aggregateChildRemoved(ProxAggregator* handler, const UUID& objid, const UUID& child, const BoundingSphere3f& bnds) {
    if (!mLocService->contains(objid) || mLocService->bounds(objid) != bnds) {
      // Loc cares only about this chance to update state of aggregate
      mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximity::updateAggregateLoc, this,
            objid, bnds
        )
      );
    }

    mAggregateManager->removeChild(objid, child);
}

void LibproxProximity::aggregateBoundsUpdated(ProxAggregator* handler, const UUID& objid, const BoundingSphere3f& bnds) {
  if (!mLocService->contains(objid) || mLocService->bounds(objid) != bnds) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LibproxProximity::updateAggregateLoc, this,
            objid, bnds
        )
    );
  }

  if (mLocService->contains(objid) && mLocService->bounds(objid) != bnds)
    mAggregateManager->generateAggregateMesh(objid, Duration::seconds(300.0+rand()%300));
}

void LibproxProximity::aggregateDestroyed(ProxAggregator* handler, const UUID& objid) {
    mContext->mainStrand->post(
        std::tr1::bind(
            &LocationService::removeLocalAggregateObject, mLocService, objid
        )
    );
    mAggregateManager->removeAggregate(objid);
}

void LibproxProximity::aggregateObserved(ProxAggregator* handler, const UUID& objid, uint32 nobservers) {
  mAggregateManager->aggregateObserved(objid, nobservers);
}

void LibproxProximity::onSpaceNetworkConnected(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleConnectedServer, this, sid)
    );
}

void LibproxProximity::onSpaceNetworkDisconnected(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleDisconnectedServer, this, sid)
    );
}


void LibproxProximity::updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa, uint32 max_results) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleUpdateServerQuery, this, sid, loc, bounds, sa, max_results)
    );
}

void LibproxProximity::removeQuery(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleRemoveServerQuery, this, sid)
    );
}

void LibproxProximity::addQuery(UUID obj, SolidAngle sa, uint32 max_results) {
    updateQuery(obj, mLocService->location(obj), mLocService->bounds(obj), sa, max_results);
}

void LibproxProximity::addQuery(UUID obj, const String& params) {
    SolidAngle sa;
    uint32 max_results;
    if (parseQueryRequest(params, &sa, &max_results))
        updateQuery(obj, mLocService->location(obj), mLocService->bounds(obj), sa, max_results);
}

void LibproxProximity::updateQuery(UUID obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa, uint32 max_results) {
    SeqNoPtr obj_seqno = mContext->objectSessionManager()->getSession(ObjectReference(obj))->getSeqNoPtr();

    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleUpdateObjectQuery, this, obj, loc, bounds, sa, max_results, obj_seqno)
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

    if (update_remote_queries) {
        PROXLOG(debug,"Query addition initiated server query request.");
        addAllServersForUpdate();
        mServerQuerier->updateQuery(mMinObjectQueryAngle, mMaxMaxCount);
    }
}

void LibproxProximity::removeQuery(UUID obj) {
    // Update the main thread's record
    SolidAngle sa = mObjectQueryAngles[obj];
    mObjectQueryAngles.erase(obj);
    uint32 max_count = mObjectQueryMaxCounts[obj];
    mObjectQueryMaxCounts.erase(obj);

    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleRemoveObjectQuery, this, obj, true)
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
            addAllServersForUpdate();
            mServerQuerier->updateQuery(mMinObjectQueryAngle, mMaxMaxCount);
        }
    }
}

void LibproxProximity::updateObjectSize(const UUID& obj, float rad) {
    mObjectSizes[obj] = rad;

    if (rad > mMaxObject) {
        mMaxObject = rad;
        mServerQuerier->updateLargestObject(mMaxObject);
    }
}

void LibproxProximity::removeObjectSize(const UUID& obj) {
    ObjectSizeMap::iterator it = mObjectSizes.find(obj);
    assert(it != mObjectSizes.end());

    float32 old_val = it->second;
    mObjectSizes.erase(it);

    if (old_val == mMaxObject) {
        // We need to recompute mMaxObject since it may not be accurate anymore
        mMaxObject = 0.0f;
        for(ObjectSizeMap::iterator oit = mObjectSizes.begin(); oit != mObjectSizes.end(); oit++)
            mMaxObject = std::max(mMaxObject, oit->second);

        if (mMaxObject != old_val)
            mServerQuerier->updateLargestObject(mMaxObject);
    }
}

void LibproxProximity::checkObjectClass(bool is_local, const UUID& objid, const TimedMotionVector3f& newval) {
    mProxStrand->post(
        std::tr1::bind(&LibproxProximity::handleCheckObjectClass, this, is_local, objid, newval)
    );
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
        server_sent = mProxServerMessageService->route(msg_front);
        if (server_sent)
            mServerResultsToSend.pop_front();
    }

    // Get and ship object results
    std::deque<Sirikata::Protocol::Object::ObjectMessage*> object_results_copy;
    mObjectResults.swap(object_results_copy);
    mObjectResultsToSend.insert(mObjectResultsToSend.end(), object_results_copy.begin(), object_results_copy.end());


    bool object_sent = true;
    while(object_sent && !mObjectResultsToSend.empty()) {
        Sirikata::Protocol::Object::ObjectMessage* msg_front = mObjectResultsToSend.front();
        sendObjectResult(msg_front);
        delete msg_front;
        mObjectResultsToSend.pop_front();
    }
}



void LibproxProximity::handleAddObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed);
}

void LibproxProximity::handleRemoveObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}

void LibproxProximity::handleRemoveAllObjectLocSubscription(const UUID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void LibproxProximity::handleAddServerLocSubscription(const ServerID& subscriber, const UUID& observed, SeqNoPtr seqPtr) {
    // We check the cache when we get the request, but also check it here since
    // the observed object may have been removed between the request to add this
    // subscription and its actual execution.
    if (!mLocService->contains(observed)) return;

    mLocService->subscribe(subscriber, observed, seqPtr);
}
void LibproxProximity::handleRemoveServerLocSubscription(const ServerID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}
void LibproxProximity::handleRemoveAllServerLocSubscription(const ServerID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void LibproxProximity::queryHasEvents(Query* query) {
    if (
        query->handler() == mServerQueryHandler[OBJECT_CLASS_STATIC] ||
        query->handler() == mServerQueryHandler[OBJECT_CLASS_DYNAMIC]
    )
        generateServerQueryEvents(query);
    else
        generateObjectQueryEvents(query);
}


// Note: LocationServiceListener interface is only used in order to get updates on objects which have
// registered queries, allowing us to update those queries as appropriate.  All updating of objects
// in the prox data structure happens via the LocationServiceCache
void LibproxProximity::localObjectAdded(const UUID& uuid, bool agg, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bounds, const String& mesh, const String& physics) {
    updateObjectSize(uuid, bounds.radius());
}
void LibproxProximity::localObjectRemoved(const UUID& uuid, bool agg) {
    removeObjectSize(uuid);
}
void LibproxProximity::localLocationUpdated(const UUID& uuid, bool agg, const TimedMotionVector3f& newval) {
    updateQuery(uuid, newval, mLocService->bounds(uuid), NoUpdateSolidAngle, NoUpdateMaxResults);
    if (mSeparateDynamicObjects)
        checkObjectClass(true, uuid, newval);
}
void LibproxProximity::localBoundsUpdated(const UUID& uuid, bool agg, const BoundingSphere3f& newval) {
    updateQuery(uuid, mLocService->location(uuid), newval, NoUpdateSolidAngle, NoUpdateMaxResults);
    updateObjectSize(uuid, newval.radius());
}
void LibproxProximity::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    if (mSeparateDynamicObjects)
        checkObjectClass(false, uuid, newval);
}

void LibproxProximity::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg) {
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = aggregateBBoxes(bboxes);
    mServerQuerier->updateRegion(bbox);
}


// PROX Thread: Everything after this should only be called from within the prox thread.

void LibproxProximity::tickQueryHandler(ProxQueryHandler* qh[NUM_OBJECT_CLASSES]) {
    Time simT = mContext->simTime();
    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (qh[i] != NULL)
            qh[i]->tick(simT);
    }
}

void LibproxProximity::rebuildHandler(ObjectClass objtype) {
    if (mServerQueryHandler[objtype] != NULL)
        mServerQueryHandler[objtype]->rebuild();
    if (mObjectQueryHandler[objtype] != NULL)
        mObjectQueryHandler[objtype]->rebuild();
}

void LibproxProximity::generateServerQueryEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    Time t = mContext->simTime();
    uint32 max_count = GetOptionValue<uint32>(PROX_MAX_PER_RESULT);

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
                UUID objid = evt.additions()[aidx].id();
                if (mLocCache->tracking(objid)) { // If the cache already lost it, we can't do anything
                    count++;

                    mContext->mainStrand->post(
                        std::tr1::bind(&LibproxProximity::handleAddServerLocSubscription, this, sid, objid, seqNoPtr)
                    );

                    Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
                    addition.set_object( objid );

                    uint64 seqNo = (*seqNoPtr)++;
                    addition.set_seqno (seqNo);

                    TimedMotionVector3f loc = mLocCache->location(objid);
                    Sirikata::Protocol::ITimedMotionVector msg_loc = addition.mutable_location();
                    msg_loc.set_t(loc.updateTime());
                    msg_loc.set_position(loc.position());
                    msg_loc.set_velocity(loc.velocity());

                    TimedMotionQuaternion orient = mLocCache->orientation(objid);
                    Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                    msg_orient.set_t(orient.updateTime());
                    msg_orient.set_position(orient.position());
                    msg_orient.set_velocity(orient.velocity());

                    addition.set_bounds( mLocCache->bounds(objid) );
                    const String& mesh = mLocCache->mesh(objid);
                    if (mesh.size() > 0)
                        addition.set_mesh(mesh);
                    const String& phy = mLocCache->physics(objid);
                    if (phy.size() > 0)
                        addition.set_physics(phy);
                }
            }
            for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
                UUID objid = evt.removals()[ridx].id();
                count++;

                mContext->mainStrand->post(
                    std::tr1::bind(&LibproxProximity::handleRemoveServerLocSubscription, this, sid, objid)
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

void LibproxProximity::generateObjectQueryEvents(Query* query) {
    typedef std::deque<QueryEvent> QueryEventList;

    uint32 max_count = GetOptionValue<uint32>(PROX_MAX_PER_RESULT);

    UUID query_id = mInvertedObjectQueries[query];
    SeqNoPtr seqNoPtr = getSeqNoInfo(query_id);

    QueryEventList evts;
    query->popEvents(evts);

    while(!evts.empty()) {
        Sirikata::Protocol::Prox::ProximityResults prox_results;
        prox_results.set_t(mContext->simTime());

        uint32 count = 0;
        while(count < max_count && !evts.empty()) {
            const QueryEvent& evt = evts.front();
            Sirikata::Protocol::Prox::IProximityUpdate event_results = prox_results.add_update();

            for(uint32 aidx = 0; aidx < evt.additions().size(); aidx++) {
                UUID objid = evt.additions()[aidx].id();
                if (mLocCache->tracking(objid)) { // If the cache already lost it, we can't do anything
                    count++;

                    mContext->mainStrand->post(
                        std::tr1::bind(&LibproxProximity::handleAddObjectLocSubscription, this, query_id, objid)
                    );

                    Sirikata::Protocol::Prox::IObjectAddition addition = event_results.add_addition();
                    addition.set_object( objid );


                    //query_id contains the uuid of the object that is receiving
                    //the proximity message that obj_id has been added.
                    uint64 seqNo = (*seqNoPtr);
                    addition.set_seqno (seqNo);


                    Sirikata::Protocol::ITimedMotionVector motion = addition.mutable_location();
                    TimedMotionVector3f loc = mLocCache->location(objid);
                    motion.set_t(loc.updateTime());
                    motion.set_position(loc.position());
                    motion.set_velocity(loc.velocity());

                    TimedMotionQuaternion orient = mLocCache->orientation(objid);
                    Sirikata::Protocol::ITimedMotionQuaternion msg_orient = addition.mutable_orientation();
                    msg_orient.set_t(orient.updateTime());
                    msg_orient.set_position(orient.position());
                    msg_orient.set_velocity(orient.velocity());

                    addition.set_bounds( mLocCache->bounds(objid) );
                    const String& mesh = mLocCache->mesh(objid);
                    if (mesh.size() > 0)
                        addition.set_mesh(mesh);
                    const String& phy = mLocCache->physics(objid);
                    if (phy.size() > 0)
                        addition.set_physics(phy);
                }
            }
            for(uint32 ridx = 0; ridx < evt.removals().size(); ridx++) {
                UUID objid = evt.removals()[ridx].id();
                count++;
                // Clear out seqno and let main strand remove loc
                // subcription
                mContext->mainStrand->post(
                    std::tr1::bind(&LibproxProximity::handleRemoveObjectLocSubscription, this, query_id, objid)
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
    BoundingSphere3f region(bounds.center(), 0);
    float ms = bounds.radius();

    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mServerQueryHandler[i] == NULL) continue;

        ServerQueryMap::iterator it = mServerQueries[i].find(server);
        if (it == mServerQueries[i].end()) {
            PROXLOG(debug,"Add server query from " << server << ", min angle " << angle.asFloat() << ", object class " << ObjectClassToString((ObjectClass)i));

            Query* q = mServerDistance ?
                mServerQueryHandler[i]->registerQuery(loc, region, ms, SolidAngle::Min, mDistanceQueryDistance) :
                mServerQueryHandler[i]->registerQuery(loc, region, ms, angle) ;
            if (max_results != NoUpdateMaxResults && max_results > 0)
                q->maxResults(max_results);
            q->setEventListener(this);
            mServerQueries[i][server] = q;
            mInvertedServerQueries[q] = server;
        }
        else {
            PROXLOG(debug,"Update server query from " << server << ", min angle " << angle.asFloat() << ", object class " << ObjectClassToString((ObjectClass)i));

            Query* q = it->second;
            q->position(loc);
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
        if (mServerQueryHandler[i] == NULL) continue;

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
        std::tr1::bind(&LibproxProximity::handleRemoveAllServerLocSubscription, this, server)
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

    PROXLOG(detailed,"Update object query from " << object.toString() << ", min angle " << angle.asFloat() << ", max results " << max_results);

    if (mObjectSeqNos.find(object) == mObjectSeqNos.end())
        mObjectSeqNos.insert( ObjectSeqNoInfoMap::value_type(object, seqno) );

    for(int i = 0; i < NUM_OBJECT_CLASSES; i++) {
        if (mObjectQueryHandler[i] == NULL) continue;

        ObjectQueryMap::iterator it = mObjectQueries[i].find(object);

        if (it == mObjectQueries[i].end()) {
            // We only add if we actually have all the necessary info, most importantly a real minimum angle.
            // This is necessary because we get this update for all location updates, even those for objects
            // which don't have subscriptions.
            if (angle != NoUpdateSolidAngle) {
                Query* q = mObjectDistance ?
                    mObjectQueryHandler[i]->registerQuery(loc, region, ms, SolidAngle::Min, mDistanceQueryDistance) :
                    mObjectQueryHandler[i]->registerQuery(loc, region, ms, angle);
                if (max_results != NoUpdateMaxResults && max_results > 0)
                    q->maxResults(max_results);
                q->setEventListener(this);
                mObjectQueries[i][object] = q;
                mInvertedObjectQueries[q] = object;
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
        if (mObjectQueryHandler[i] == NULL) continue;

        ObjectQueryMap::iterator it = mObjectQueries[i].find(object);
        if (it == mObjectQueries[i].end()) continue;

        Query* q = it->second;
        mObjectQueries[i].erase(it);
        mInvertedObjectQueries.erase(q);
        delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.
    }

    // Clear out sequence numbers
    eraseSeqNoInfo(object);

    // Optionally let the main thread know to clear its communication state
    if (notify_main_thread) {
        // There's no corresponding removeAllSeqNoPtr because we
        // should have erased it above.
        mContext->mainStrand->post(
            std::tr1::bind(&LibproxProximity::handleRemoveAllObjectLocSubscription, this, object)
        );
    }
}

void LibproxProximity::handleDisconnectedObject(const UUID& object) {
    // Clear out query state if it exists
    handleRemoveObjectQuery(object, false);
}

bool LibproxProximity::handlerShouldHandleObject(bool is_static_handler, bool is_global_handler, const UUID& obj_id, bool is_local, const TimedMotionVector3f& pos, const BoundingSphere3f& region, float maxSize) {
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

void LibproxProximity::handleCheckObjectClassForHandlers(const UUID& objid, bool is_static, ProxQueryHandler* handlers[NUM_OBJECT_CLASSES]) {
    if ( (is_static && handlers[OBJECT_CLASS_STATIC]->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_DYNAMIC]->containsObject(objid)) )
        return;

    // Validate that the other handler has the object.
    assert(
        (is_static && handlers[OBJECT_CLASS_DYNAMIC]->containsObject(objid)) ||
        (!is_static && handlers[OBJECT_CLASS_STATIC]->containsObject(objid))
    );

    // If it wasn't in the right place, switch it.
    int swap_out = is_static ? OBJECT_CLASS_DYNAMIC : OBJECT_CLASS_STATIC;
    int swap_in = is_static ? OBJECT_CLASS_STATIC : OBJECT_CLASS_DYNAMIC;
    PROXLOG(debug, "Swapping " << objid.toString() << " from " << ObjectClassToString((ObjectClass)swap_out) << " to " << ObjectClassToString((ObjectClass)swap_in));
    handlers[swap_out]->removeObject(objid);
    handlers[swap_in]->addObject(objid);
}

void LibproxProximity::handleCheckObjectClass(bool is_local, const UUID& objid, const TimedMotionVector3f& newval) {
    assert(mSeparateDynamicObjects == true);

    // Basic approach: we need to check if the object has switched between
    // static/dynamic. We need to do this for both the local (object query) and
    // global (server query) handlers.
    bool is_static = velocityIsStatic(newval.velocity());
    handleCheckObjectClassForHandlers(objid, is_static, mObjectQueryHandler);
    if (is_local)
        handleCheckObjectClassForHandlers(objid, is_static, mServerQueryHandler);
}

} // namespace Sirikata
