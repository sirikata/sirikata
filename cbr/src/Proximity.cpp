/*  Sirikata
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

#include "Proximity.hpp"
#include "Options.hpp"
#include <sirikata/cbrcore/Options.hpp>

#include <algorithm>

#include <prox/BruteForceQueryHandler.hpp>
#include <prox/RTreeQueryHandler.hpp>

#include "CBR_Prox.pbj.hpp"

#include <sirikata/core/network/IOServiceFactory.hpp>

#include <float.h>

#define PROXLOG(level,msg) SILOG(prox,level,"[PROX] " << msg)

namespace Sirikata {

const ProxSimulationTraits::realType ProxSimulationTraits::InfiniteRadius = FLT_MAX;

static SolidAngle NoUpdateSolidAngle = SolidAngle(0.f);

Proximity::Proximity(SpaceContext* ctx, LocationService* locservice)
 : PollingService(ctx->mainStrand, Duration::milliseconds((int64)100)), // FIXME
   mContext(ctx),
   mLocService(locservice),
   mCSeg(NULL),
   mMinObjectQueryAngle(SolidAngle::Max),
   mProxThread(NULL),
   mProxService(NULL),
   mProxStrand(NULL),
   mServerQueries(),
   mLocalLocCache(NULL),
   mServerQueryHandler(NULL),
   mObjectQueries(),
   mGlobalLocCache(NULL),
   mObjectQueryHandler(NULL)
{
    // Do some necessary initialization for the prox thread, needed to let main thread
    // objects know about it's strand/service
    mProxService = Network::IOServiceFactory::makeIOService();
    mProxStrand = mProxService->createStrand();

    // Server Queries
    mLocalLocCache = new CBRLocationServiceCache(mProxStrand, locservice, false);
    mServerQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mServerQueryHandler->initialize(mLocalLocCache);

    // Object Queries
    mGlobalLocCache = new CBRLocationServiceCache(mProxStrand, locservice, true);
    mObjectQueryHandler = new Prox::BruteForceQueryHandler<ProxSimulationTraits>();
    mObjectQueryHandler->initialize(mGlobalLocCache);

    mLocService->addListener(this);

    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_PROX, this);

    mProxServerMessageService = mContext->serverRouter()->createServerMessageService("proximity");

    // Start the processing thread
    mProxThread = new Thread( std::tr1::bind(&Proximity::proxThreadMain, this) );
}

Proximity::~Proximity() {
    delete mProxServerMessageService;

    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_PROX, this);

    delete mObjectQueryHandler;
    delete mGlobalLocCache;

    delete mServerQueryHandler;
    delete mLocalLocCache;

    delete mProxStrand;
    Network::IOServiceFactory::destroyIOService(mProxService);
    mProxService = NULL;
}


// MAIN Thread Methods: The following should only be called from the main thread.

void Proximity::initialize(CoordinateSegmentation* cseg) {
    mCSeg = cseg;

    mCSeg->addListener(this);
}

void Proximity::shutdown() {
    // Shut down the main processing thread
    if (mProxThread != NULL) {
        if (mProxService != NULL)
            mProxService->stop();
        mProxThread->join();
    }
}

void Proximity::addAllServersForUpdate() {
    // FIXME this assumes that ServerIDs are simple sequence of IDs
    for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
        if (sid == mContext->id()) continue;
        mNeedServerQueryUpdate.insert(sid);
    }
}

void Proximity::sendQueryRequests() {
    TimedMotionVector3f loc;

    // FIXME avoid computing this so much
    BoundingBoxList bboxes = mCSeg->serverRegion(mContext->id());
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);

    BoundingSphere3f bounds = bbox.toBoundingSphere();

    std::set<ServerID> sub_servers;
    sub_servers.swap(mNeedServerQueryUpdate);
    for(std::set<ServerID>::iterator it = sub_servers.begin(); it != sub_servers.end(); it++) {
        ServerID sid = *it;
        Sirikata::Protocol::Prox::Container container;
        Sirikata::Protocol::Prox::IServerQuery msg = container.mutable_query();
        msg.set_action(Sirikata::Protocol::Prox::ServerQuery::AddOrUpdate);
        Sirikata::Protocol::Prox::ITimedMotionVector msg_loc = msg.mutable_location();
        msg_loc.set_t(loc.updateTime());
        msg_loc.set_position(loc.position());
        msg_loc.set_position(loc.velocity());
        msg.set_bounds(bounds);
        msg.set_min_angle(mMinObjectQueryAngle.asFloat());

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
            mNeedServerQueryUpdate.insert(sid);
        }
    }
}

void Proximity::receiveMessage(Message* msg) {
    assert(msg->dest_port() == SERVER_PORT_PROX);

    Sirikata::Protocol::Prox::Container prox_container;
    bool parsed = parsePBJMessage(&prox_container, msg->payload());
    if (!parsed) {
        PROXLOG(warn,"Couldn't parse message, ID=" << msg->id());
        delete msg;
        return;
    }

    if (prox_container.has_query()) {
        Sirikata::Protocol::Prox::ServerQuery prox_query_msg = prox_container.query();

        ServerID source_server = msg->source_server();

        if (prox_query_msg.action() == Sirikata::Protocol::Prox::ServerQuery::AddOrUpdate) {
            assert(
                prox_query_msg.has_location() &&
                prox_query_msg.has_bounds() &&
                prox_query_msg.has_min_angle()
            );

            Sirikata::Protocol::Prox::TimedMotionVector msg_loc = prox_query_msg.location();
            TimedMotionVector3f qloc(msg_loc.t(), MotionVector3f(msg_loc.position(), msg_loc.velocity()));
            SolidAngle minangle(prox_query_msg.min_angle());

            updateQuery(source_server, qloc, prox_query_msg.bounds(), minangle);
        }
        else if (prox_query_msg.action() == Sirikata::Protocol::Prox::ServerQuery::Remove) {
            removeQuery(source_server);
        }
        else {
            assert(false);
        }
    }

    if (prox_container.has_result()) {
        Sirikata::Protocol::Prox::ProximityResults prox_result_msg = prox_container.result();

        assert( prox_result_msg.has_t() );
        Time t = prox_result_msg.t();

        if (prox_result_msg.addition_size() > 0) {
            for(int32 idx = 0; idx < prox_result_msg.addition_size(); idx++) {
                Sirikata::Protocol::Prox::ObjectAddition addition = prox_result_msg.addition(idx);
                mLocService->addReplicaObject(
                    t,
                    addition.object(),
                    TimedMotionVector3f( addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()) ),
                    addition.bounds()
                );
            }
        }

        if (prox_result_msg.removal_size() > 0) {
            for(int32 idx = 0; idx < prox_result_msg.removal_size(); idx++) {
                Sirikata::Protocol::Prox::ObjectRemoval removal = prox_result_msg.removal(idx);
                mLocService->removeReplicaObject(t, removal.object());
            }
        }
    }

    delete msg;
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

        Sirikata::Protocol::Prox::ObjectMigrationData migr_data;
        migr_data.set_min_angle( query_angle.asFloat() );
        return serializePBJMessage(migr_data);
    }
}

void Proximity::receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) {
    Sirikata::Protocol::Prox::ObjectMigrationData migr_data;
    bool parse_success = migr_data.ParseFromString(data);
    assert(parse_success);

    SolidAngle obj_query_angle(migr_data.min_angle());
    addQuery(obj, obj_query_angle);
}


void Proximity::updateQuery(ServerID sid, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& sa) {
    mProxStrand->post(
        std::tr1::bind(&Proximity::handleUpdateServerQuery, this, sid, loc, bounds, sa)
    );
}

void Proximity::removeQuery(ServerID sid) {
    mProxStrand->post(
        std::tr1::bind(&Proximity::handleRemoveServerQuery, this, sid)
    );
}

void Proximity::addQuery(UUID obj, SolidAngle sa) {
    updateQuery(obj, mLocService->location(obj), mLocService->bounds(obj), sa);
}

void Proximity::updateQuery(UUID obj, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, SolidAngle sa) {
    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&Proximity::handleUpdateObjectQuery, this, obj, loc, bounds, sa)
    );

    if (sa != NoUpdateSolidAngle) {
        // Update the main thread's record
        mObjectQueryAngles[obj] = sa;

        // Update min query angle, and update remote queries if necessary
        if (sa < mMinObjectQueryAngle) {
            mMinObjectQueryAngle = sa;
            PROXLOG(debug,"Query addition initiated server query request.");
            addAllServersForUpdate();
        }
    }
}

void Proximity::removeQuery(UUID obj) {
    // Update the main thread's record
    SolidAngle sa = mObjectQueryAngles[obj];
    mObjectQueryAngles.erase(obj);

    // Update the prox thread
    mProxStrand->post(
        std::tr1::bind(&Proximity::handleRemoveObjectQuery, this, obj)
    );

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
            addAllServersForUpdate();
        }
    }
}

void Proximity::poll() {
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
        boost::shared_ptr<Stream<UUID> > proxStream = mContext->getObjectStream(msg_front->dest_object());
        std::string proxMsg = msg_front->payload();

        if (proxStream != boost::shared_ptr<Stream<UUID> >()) {
          object_sent = proxStream->connection().lock()->datagram( (void*)proxMsg.data(), proxMsg.size(),
                                                         OBJECT_PORT_PROXIMITY, OBJECT_PORT_PROXIMITY, NULL);
        }
        else {
          object_sent = false;
        }

        if (object_sent)
            mObjectResultsToSend.pop_front();
    }
}

void Proximity::handleAddObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    mLocService->subscribe(subscriber, observed);
}
void Proximity::handleRemoveObjectLocSubscription(const UUID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}
void Proximity::handleRemoveAllObjectLocSubscription(const UUID& subscriber) {
    mLocService->unsubscribe(subscriber);
}

void Proximity::handleAddServerLocSubscription(const ServerID& subscriber, const UUID& observed) {
    mLocService->subscribe(subscriber, observed);
}
void Proximity::handleRemoveServerLocSubscription(const ServerID& subscriber, const UUID& observed) {
    mLocService->unsubscribe(subscriber, observed);
}
void Proximity::handleRemoveAllServerLocSubscription(const ServerID& subscriber) {
    mLocService->unsubscribe(subscriber);
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
    Duration max_rate = Duration::milliseconds((int64)100);

    Poller mServerHandlerPoller(mProxStrand, std::tr1::bind(&Proximity::tickQueryHandler, this, mServerQueryHandler), max_rate);
    Poller mObjectHandlerPoller(mProxStrand, std::tr1::bind(&Proximity::tickQueryHandler, this, mObjectQueryHandler), max_rate);
    Poller mServerEventsPoller(mProxStrand, std::tr1::bind(&Proximity::generateServerQueryEvents, this), max_rate);
    Poller mObjectEventsPoller(mProxStrand, std::tr1::bind(&Proximity::generateObjectQueryEvents, this), max_rate);

    mServerHandlerPoller.start();
    mObjectHandlerPoller.start();
    mServerEventsPoller.start();
    mObjectEventsPoller.start();

    mProxService->run();
}

void Proximity::tickQueryHandler(ProxQueryHandler* qh) {
    Time simT = mContext->simTime();
    qh->tick(simT);
}

void Proximity::generateServerQueryEvents() {
    typedef std::deque<QueryEvent> QueryEventList;

    Time t = mContext->simTime();
    uint32 max_count = GetOption(PROX_MAX_PER_RESULT)->as<uint32>();

    for(ServerQueryMap::iterator query_it = mServerQueries.begin(); query_it != mServerQueries.end(); query_it++) {
        ServerID sid = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        while(!evts.empty()) {
            Sirikata::Protocol::Prox::Container container;
            Sirikata::Protocol::Prox::IProximityResults contents = container.mutable_result();
            contents.set_t(t);
            uint32 count = 0;
            while(count < max_count && !evts.empty()) {
                const QueryEvent& evt = evts.front();
                if (evt.type() == QueryEvent::Added) {
                    if (mLocalLocCache->tracking(evt.id())) { // If the cache already lost it, we can't do anything
                        count++;

                        mContext->mainStrand->post(
                            std::tr1::bind(&Proximity::handleAddServerLocSubscription, this, sid, evt.id())
                        );

                        Sirikata::Protocol::Prox::IObjectAddition addition = contents.add_addition();
                        addition.set_object( evt.id() );

                        TimedMotionVector3f loc = mLocalLocCache->location(evt.id());
                        Sirikata::Protocol::Prox::ITimedMotionVector msg_loc = addition.mutable_location();
                        msg_loc.set_t(loc.updateTime());
                        msg_loc.set_position(loc.position());
                        msg_loc.set_velocity(loc.velocity());

                        addition.set_bounds( mLocalLocCache->bounds(evt.id()) );
                    }
                }
                else {
                    count++;
                    mContext->mainStrand->post(
                        std::tr1::bind(&Proximity::handleRemoveServerLocSubscription, this, sid, evt.id())
                    );
                    Sirikata::Protocol::Prox::IObjectRemoval removal = contents.add_removal();
                    removal.set_object( evt.id() );
                }

                evts.pop_front();
            }

            PROXLOG(insane,"Reporting " << contents.addition_size() << " additions, " << contents.removal_size() << " removals to server " << sid);

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
}

void Proximity::generateObjectQueryEvents() {
    typedef std::deque<QueryEvent> QueryEventList;

    uint32 max_count = GetOption(PROX_MAX_PER_RESULT)->as<uint32>();

    for(ObjectQueryMap::iterator query_it = mObjectQueries.begin(); query_it != mObjectQueries.end(); query_it++) {
        UUID query_id = query_it->first;
        Query* query = query_it->second;

        QueryEventList evts;
        query->popEvents(evts);

        while(!evts.empty()) {
            Sirikata::Protocol::Prox::ProximityResults prox_results;
            prox_results.set_t(mContext->simTime());

            uint32 count = 0;
            while(count < max_count && !evts.empty()) {
                const QueryEvent& evt = evts.front();

                if (evt.type() == QueryEvent::Added) {
                    if (mGlobalLocCache->tracking(evt.id())) { // If the cache already lost it, we can't do anything
                        count++;
                        mContext->mainStrand->post(
                            std::tr1::bind(&Proximity::handleAddObjectLocSubscription, this, query_id, evt.id())
                        );

                        Sirikata::Protocol::Prox::IObjectAddition addition = prox_results.add_addition();
                        addition.set_object( evt.id() );

                        Sirikata::Protocol::Prox::ITimedMotionVector motion = addition.mutable_location();
                        TimedMotionVector3f loc = mGlobalLocCache->location(evt.id());
                        motion.set_t(loc.updateTime());
                        motion.set_position(loc.position());
                        motion.set_velocity(loc.velocity());

                        addition.set_bounds( mGlobalLocCache->bounds(evt.id()) );
                    }
                }
                else {
                    count++;
                    mContext->mainStrand->post(
                        std::tr1::bind(&Proximity::handleRemoveObjectLocSubscription, this, query_id, evt.id())
                    );

                    Sirikata::Protocol::Prox::IObjectRemoval removal = prox_results.add_removal();
                    removal.set_object( evt.id() );
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
}



void Proximity::handleUpdateServerQuery(const ServerID& server, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle) {
    ServerQueryMap::iterator it = mServerQueries.find(server);
    if (it == mServerQueries.end()) {
        PROXLOG(debug,"Add server query from " << server << ", min angle " << angle.asFloat());

        Query* q = mServerQueryHandler->registerQuery(loc, bounds, angle);
        mServerQueries[server] = q;
    }
    else {
        PROXLOG(debug,"Update server query from " << server << ", min angle " << angle.asFloat());

        Query* q = it->second;
        q->position(loc);
        q->bounds(bounds);
        q->angle(angle);
    }
}

void Proximity::handleRemoveServerQuery(const ServerID& server) {
    PROXLOG(debug,"Remove server query from " << server);

    ServerQueryMap::iterator it = mServerQueries.find(server);
    if (it == mServerQueries.end()) return;

    Query* q = it->second;
    mServerQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.

    mContext->mainStrand->post(
        std::tr1::bind(&Proximity::handleRemoveAllServerLocSubscription, this, server)
    );
}

void Proximity::handleUpdateObjectQuery(const UUID& object, const TimedMotionVector3f& loc, const BoundingSphere3f& bounds, const SolidAngle& angle) {
    ObjectQueryMap::iterator it = mObjectQueries.find(object);

    if (it == mObjectQueries.end()) {
        // We only add if we actually have all the necessary info, most importantly a real minimum angle.
        // This is necessary because we get this update for all location updates, even those for objects
        // which don't have subscriptions.
        if (angle != NoUpdateSolidAngle) {
            Query* q = mObjectQueryHandler->registerQuery(loc, bounds, angle);
            mObjectQueries[object] = q;
        }
    }
    else {
        Query* query = it->second;
        query->position(loc);
        query->bounds(bounds);
        if (angle != NoUpdateSolidAngle)
            query->angle(angle);
    }
}

void Proximity::handleRemoveObjectQuery(const UUID& object) {
    ObjectQueryMap::iterator it = mObjectQueries.find(object);
    if (it == mObjectQueries.end()) return;

    Query* q = it->second;
    mObjectQueries.erase(it);
    delete q; // Note: Deleting query notifies QueryHandler and unsubscribes.

    mContext->mainStrand->post(
        std::tr1::bind(&Proximity::handleRemoveAllObjectLocSubscription, this, object)
    );
}

} // namespace Sirikata
