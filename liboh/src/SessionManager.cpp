/*  Sirikata
 *  SessionManager.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include <sirikata/oh/SessionManager.hpp>
#include <sirikata/core/network/ServerIDMap.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/StreamFactory.hpp>
#include <sirikata/core/options/CommonOptions.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include "Protocol_Session.pbj.hpp"
#include <sirikata/core/util/Platform.hpp>

#define OH_LOG(level,msg) SILOG(oh,level,msg)

using namespace Sirikata::Network;

namespace Sirikata {

// ObjectInfo Implementation
SessionManager::ObjectConnections::ObjectInfo::ObjectInfo()
 : connectingTo(0),
   connectedTo(0),
   migratingTo(0),
   migratedCB(0)
{
}

// ObjectConnections Implementation

SessionManager::ObjectConnections::ObjectConnections(SessionManager* _parent)
 : parent(_parent)
{
}

void SessionManager::ObjectConnections::add(
    const SpaceObjectReference& sporef_objid, ConnectingInfo ci, InternalConnectedCallback connect_cb,
    MigratedCallback migrate_cb, StreamCreatedCallback stream_created_cb,
    DisconnectedCallback disconnected_cb
)
{
    // Make sure we have this object's info stored
    ObjectInfoMap::iterator it = mObjectInfo.find(sporef_objid);
    if (it != mObjectInfo.end()) {
        SILOG(oh,error,"Uh oh: this object connected twice "<<sporef_objid);
    }
    assert (it == mObjectInfo.end());
    it = mObjectInfo.insert( ObjectInfoMap::value_type( sporef_objid, ObjectInfo() ) ).first;
    ObjectInfo& obj_info = it->second;
    obj_info.connectingInfo = ci;
    obj_info.connectedCB = connect_cb;
    obj_info.migratedCB = migrate_cb;
    obj_info.streamCreatedCB = stream_created_cb;
    obj_info.disconnectedCB = disconnected_cb;
    // Add to reverse index // FIXME we need a real ObjectReference to use here
    //mInternalIDs[sporef_objid] = sporef_objid.object();
}

SessionManager::ConnectingInfo& SessionManager::ObjectConnections::connectingTo(const SpaceObjectReference& sporef_objid, ServerID connecting_to) {
    mObjectInfo[sporef_objid].connectingTo = connecting_to;
    return mObjectInfo[sporef_objid].connectingInfo;
}

void SessionManager::ObjectConnections::startMigration(const SpaceObjectReference& sporef_objid, ServerID migrating_to) {
    mObjectInfo[sporef_objid].migratingTo = migrating_to;

    // Update object indices
    std::vector<SpaceObjectReference>& sporef_objects = mObjectServerMap[mObjectInfo[sporef_objid].connectedTo];
    std::vector<SpaceObjectReference>::iterator sporef_where = std::find(sporef_objects.begin(), sporef_objects.end(), sporef_objid);
    if (sporef_where != sporef_objects.end()) {
        sporef_objects.erase(sporef_where);
    }

    // Notify the object
    mObjectInfo[sporef_objid].migratedCB(parent->mSpace, sporef_objid.object(), migrating_to);
}

SessionManager::InternalConnectedCallback& SessionManager::ObjectConnections::getConnectCallback(const SpaceObjectReference& sporef_objid) {
    return mObjectInfo[sporef_objid].connectedCB;
}

ServerID SessionManager::ObjectConnections::handleConnectSuccess(const SpaceObjectReference& sporef_obj, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& mesh, const String& phy, bool do_cb) {
    if (mObjectInfo[sporef_obj].connectingTo != NullServerID) { // We were connecting to a server
        ServerID connectedTo = mObjectInfo[sporef_obj].connectingTo;
        OH_LOG(detailed,"Successfully connected " << sporef_obj << " to space node " << connectedTo);
        mObjectInfo[sporef_obj].connectedTo = connectedTo;
        mObjectInfo[sporef_obj].connectingTo = NullServerID;
        mObjectServerMap[connectedTo].push_back(sporef_obj);

        mObjectInfo[sporef_obj].connectedAs = sporef_obj;

	ConnectingInfo ci;
	ci.loc = loc;
	ci.orient = orient;
	ci.bounds = bnds;
	ci.mesh = mesh;
	ci.physics = phy;

        if (do_cb) {
            mObjectInfo[sporef_obj].connectedCB(parent->mSpace, sporef_obj.object(), connectedTo, ci);
        }
        else {
            mDeferredCallbacks.push_back(
                std::tr1::bind(
                    mObjectInfo[sporef_obj].connectedCB,
                    parent->mSpace, sporef_obj.object(), connectedTo, ci
                )
            );
        }

        // FIXME shoudl be setting internal/external ID maps here
        // Look up internal ID so the OH can find the right object without
        // tracking space IDs
        //UUID dest_internal = getInternalID(ObjectReference(obj));
        parent->mObjectConnectedCallback(sporef_obj, connectedTo);
        return connectedTo;
    }
    else if (mObjectInfo[sporef_obj].migratingTo != NullServerID) { // We were migrating
        ServerID migratedFrom = mObjectInfo[sporef_obj].connectedTo;
        ServerID migratedTo = mObjectInfo[sporef_obj].migratingTo;

        OH_LOG(detailed,"Successfully migrated " << sporef_obj << " to space node " << migratedTo);
        mObjectInfo[sporef_obj].connectedTo = migratedTo;
        mObjectInfo[sporef_obj].migratingTo = NullServerID;
        mObjectServerMap[migratedTo].push_back(sporef_obj);

        //UUID dest_internal = getInternalID(ObjectReference(obj));
        parent->mObjectMigratedCallback(sporef_obj, migratedFrom, migratedTo);
        return migratedTo;
    }
    else { // What were we doing?
        OH_LOG(error,"Got connection response with no outstanding requests.");
        return NullServerID;
    }
}

void SessionManager::ObjectConnections::handleConnectError(const SpaceObjectReference& sporef_objid) {
    mObjectInfo[sporef_objid].connectingTo = NullServerID;
    ConnectingInfo ci;
    mObjectInfo[sporef_objid].connectedCB(parent->mSpace, sporef_objid.object(), NullServerID, ci);
}

void SessionManager::ObjectConnections::handleConnectStream(const SpaceObjectReference& sporef_objid, bool do_cb) {
    if (do_cb) {
        mObjectInfo[sporef_objid].streamCreatedCB( mObjectInfo[sporef_objid].connectedAs );
    }
    else {
        mDeferredCallbacks.push_back(
            std::tr1::bind(
                mObjectInfo[sporef_objid].streamCreatedCB,
                mObjectInfo[sporef_objid].connectedAs
            )
        );
    }
}

void SessionManager::ObjectConnections::remove(const SpaceObjectReference& sporef_objid) {
    // Update object indices
    std::vector<SpaceObjectReference>& sporef_objects = mObjectServerMap[mObjectInfo[sporef_objid].connectedTo];
    std::vector<SpaceObjectReference>::iterator sporef_where = std::find(sporef_objects.begin(), sporef_objects.end(), sporef_objid);
    if (sporef_where != sporef_objects.end()) {
        sporef_objects.erase(sporef_where);
    }

    // Remove from main object set
    mObjectInfo.erase(sporef_objid);
    // Remove from reverse index // FIXME need real object reference
    //mInternalIDs.erase( ObjectReference(objid) );
}

void SessionManager::ObjectConnections::handleUnderlyingDisconnect(ServerID sid, const String& reason) {
    ObjectServerMap::const_iterator server_it = mObjectServerMap.find(sid);
    if (server_it == mObjectServerMap.end())
        return;

    // We need to be careful invoking the callbacks for these. Since
    // the callbacks may make calls back to us, we need to make sure
    // we have a copy of all the data we need.
    typedef std::vector<SpaceObjectReference> SporefVector;
    // NOTE: Copy so iterators stay valid
    SporefVector sporef_objects = server_it->second;

    for(SporefVector::const_iterator sporef_obj_it = sporef_objects.begin(); sporef_obj_it != sporef_objects.end(); sporef_obj_it++) {
        SpaceObjectReference sporef_obj = *sporef_obj_it;
        assert(mObjectInfo.find(sporef_obj) != mObjectInfo.end());
        mObjectInfo[sporef_obj].disconnectedCB(mObjectInfo[sporef_obj].connectedAs, Disconnect::Forced);
        parent->mObjectDisconnectedCallback(sporef_obj, Disconnect::Forced);
    }
}

ServerID SessionManager::ObjectConnections::getConnectedServer(const SpaceObjectReference& sporef_obj_id, bool allow_connecting) {
    // FIXME getConnectedServer during migrations?

    ServerID dest_server = mObjectInfo[sporef_obj_id].connectedTo;

    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[sporef_obj_id].connectingTo;

    return dest_server;
}


void SessionManager::ObjectConnections::invokeDeferredCallbacks() {
    for(DeferredCallbackList::iterator it = mDeferredCallbacks.begin(); it != mDeferredCallbacks.end(); it++)
        (*it)();
    mDeferredCallbacks.clear();
}

// SessionManager Implementation

SessionManager::SessionManager(
    ObjectHostContext* ctx, const SpaceID& space, ServerIDMap* sidmap,
    ObjectConnectedCallback conn_cb, ObjectMigratedCallback mig_cb,
    ObjectMessageHandlerCallback msg_cb, ObjectDisconnectedCallback disconn_cb
)
 : ODP::DelegateService( std::tr1::bind(&SessionManager::createDelegateODPPort, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2,  std::tr1::placeholders::_3) ),
   mContext( ctx ),
   mSpace(space),
   mIOStrand( ctx->ioService->createStrand() ),
   mServerIDMap(sidmap),
   mObjectConnectedCallback(conn_cb),
   mObjectMigratedCallback(mig_cb),
   mObjectMessageHandlerCallback(msg_cb),
   mObjectDisconnectedCallback(disconn_cb),
   mObjectConnections(this),
   mTimeSyncClient(NULL),
   mShuttingDown(false)
{
    mStreamOptions=Sirikata::Network::StreamFactory::getSingleton().getOptionParser(GetOptionValue<String>("ohstreamlib"))(GetOptionValue<String>("ohstreamoptions"));

    mHandleReadProfiler = mContext->profiler->addStage("Handle Read Network");
    mHandleMessageProfiler = mContext->profiler->addStage("Handle Server Message");
}


SessionManager::~SessionManager() {
    delete mTimeSyncClient;

    // Close all connections
    for (ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        delete conn;
    }
    mConnections.clear();

    delete mHandleReadProfiler;
    delete mHandleMessageProfiler;

    delete mIOStrand;
}

void SessionManager::start() {
}

void SessionManager::stop() {
    mShuttingDown = true;

    // Stop processing of all connections
    for (ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        conn->shutdown();
    }
}

void SessionManager::connect(
    const SpaceObjectReference& sporef_objid,
    const TimedMotionVector3f& init_loc, const TimedMotionQuaternion& init_orient, const BoundingSphere3f& init_bounds,
    bool regQuery, const SolidAngle& init_sa, const String& init_mesh, const String& init_phy,
    ConnectedCallback connect_cb, MigratedCallback migrate_cb,
    StreamCreatedCallback stream_created_cb, DisconnectedCallback disconn_cb
)
{
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;
    using std::tr1::placeholders::_4;
    using std::tr1::placeholders::_5;
    using std::tr1::placeholders::_6;
    using std::tr1::placeholders::_7;
    using std::tr1::placeholders::_8;

    ConnectingInfo ci;
    ci.loc = init_loc;
    ci.orient = init_orient;
    ci.bounds = init_bounds;
    ci.regQuery = regQuery;
    ci.queryAngle = init_sa;
    ci.mesh = init_mesh;
    ci.physics = init_phy;


    // connect_cb gets wrapped so we can start some automatic steps (initial
    // connection of sst stream to space) at the correc time
    mObjectConnections.add(
        sporef_objid, ci,
        std::tr1::bind(&SessionManager::handleObjectFullyConnected, this,
		       _1, _2, _3, ci,
            connect_cb
        ),
        std::tr1::bind(&SessionManager::handleObjectFullyMigrated, this,
		       _1, _2, _3,
            migrate_cb
        ),
        stream_created_cb, disconn_cb
    );

    // Get a connection to request
    getAnySpaceConnection(
        std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_objid, _1)
    );
}

void SessionManager::disconnect(const SpaceObjectReference& sporef_objid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    // Construct and send disconnect message.  This has to happen first so we still have
    // connection information so we know where to send the disconnect
    Sirikata::Protocol::Session::Container session_msg;
    Sirikata::Protocol::Session::IDisconnect disconnect_msg = session_msg.mutable_disconnect();
    disconnect_msg.set_object(sporef_objid.object().getAsUUID());
    disconnect_msg.set_reason("Quit");

    send(sporef_objid, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg)
    );
    // FIXME do something on failure

    mObjectConnections.remove(sporef_objid);
}

Duration SessionManager::serverTimeOffset() const {
    assert(mTimeSyncClient != NULL);
    return mTimeSyncClient->offset();
}

Duration SessionManager::clientTimeOffset() const {
    assert(mTimeSyncClient != NULL);
    return -(mTimeSyncClient->offset());
}

void SessionManager::retryOpenConnection(const SpaceObjectReference&sporef_uuid,ServerID sid) {
    using std::tr1::placeholders::_1;

    getSpaceConnection(
        sid,
        std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_uuid, _1)
    );
}
void SessionManager::openConnectionStartSession(const SpaceObjectReference& sporef_uuid, SpaceNodeConnection* conn) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (conn == NULL) {
        OH_LOG(warn,"Couldn't initiate connection for " << sporef_uuid);
        // FIXME disconnect? retry?
	ConnectingInfo ci;
        mObjectConnections.getConnectCallback(sporef_uuid)(mSpace, ObjectReference::null(), NullServerID, ci);
        return;
    }

    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    ConnectingInfo ci = mObjectConnections.connectingTo(sporef_uuid, conn->server());

    Sirikata::Protocol::Session::Container session_msg;
    Sirikata::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(Sirikata::Protocol::Session::Connect::Fresh);
    connect_msg.set_object(sporef_uuid.object().getAsUUID());
    Sirikata::Protocol::ITimedMotionVector loc = connect_msg.mutable_loc();
    loc.set_t( ci.loc.updateTime() );
    loc.set_position( ci.loc.position() );
    loc.set_velocity( ci.loc.velocity() );
    Sirikata::Protocol::ITimedMotionQuaternion orient = connect_msg.mutable_orientation();
    orient.set_t( ci.orient.updateTime() );
    orient.set_position( ci.orient.position() );
    orient.set_velocity( ci.orient.velocity() );
    connect_msg.set_bounds( ci.bounds );


   if (ci.regQuery)
       connect_msg.set_query_angle( ci.queryAngle.asFloat() );




    if (ci.mesh.size() > 0)
        connect_msg.set_mesh( ci.mesh );

    if (ci.physics.size() > 0)
        connect_msg.set_physics( ci.physics );

    if (!send(sporef_uuid, OBJECT_PORT_SESSION,
              UUID::null(), OBJECT_PORT_SESSION,
              serializePBJMessage(session_msg),
            conn->server()
            )) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&SessionManager::retryOpenConnection,this,sporef_uuid,conn->server()));
    }
}


void SessionManager::migrate(const SpaceObjectReference& sporef_obj_id, ServerID sid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;

    OH_LOG(insane,"Starting migration of " << sporef_obj_id << " to " << sid);

    //forcibly close the SST connection for this object to its current previous
    //space server
    //ObjectReference objref(sporef_obj_id.object());
    if (mObjectToSpaceStreams.find(sporef_obj_id.object()) != mObjectToSpaceStreams.end()) {
      std::cout << "deleting object-space streams  of " << sporef_obj_id << " to " << sid << "\n";
      mObjectToSpaceStreams[sporef_obj_id.object()]->connection().lock()->close(true);
      mObjectToSpaceStreams.erase(sporef_obj_id.object());
    }

    mObjectConnections.startMigration(sporef_obj_id, sid);

    // Get or start the connection we need to start this migration
    getSpaceConnection(
        sid,
        std::tr1::bind(&SessionManager::openConnectionStartMigration, this, sporef_obj_id, sid, _1)
    );
}

void SessionManager::openConnectionStartMigration(const SpaceObjectReference& sporef_obj_id, ServerID sid, SpaceNodeConnection* conn) {
    using std::tr1::placeholders::_1;
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (conn == NULL) {
        OH_LOG(warn,"Couldn't open connection to server " << sid << " for migration of object " << sporef_obj_id);
        // FIXME disconnect? retry?
        return;
    }

    Sirikata::Protocol::Session::Container session_msg;
    Sirikata::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(Sirikata::Protocol::Session::Connect::Migration);
    connect_msg.set_object(sporef_obj_id.object().getAsUUID());
    if (!send(
            sporef_obj_id, OBJECT_PORT_SESSION,
            UUID::null(), OBJECT_PORT_SESSION,
            serializePBJMessage(session_msg),
            sid
            ))    {
        std::tr1::function<void(SpaceNodeConnection*)> retry(std::tr1::bind(&SessionManager::openConnectionStartMigration,
                                                                          this,
                                                                          sporef_obj_id,
                                                                          sid,
                                                                          _1));
        mContext->mainStrand
            ->post(Duration::seconds(.05),
                   std::tr1::bind(&SessionManager::getSpaceConnection,
                                  this,
                                  sid,
                                  retry));
    }

    // FIXME do something on failure
}

ODP::DelegatePort* SessionManager::createDelegateODPPort(DelegateService*, const SpaceObjectReference& sor, ODP::PortID port) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    ODP::Endpoint port_ep(sor, port);
    return new ODP::DelegatePort(
        (ODP::DelegateService*)this,
        port_ep,
        std::tr1::bind(
            &SessionManager::delegateODPPortSend, this, port_ep, _1, _2
        )
    );
}

bool SessionManager::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    // Create new ObjectMessage
    return send(
        SpaceObjectReference(source_ep.space(),source_ep.object()), (uint16)source_ep.port(),
        dest_ep.object().getAsUUID(), (uint16)dest_ep.port(),
        String((char*)payload.data(), payload.size())
    );
}

void SessionManager::timeSyncUpdated() {
    assert( mTimeSyncClient->valid() );
    mObjectConnections.invokeDeferredCallbacks();
}

bool SessionManager::send(const SpaceObjectReference& sporef_src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mShuttingDown)
        return false;

    if (dest_server == NullServerID) {
        // Try looking it up
        // Special case TIMESYNC messages. Just use any space server to try
        // syncing with.
        if (dest_port == OBJECT_PORT_TIMESYNC && !mConnections.empty())
            dest_server = mConnections.begin()->first;
        else
            dest_server = mObjectConnections.getConnectedServer(sporef_src);
        // And if we still don't have something, give up
        if (dest_server == NullServerID) {
            OH_LOG(error,"Tried to send message when not connected.");
            return false;
        }
    }

    ServerConnectionMap::iterator it = mConnections.find(dest_server);
    if (it == mConnections.end()) {
        OH_LOG(warn,"Tried to send message for object to unconnected server.");
        return false;
    }
    SpaceNodeConnection* conn = it->second;

    // FIXME would be nice not to have to do this alloc/dealloc
    ObjectMessage obj_msg;
    createObjectHostMessage(mContext->id, sporef_src, src_port, dest, dest_port, payload, &obj_msg);
    TIMESTAMP_CREATED((&obj_msg), Trace::CREATED);
    return conn->push(obj_msg);
}

void SessionManager::sendRetryingMessage(const SpaceObjectReference& sporef_src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server, Network::IOStrand* strand, const Duration& rate) {
    bool sent = send(
        sporef_src, src_port,
        dest, dest_port,
        payload,
        dest_server);

    if (!sent) {
        strand->post(
            rate,
            std::tr1::bind(&SessionManager::sendRetryingMessage, this,
                sporef_src, src_port,
                dest, dest_port,
                payload,
                dest_server,
                strand, rate)
        );
    }
}

void SessionManager::getAnySpaceConnection(SpaceNodeConnection::GotSpaceConnectionCallback cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mShuttingDown) {
        cb(NULL);
        return;
    }

    // Check if we have any fully open connections we can already use
    if (!mConnections.empty()) {
        for(ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
            SpaceNodeConnection* rand_conn = it->second;
            if (rand_conn == NULL) continue;
            if (!rand_conn->connecting()) {
                cb(rand_conn);
                return;
            }
            else {
                rand_conn->addCallback(cb);
                return;
            }
        }
    }

    // Otherwise, initiate one at random
    ServerID server_id = (ServerID)1; // FIXME should be selected at random somehow, and shouldn't already be in the connection map
    getSpaceConnection(server_id, cb);
}

void SessionManager::getSpaceConnection(ServerID sid, SpaceNodeConnection::GotSpaceConnectionCallback cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mShuttingDown) {
        cb(NULL);
        return;
    }

    // Check if we have the connection already
    ServerConnectionMap::iterator it = mConnections.find(sid);
    if (it != mConnections.end()) {
        cb(it->second);
        return;
    }

    // Otherwise start it up
    setupSpaceConnection(sid, cb);
}

void SessionManager::setupSpaceConnection(ServerID server, SpaceNodeConnection::GotSpaceConnectionCallback cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // Lookup the server's address
    Address4* addr = mServerIDMap->lookupExternal(server);
    Address addy(convertAddress4ToSirikata(*addr));

    SpaceNodeConnection* conn = new SpaceNodeConnection(
        mContext,
        mIOStrand,
        mHandleReadProfiler,
        mStreamOptions,
        mSpace,
        server,
        addy,
        mContext->mainStrand->wrap(
            std::tr1::bind(&SessionManager::handleSpaceConnection,
                this,
                _1, _2, server
            )
        ),
        std::tr1::bind(&SessionManager::scheduleHandleServerMessages, this, _1)
    );
    conn->addCallback(cb);
    mConnections[server] = conn;

    conn->connect();

    OH_LOG(detailed,"Trying to connect to " << addy.toString());
}

void SessionManager::handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                                       const std::string&reason,
                                       ServerID sid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    ServerConnectionMap::iterator conn_it = mConnections.find(sid);
    if (conn_it == mConnections.end())
        return;
    SpaceNodeConnection* conn = conn_it->second;

    OH_LOG(detailed,"Handling space connection event...");

    if (status == Sirikata::Network::Stream::Connected) {
        OH_LOG(detailed,"Successfully connected to " << sid);

        // Try to setup time syncing if it isn't on yet.
        if (mTimeSyncClient == NULL) {
            // The object IDs and ports used are bogus since the space server just
            // ignores them
            mTimeSyncClient = new TimeSyncClient(
                mContext, this->bindODPPort(mSpace, ObjectReference(UUID::random()), OBJECT_PORT_TIMESYNC),
                ODP::Endpoint(mSpace, ObjectReference::spaceServiceID(), OBJECT_PORT_TIMESYNC),
                Duration::seconds(5),
                std::tr1::bind(&SessionManager::timeSyncUpdated, this)
            );
            mContext->add(mTimeSyncClient);
        }
    }
    else if (status == Sirikata::Network::Stream::ConnectionFailed) {
        OH_LOG(error,"Failed to connect to server " << sid << ": " << reason);
        delete conn;
        mConnections.erase(sid);
        return;    }
    else if (status == Sirikata::Network::Stream::Disconnected) {
        OH_LOG(error,"Disconnected from server " << sid << ": " << reason);
        delete conn;
        mConnections.erase(sid);
        // Notify connected objects
        mObjectConnections.handleUnderlyingDisconnect(sid, reason);
        // If we have no connections left, we have to give up on TimeSync
        if (mConnections.empty() && mTimeSyncClient != NULL) {
            mContext->remove(mTimeSyncClient);
            mTimeSyncClient->stop();
            delete mTimeSyncClient;
            mTimeSyncClient = NULL;
        }
        return;
    }
}

void SessionManager::scheduleHandleServerMessages(SpaceNodeConnection* conn) {
    mContext->mainStrand->post(
        std::tr1::bind(&SessionManager::handleServerMessages, this, conn)
    );
}

void SessionManager::handleServerMessages(SpaceNodeConnection* conn) {
#define MAX_HANDLE_SERVER_MESSAGES 20
    mHandleMessageProfiler->started();

    for(uint32 ii = 0; ii < MAX_HANDLE_SERVER_MESSAGES; ii++) {
        // Pull it off the queue
        ObjectMessage* msg = conn->pull();
        if (msg == NULL) {
            mHandleMessageProfiler->finished();
            return;
        }
        handleServerMessage(msg);
    }

    if (!conn->empty())
        scheduleHandleServerMessages(conn);

    mHandleMessageProfiler->finished();
}

void SessionManager::handleServerMessage(ObjectMessage* msg) {
    TIMESTAMP_START(tstamp, msg);

    // Mark as received
    TIMESTAMP_END(tstamp, Trace::OH_RECEIVED);

    // As a special case, messages dealing with sessions are handled by the object host
    if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_SESSION) {
        handleSessionMessage(msg);
    }
    else if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_TIMESYNC) {
        // A second special case, messages dealing with OH <-> Space time
        // synchronization, shared between all objects connected to a space
        // Deliver it via this DelegateService
        this->deliver(
            ODP::Endpoint(mSpace, ObjectReference(msg->source_object()), ODP::PortID(msg->source_port())),
            ODP::Endpoint(mSpace, ObjectReference(msg->dest_object()), ODP::PortID(msg->dest_port())),
            MemoryReference(msg->payload())
        );
    }
    else {
        // Look up internal ID so the OH can find the right object without
        // tracking space IDs
        //UUID dest_internal = mObjectConnections.getInternalID(ObjectReference(msg->dest_object()));
        mObjectMessageHandlerCallback(SpaceObjectReference(mSpace,ObjectReference(msg->dest_object())), msg);
    }

    TIMESTAMP_END(tstamp, Trace::DESTROYED);
}

void SessionManager::handleSessionMessage(Sirikata::Protocol::Object::ObjectMessage* msg) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    Sirikata::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg->payload());
    if (!parse_success) {
        LOG_INVALID_MESSAGE(session, error, msg->payload());
        delete msg;
        return;
    }

    assert(!session_msg.has_connect());
    SpaceObjectReference sporef_obj(mSpace,ObjectReference(msg->dest_object()));

    if (session_msg.has_connect_response()) {
        Sirikata::Protocol::Session::ConnectResponse conn_resp = session_msg.connect_response();

        //UUID obj = msg->dest_object();

        if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Success)
        {
            TimedMotionVector3f loc(
                conn_resp.loc().t(),
                MotionVector3f(conn_resp.loc().position(), conn_resp.loc().velocity())
            );
            TimedMotionQuaternion orient(
                conn_resp.orientation().t(),
                MotionQuaternion( conn_resp.orientation().position(), conn_resp.orientation().velocity() )
            );
            BoundingSphere3f bnds = conn_resp.bounds();
            String mesh = "";
            if(conn_resp.has_mesh())
               mesh = conn_resp.mesh();
            String phy = "";
            if(conn_resp.has_physics())
               phy = conn_resp.physics();
            // If we've got proper time syncing setup, we can dispatch the callback
            // immediately.  Otherwise, we need to delay the callback
            assert(mTimeSyncClient != NULL);
            bool time_synced = mTimeSyncClient->valid();

	    ServerID connected_to = mObjectConnections.handleConnectSuccess(sporef_obj, loc, orient, bnds, mesh, phy, time_synced);

	    // Send an ack so the server (our first conn or after migrating) can start sending data to us
	    Sirikata::Protocol::Session::Container ack_msg;
	    Sirikata::Protocol::Session::IConnectAck connect_ack_msg = ack_msg.mutable_connect_ack();
	    sendRetryingMessage(
				sporef_obj, OBJECT_PORT_SESSION,
				UUID::null(), OBJECT_PORT_SESSION,
				serializePBJMessage(ack_msg),
				connected_to,
				mContext->mainStrand,
				Duration::seconds(0.05)
				);
        }
        else if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Redirect) {
            ServerID redirected = conn_resp.redirect();
            OH_LOG(detailed,"Object connection for " << sporef_obj << " redirected to " << redirected);
            // Get a connection to request
            getSpaceConnection(
                redirected,
                std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_obj, std::tr1::placeholders::_1)
            );
        }
        else if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Error) {
            OH_LOG(error,"Error connecting " << sporef_obj << " to space");
            mObjectConnections.handleConnectError(sporef_obj);
        }
        else {
            OH_LOG(error,"Unknown connection response code: " << (int)conn_resp.response());
        }
    }

    if (session_msg.has_init_migration()) {
        Sirikata::Protocol::Session::InitiateMigration init_migr = session_msg.init_migration();
        OH_LOG(insane,"Received migration request for " << sporef_obj << " to " << init_migr.new_server());
        migrate(sporef_obj, init_migr.new_server());
    }

    delete msg;
}

void SessionManager::handleObjectFullyConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const ConnectingInfo& ci, ConnectedCallback real_cb) {
    SpaceObjectReference spaceobj(space, obj);

    ConnectionInfo conn_info;
    conn_info.server = server;
    conn_info.loc = ci.loc;
    conn_info.orient = ci.orient;
    conn_info.bounds = ci.bounds;
    conn_info.mesh = ci.mesh;
    conn_info.physics = ci.physics;
    real_cb(space, obj, conn_info);

    SSTStream::connectStream(
        SSTEndpoint(spaceobj, 0), // Local port is random
        SSTEndpoint(SpaceObjectReference(space, ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
        std::tr1::bind( &SessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj)
    );
}

void SessionManager::handleObjectFullyMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server, MigratedCallback real_cb) {
    SpaceObjectReference spaceobj(space, obj);

    real_cb(space, obj, server);

    SSTStream::connectStream(
        SSTEndpoint(spaceobj, 0), // Local port is random
        SSTEndpoint(SpaceObjectReference(space, ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
        std::tr1::bind( &SessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj)
    );
}

SessionManager::SSTStreamPtr SessionManager::getSpaceStream(const ObjectReference& objectID) {
  if (mObjectToSpaceStreams.find(objectID) != mObjectToSpaceStreams.end()) {
    return mObjectToSpaceStreams[objectID];
  }

  return SSTStreamPtr();
}


void SessionManager::spaceConnectCallback(int err, SSTStreamPtr s, SpaceObjectReference spaceobj) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    OH_LOG(detailed, "SST object-space connect callback for " << spaceobj.toString() << " : " << err << "\n");

    if (err != SST_IMPL_SUCCESS) {
        // retry creating an SST stream from the space server to object 'obj'.
        SSTStream::connectStream(
            SSTEndpoint(spaceobj, 0), // Local port is random
            SSTEndpoint(SpaceObjectReference(spaceobj.space(), ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
            std::tr1::bind( &SessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj)
        );
        return;
    }

    mObjectToSpaceStreams[spaceobj.object()] = s;


    assert(mTimeSyncClient != NULL);
    bool time_synced = mTimeSyncClient->valid();
    mObjectConnections.handleConnectStream(spaceobj, time_synced);
}

} // namespace Sirikata
