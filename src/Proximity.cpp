/*  cbr
 *  Proximity.cpp
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#include "Proximity.hpp"
#include "Options.hpp"

#include <algorithm>

#include <prox/BruteForceQueryHandler.hpp>
#include <prox/RTreeQueryHandler.hpp>

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

namespace CBR {

static SolidAngle NoUpdateSolidAngle = SolidAngle(0.f);

struct ProximityInputEvent {
    enum Type {
        UpdateObjectQuery,
        RemoveObjectQuery,
        UpdateServerQuery,
        RemoveServerQuery,
    };

    Type type;

    UUID object;
    ServerID server;

    TimedMotionVector3f loc;
    BoundingSphere3f bounds;
    SolidAngle angle;

    static ProximityInputEvent UpdateObjectQueryEvent(const UUID& obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle) {
        ProximityInputEvent evt;
        evt.type = UpdateObjectQuery;
        evt.object = obj;
        evt.loc = loc;
        evt.bounds = bounds;
        evt.angle = angle;
        return evt;
    }

    static ProximityInputEvent RemoveObjectQueryEvent(const UUID& obj) {
        ProximityInputEvent evt;
        evt.type = RemoveObjectQuery;
        evt.object = obj;
        return evt;
    }

    static ProximityInputEvent UpdateServerQueryEvent(const ServerID& server, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle) {
        ProximityInputEvent evt;
        evt.type = UpdateServerQuery;
        evt.server = server;
        evt.loc = loc;
        evt.bounds = bounds;
        evt.angle = angle;
        return evt;
    }

    static ProximityInputEvent RemoveServerQueryEvent(const ServerID& server) {
        ProximityInputEvent evt;
        evt.type = RemoveServerQuery;
        evt.server = server;
        return evt;
    }
};


struct ProximityOutputEvent {
    enum Type {
        AddObjectLocSubscription,
        RemoveObjectLocSubscription,
        RemoveAllObjectLocSubscription,
        AddServerLocSubscription,
        RemoveServerLocSubscription,
        RemoveAllServerLocSubscription,
    };

    Type type;

    UUID object;
    ServerID server;

    UUID observed;


    static ProximityOutputEvent AddObjectLocSubscriptionEvent(const UUID& object, const UUID& observed) {
        ProximityOutputEvent evt;
        evt.type = AddObjectLocSubscription;
        evt.object = object;
        evt.observed = observed;
        return evt;
    }
    static ProximityOutputEvent RemoveObjectLocSubscriptionEvent(const UUID& object, const UUID& observed) {
        ProximityOutputEvent evt;
        evt.type = RemoveObjectLocSubscription;
        evt.object = object;
        evt.observed = observed;
        return evt;
    }
    static ProximityOutputEvent RemoveAllObjectLocSubscriptionEvent(const UUID& object) {
        ProximityOutputEvent evt;
        evt.type = RemoveAllObjectLocSubscription;
        evt.object = object;
        return evt;
    }

    static ProximityOutputEvent AddServerLocSubscriptionEvent(const ServerID& server, const UUID& observed) {
        ProximityOutputEvent evt;
        evt.type = AddServerLocSubscription;
        evt.server = server;
        evt.observed = observed;
        return evt;
    }
    static ProximityOutputEvent RemoveServerLocSubscriptionEvent(const ServerID& server, const UUID& observed) {
        ProximityOutputEvent evt;
        evt.type = RemoveServerLocSubscription;
        evt.server = server;
        evt.observed = observed;
        return evt;
    }
    static ProximityOutputEvent RemoveAllServerLocSubscriptionEvent(const ServerID& server) {
        ProximityOutputEvent evt;
        evt.type = RemoveAllServerLocSubscription;
        evt.server = server;
        return evt;
    }
};



Proximity::Proximity(SpaceContext* ctx, LocationService* locservice)
 : mContext(ctx),
   mLocService(locservice),
   mCSeg(NULL),
   mMinObjectQueryAngle(SolidAngle::Max),
   mNeedServerQueryUpdate(false),
   mProxThread(NULL),
   mShutdownProxThread(false),
   mServerQueries(),
   mLocalLocCache(NULL),
   mServerQueryHandler(NULL),
   mObjectQueries(),
   mGlobalLocCache(NULL),
   mObjectQueryHandler(NULL),
   mProfiler("Proximity Thread")
{
    // Server Queries
    mLocalLocCache = new CBRLocationServiceCache(locservice, false);
    mServerQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mServerQueryHandler->initialize(mLocalLocCache);

    // Object Queries
    mGlobalLocCache = new CBRLocationServiceCache(locservice, true);
    mObjectQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mObjectQueryHandler->initialize(mGlobalLocCache);

    mLocService->addListener(this);

    mContext->dispatcher()->registerMessageRecipient(MESSAGE_TYPE_SERVER_PROX_QUERY, this);
    mContext->dispatcher()->registerMessageRecipient(MESSAGE_TYPE_SERVER_PROX_RESULT, this);

    // Start the processing thread
    mProxThread = new boost::thread( std::tr1::bind(&Proximity::proxThreadMain, this) );
}

Proximity::~Proximity() {
    mContext->dispatcher()->unregisterMessageRecipient(MESSAGE_TYPE_SERVER_PROX_QUERY, this);
    mContext->dispatcher()->unregisterMessageRecipient(MESSAGE_TYPE_SERVER_PROX_RESULT, this);

    delete mObjectQueryHandler;
    delete mGlobalLocCache;

    delete mServerQueryHandler;
    delete mLocalLocCache;
}


// MAIN Thread Methods: The following should only be called from the main thread.

void Proximity::initialize(CoordinateSegmentation* cseg) {
    mCSeg = cseg;

    mCSeg->addListener(this);

    mNeedServerQueryUpdate = true;
}

void Proximity::shutdown() {
    // Shut down the main processing thread
    mShutdownProxThread = true;
    mProxThread->join(); // FIXME this should probably be a timed_join
}

void Proximity::sendQueryRequests() {
    TimedMotionVector3f loc;

    // FIXME avoid computing this so much
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);

    BoundingSphere3f bounds = bbox.toBoundingSphere();

    // FIXME this assumes that ServerIDs are simple sequence of IDs
    for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
        if (sid == mContext->id()) continue;

        ServerProximityQueryMessage* msg = new ServerProximityQueryMessage(mContext->id());
        msg->contents.set_action(CBR::Protocol::Prox::ServerQuery::AddOrUpdate);
        CBR::Protocol::Prox::ITimedMotionVector msg_loc = msg->contents.mutable_location();
        msg_loc.set_t(loc.updateTime());
        msg_loc.set_position(loc.position());
        msg_loc.set_position(loc.velocity());
        msg->contents.set_bounds(bounds);
        msg->contents.set_min_angle(mMinObjectQueryAngle.asFloat());
        mContext->router()->route(MessageRouter::PROXS, msg, sid);
    }
}

void Proximity::receiveMessage(Message* msg) {
    ServerProximityQueryMessage* prox_query_msg = dynamic_cast<ServerProximityQueryMessage*>(msg);
    if (prox_query_msg != NULL) {

        // FIXME we need to have a way to get the source server from the message without using this magic,
        // probably by saving the header and delivering it along with the message
        ServerID source_server = GetUniqueIDServerID(msg->id());

        if (prox_query_msg->contents.action() == CBR::Protocol::Prox::ServerQuery::AddOrUpdate) {
            assert(
                prox_query_msg->contents.has_location() &&
                prox_query_msg->contents.has_bounds() &&
                prox_query_msg->contents.has_min_angle()
            );

            CBR::Protocol::Prox::ITimedMotionVector msg_loc = prox_query_msg->contents.location();
            TimedMotionVector3f qloc(msg_loc.t(), MotionVector3f(msg_loc.position(), msg_loc.velocity()));
            SolidAngle minangle(prox_query_msg->contents.min_angle());

            updateQuery(source_server, qloc, prox_query_msg->contents.bounds(), minangle);
        }
        else if (prox_query_msg->contents.action() == CBR::Protocol::Prox::ServerQuery::Remove) {
            removeQuery(source_server);
        }
        else {
            assert(false);
        }
    }

    ServerProximityResultMessage* prox_result_msg = dynamic_cast<ServerProximityResultMessage*>(msg);
    if (prox_result_msg != NULL) {
        assert( prox_result_msg->contents.has_t() );
        Time t = prox_result_msg->contents.t();

        if (prox_result_msg->contents.addition_size() > 0) {
            for(int32 idx = 0; idx < prox_result_msg->contents.addition_size(); idx++) {
                CBR::Protocol::Prox::ObjectAddition addition = prox_result_msg->contents.addition(idx);
                mLocService->addReplicaObject(
                    t,
                    addition.object(),
                    TimedMotionVector3f( addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()) ),
                    addition.bounds()
                );
            }
        }

        if (prox_result_msg->contents.removal_size() > 0) {
            for(int32 idx = 0; idx < prox_result_msg->contents.removal_size(); idx++) {
                CBR::Protocol::Prox::ObjectRemoval removal = prox_result_msg->contents.removal(idx);
                mLocService->removeReplicaObject(t, removal.object());
            }
        }
    }
}

// MigrationDataClient Interface

std::string Proximity::migrationClientTag() {
    return "prox";
}

std::string Proximity::generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) {
    ObjectQueryAngleMap::iterator it = mObjectQueryAngles.find(obj);
    if (it == mObjectQueryAngles.end()) // no query registered, return nothing
        return std::string();
    else {
        SolidAngle query_angle = it->second;
        removeQuery(obj);

        CBR::Protocol::Prox::ObjectMigrationData migr_data;
        migr_data.set_min_angle( query_angle.asFloat() );
        return serializePBJMessage(migr_data);
    }
}

void Proximity::receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) {
    CBR::Protocol::Prox::ObjectMigrationData migr_data;
    bool parse_success = migr_data.ParseFromString(data);
    assert(parse_success);

    SolidAngle obj_query_angle(migr_data.min_angle());
    addQuery(obj, obj_query_angle);
}


void Proximity::updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa) {
    mInputEvents.push(ProximityInputEvent::UpdateServerQueryEvent(sid, loc, bounds, sa));
}

void Proximity::removeQuery(ServerID sid) {
    mInputEvents.push(ProximityInputEvent::RemoveServerQueryEvent(sid));
}

void Proximity::addQuery(UUID obj, SolidAngle sa) {
    updateQuery(obj, mLocService->location(obj), mLocService->bounds(obj), sa);
}

void Proximity::updateQuery(UUID obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa) {
    // Update the main thread's record
    mObjectQueryAngles[obj] = sa;

    // Update the prox thread
    mInputEvents.push(ProximityInputEvent::UpdateObjectQueryEvent(obj, loc, bounds, sa));

    // Update min query angle, and update remote queries if necessary
    if (sa < mMinObjectQueryAngle) {
        mMinObjectQueryAngle = sa;
        PROXLOG(debug,"Query addition initiated server query request.");
        mNeedServerQueryUpdate = true;
    }
}

void Proximity::removeQuery(UUID obj) {
    // Update the main thread's record
    SolidAngle sa = mObjectQueryAngles[obj];
    mObjectQueryAngles.erase(obj);

    // Update the prox thread
    mInputEvents.push(ProximityInputEvent::RemoveObjectQueryEvent(obj));

    // Update min query angle, and update remote queries if necessary
    if (sa == mMinObjectQueryAngle) {
        PROXLOG(debug,"Query removal initiated server query request.");
        SolidAngle minangle(SolidAngle::Max);
        for(ObjectQueryAngleMap::iterator it = mObjectQueryAngles.begin(); it != mObjectQueryAngles.end(); it++)
            if (it->second < minangle) minangle = it->second;

        // NOTE: Even if this condition is satisfied, we could only be increasing
        // the minimum angle, so we don't *strictly* need to update the query.
        // Some buffer timing might be in order here to avoid excessive updates
        // while still getting the benefit from reducing the query angle.
        if (minangle != mMinObjectQueryAngle) {
            mMinObjectQueryAngle = minangle;
            mNeedServerQueryUpdate = true;
        }
    }
}

void Proximity::service() {
    // Update server-to-server angles if necessary
    if (mNeedServerQueryUpdate) {
        sendQueryRequests();
        mNeedServerQueryUpdate = false;
    }

    // Get and handle output events
    std::deque<ProximityOutputEvent> output_events;
    mOutputEvents.swap(output_events);
    for(std::deque<ProximityOutputEvent>::iterator it = output_events.begin(); it != output_events.end(); it++)
        handleOutputEvent(*it);

    // Get and ship server results
    std::deque<ProxResultServerPair> server_results;
    mServerResults.swap(server_results);
    for(std::deque<ProxResultServerPair>::iterator it = server_results.begin(); it != server_results.end(); it++)
        mContext->router()->route(MessageRouter::PROXS, it->msg, it->dest);

    // Get and ship object results
    std::deque<CBR::Protocol::Object::ObjectMessage*> obj_results;
    mObjectResults.swap(obj_results);
    for(std::deque<CBR::Protocol::Object::ObjectMessage*>::iterator it = obj_results.begin(); it != obj_results.end(); it++)
        mContext->router()->route(*it, false);
}

void Proximity::handleOutputEvent(const ProximityOutputEvent& evt) {
    switch(evt.type) {
      case ProximityOutputEvent::AddObjectLocSubscription:
        mLocService->subscribe(evt.object, evt.observed);
        break;
      case ProximityOutputEvent::RemoveObjectLocSubscription:
        mLocService->unsubscribe(evt.object, evt.observed);
        break;
      case ProximityOutputEvent::RemoveAllObjectLocSubscription:
        mLocService->unsubscribe(evt.object);
        break;
      case ProximityOutputEvent::AddServerLocSubscription:
        mLocService->subscribe(evt.server, evt.observed);
        break;
      case ProximityOutputEvent::RemoveServerLocSubscription:
        mLocService->unsubscribe(evt.server, evt.observed);
        break;
      case ProximityOutputEvent::RemoveAllServerLocSubscription:
        mLocService->unsubscribe(evt.server);
        break;
    }
}

void Proximity::queryHasEvents(Query* query) {
    // Currently we don't use this directly, we just always iterate over all queries and check them.
    // FIXME we could store this information and only check the ones we get callbacks for here
}


// Note: LocationServiceListener interface is only used in order to get updates on objects which have
// registered queries, allowing us to update those queries as appropriate.  All updating of objects
// in the prox data structure happens via the LocationServiceCache
void Proximity::localObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
}
void Proximity::localObjectRemoved(const UUID& uuid) {
}
void Proximity::localLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
    updateQuery(uuid, newval, mLocService->bounds(uuid), NoUpdateSolidAngle);
}
void Proximity::localBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
    updateQuery(uuid, mLocService->location(uuid), newval, NoUpdateSolidAngle);
}
void Proximity::replicaObjectAdded(const UUID& uuid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds) {
}
void Proximity::replicaObjectRemoved(const UUID& uuid) {
}
void Proximity::replicaLocationUpdated(const UUID& uuid, const TimedMotionVector3f& newval) {
}
void Proximity::replicaBoundsUpdated(const UUID& uuid, const BoundingSphere3f& newval) {
}

void Proximity::updatedSegmentation(CoordinateSegmentation* cseg, const std::vector<SegmentationInfo>& new_seg) {
}


// PROX Thread: Everything after this should only be called from within the prox thread.

// The main loop for the prox processing thread
void Proximity::proxThreadMain() {
    mProfiler.addStage("Handle Input Events");
    mProfiler.addStage("Local Loc Cache");
    mProfiler.addStage("Global Loc Cache");
    mProfiler.addStage("Server Queries");
    mProfiler.addStage("Object Queries");
    mProfiler.addStage("Generate Server Query Events");
    mProfiler.addStage("Generate Object Query Events");

    while( !mShutdownProxThread ) {
        Time tstart = Timer::now();

        Time simT = mContext->simTime(tstart);

        mProfiler.startIteration();

        // Service input events
        std::deque<ProximityInputEvent> inputEvents;
        mInputEvents.swap(inputEvents);
        for(std::deque<ProximityInputEvent>::iterator it = inputEvents.begin(); it != inputEvents.end(); it++)
            handleInputEvent( *it );
        mProfiler.finishedStage();

        // Service location caches
        mLocalLocCache->serviceListeners();    mProfiler.finishedStage();
        mGlobalLocCache->serviceListeners();   mProfiler.finishedStage();

        // Service query handlers
        mServerQueryHandler->tick(simT);       mProfiler.finishedStage();
        mObjectQueryHandler->tick(simT);       mProfiler.finishedStage();

        generateServerQueryEvents(simT);       mProfiler.finishedStage();
        generateObjectQueryEvents();           mProfiler.finishedStage();

        Time tend = Timer::now();

#define MIN_ITERATION_TIME Duration::milliseconds((int64)100)
        Duration since_start = tend - tstart;
        if (since_start < MIN_ITERATION_TIME)
            usleep( (MIN_ITERATION_TIME - since_start).toMicroseconds() );
    }

    if (GetOption(PROFILE)->as<bool>())
        mProfiler.report();
}

void Proximity::generateServerQueryEvents(const Time& t) {
    typedef std::deque<QueryEvent> QueryEventList;

    for(ServerQueryMap::iterator query_it = mServerQueries.begin(); query_it != mServerQueries.end(); query_it++) {
        ServerID sid = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        if (evts.empty()) continue;

        ServerProximityResultMessage* result_msg = new ServerProximityResultMessage(mContext->id());
        result_msg->contents.set_t(t);
        for(QueryEventList::iterator evt_it = evts.begin(); evt_it != evts.end(); evt_it++) {
            if (evt_it->type() == QueryEvent::Added) {
                mOutputEvents.push( ProximityOutputEvent::AddServerLocSubscriptionEvent(sid, evt_it->id()) );

                CBR::Protocol::Prox::IObjectAddition addition = result_msg->contents.add_addition();
                addition.set_object( evt_it->id() );

                TimedMotionVector3f loc = mLocalLocCache->location(evt_it->id());
                CBR::Protocol::Prox::ITimedMotionVector msg_loc = addition.mutable_location();
                msg_loc.set_t(loc.updateTime());
                msg_loc.set_position(loc.position());
                msg_loc.set_velocity(loc.velocity());

                addition.set_bounds( mLocalLocCache->bounds(evt_it->id()) );
            }
            else {
                mOutputEvents.push( ProximityOutputEvent::RemoveServerLocSubscriptionEvent(sid, evt_it->id()) );
                CBR::Protocol::Prox::IObjectRemoval removal = result_msg->contents.add_removal();
                removal.set_object( evt_it->id() );
            }
        }

        PROXLOG(insane,"Reporting " << result_msg->contents.addition_size() << " additions, " << result_msg->contents.removal_size() << " removals to server " << sid);

        mServerResults.push( ProxResultServerPair(sid, result_msg) );
    }
}

void Proximity::generateObjectQueryEvents() {
    typedef std::deque<QueryEvent> QueryEventList;

    for(ObjectQueryMap::iterator query_it = mObjectQueries.begin(); query_it != mObjectQueries.end(); query_it++) {
        UUID query_id = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        CBR::Protocol::Prox::ProximityResults prox_results;
        prox_results.set_t(mContext->time);
        for(QueryEventList::iterator evt_it = evts.begin(); evt_it != evts.end(); evt_it++) {
            if (evt_it->type() == QueryEvent::Added) {
                mOutputEvents.push( ProximityOutputEvent::AddObjectLocSubscriptionEvent(query_id, evt_it->id()) );

                CBR::Protocol::Prox::IObjectAddition addition = prox_results.add_addition();
                addition.set_object( evt_it->id() );

                CBR::Protocol::Prox::ITimedMotionVector motion = addition.mutable_location();
                TimedMotionVector3f loc = mGlobalLocCache->location(evt_it->id());
                motion.set_t(loc.updateTime());
                motion.set_position(loc.position());
                motion.set_velocity(loc.velocity());

                addition.set_bounds( mGlobalLocCache->bounds(evt_it->id()) );
            }
            else {
                mOutputEvents.push( ProximityOutputEvent::RemoveObjectLocSubscriptionEvent(query_id, evt_it->id()) );

                CBR::Protocol::Prox::IObjectRemoval removal = prox_results.add_removal();
                removal.set_object( evt_it->id() );
            }
        }

        CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
        obj_msg->set_source_object(UUID::null());
        obj_msg->set_source_port(OBJECT_PORT_PROXIMITY);
        obj_msg->set_dest_object(query_id);
        obj_msg->set_dest_port(OBJECT_PORT_PROXIMITY);
        obj_msg->set_unique(GenerateUniqueID(mContext->id()));
        obj_msg->set_payload( serializePBJMessage(prox_results) );

        mObjectResults.push(obj_msg);
    }
}



// Prox Thread Event Handling
void Proximity::handleInputEvent(const ProximityInputEvent& evt) {
    switch(evt.type) {
      case ProximityInputEvent::UpdateServerQuery:
          {
              ServerQueryMap::iterator it = mServerQueries.find(evt.server);
              if (it == mServerQueries.end()) {
                  PROXLOG(debug,"Add server query from " << evt.server << ", min angle " << evt.angle.asFloat());

                  Query* q = mServerQueryHandler->registerQuery(evt.loc, evt.bounds, evt.angle);
                  mServerQueries[evt.server] = q;
              }
              else {
                  PROXLOG(debug,"Update server query from " << evt.server << ", min angle " << evt.angle.asFloat());

                  Query* q = it->second;
                  q->position(evt.loc);
                  q->bounds(evt.bounds);
                  q->angle(evt.angle);
              }
          }
          break;
      case ProximityInputEvent::RemoveServerQuery:
          {
              PROXLOG(debug,"Remove server query from " << evt.server);

              ServerQueryMap::iterator it = mServerQueries.find(evt.server);
              if (it == mServerQueries.end()) return;

              Query* q = it->second;
              mServerQueries.erase(it);
              delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.

              mOutputEvents.push( ProximityOutputEvent::RemoveAllServerLocSubscriptionEvent(evt.server) );
          }
          break;
      case ProximityInputEvent::UpdateObjectQuery:
          {
              ObjectQueryMap::iterator it = mObjectQueries.find(evt.object);

              if (it == mObjectQueries.end()) {
                  Query* q = mObjectQueryHandler->registerQuery(evt.loc, evt.bounds, evt.angle);
                  mObjectQueries[evt.object] = q;
              }
              else {
                  Query* query = it->second;
                  query->position(evt.loc);
                  query->bounds(evt.bounds);
                  if (evt.angle != NoUpdateSolidAngle)
                      query->angle(evt.angle);
              }
          }
          break;
      case ProximityInputEvent::RemoveObjectQuery:
          {
              ObjectQueryMap::iterator it = mObjectQueries.find(evt.object);
              if (it == mObjectQueries.end()) return;

              Query* q = it->second;
              mObjectQueries.erase(it);
              delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.

              mOutputEvents.push( ProximityOutputEvent::RemoveAllObjectLocSubscriptionEvent(evt.object) );
          }
          break;
    }
}

} // namespace CBR
