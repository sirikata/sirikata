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
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include "Protocol_Session.pbj.hpp"
#include <sirikata/core/util/Platform.hpp>
#ifdef _WIN32
#pragma warning (disable:4355)//this within constructor initializer
#endif

#define SESSION_LOG(level,msg) SILOG(session,level,msg)

using namespace Sirikata::Network;

namespace Sirikata {

namespace {
// Helper for filling in version info for connection responses
void fillVersionInfo(Sirikata::Protocol::Session::IVersionInfo vers_info, ObjectHostContext* ctx) {
    vers_info.set_name(ctx->name());
    vers_info.set_version(SIRIKATA_VERSION);
    vers_info.set_major(SIRIKATA_VERSION_MAJOR);
    vers_info.set_minor(SIRIKATA_VERSION_MINOR);
    vers_info.set_revision(SIRIKATA_VERSION_REVISION);
    vers_info.set_vcs_version(SIRIKATA_GIT_REVISION);
}
void logVersionInfo(Sirikata::Protocol::Session::VersionInfo vers_info) {
    SESSION_LOG(info, "Connection to space server " << (vers_info.has_name() ? vers_info.name() : "(unknown)") << " version " << (vers_info.has_version() ? vers_info.version() : "(unknown)") << " (" << (vers_info.has_vcs_version() ? vers_info.vcs_version() : "") << ")");
}
} // namespace


// ObjectInfo Implementation
SessionManager::ObjectConnections::ObjectInfo::ObjectInfo()
 : seqno(0),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0),
   connectedAs(SpaceObjectReference::null()),
   migratedCB(0)
{
}

// ObjectConnections Implementation

SessionManager::ObjectConnections::ObjectConnections(SessionManager* _parent)
 : parent(_parent),
   mSeqnoSource(1)
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
        SESSION_LOG(error,"Uh oh: this object connected twice "<<sporef_objid);
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

bool SessionManager::ObjectConnections::exists(const SpaceObjectReference& sporef_objid) {
    return mObjectInfo.find(sporef_objid) != mObjectInfo.end();
}

void SessionManager::ObjectConnections::clearSeqno(const SpaceObjectReference& sporef_objid) {
    // Clear should only occur when the session is done, i.e. there
    // was a disconnection of some sort. Migrations don't cause this
    // because they just transition directly to a new session seqno.
    assert(mObjectInfo[sporef_objid].seqno != 0);
    mObjectInfo[sporef_objid].seqno = 0;
}

uint64 SessionManager::ObjectConnections::getSeqno(const SpaceObjectReference& sporef_objid) {
    // Should only request when we have an outstanding request
    assert(mObjectInfo[sporef_objid].seqno != 0);
    return mObjectInfo[sporef_objid].seqno;
}

uint64 SessionManager::ObjectConnections::updateSeqno(const SpaceObjectReference& sporef_objid) {
    uint64 seqno = mSeqnoSource++;
    mObjectInfo[sporef_objid].seqno = seqno;
    return seqno;
}

SessionManager::ConnectingInfo& SessionManager::ObjectConnections::connectingTo(const SpaceObjectReference& sporef_objid, ServerID connecting_to) {
    if (mObjectInfo[sporef_objid].connectedTo==NullServerID) {
        mObjectInfo[sporef_objid].connectingTo = connecting_to;
    }else if (connecting_to!=mObjectInfo[sporef_objid].connectedTo) {
        // TODO(ewencp) uhhh, this doesn't look right. How can we just unset
        // connectedTo without requesting a disconnect or migration?
        mObjectInfo[sporef_objid].connectedTo = NullServerID;
    }else {
        //SILOG(oh,error,"Interesting case hit");
    }
    return mObjectInfo[sporef_objid].connectingInfo;
}


void SessionManager::ObjectConnections::startMigration(const SpaceObjectReference& sporef_objid, ServerID migrating_to) {
    mObjectInfo[sporef_objid].migratingTo = migrating_to;
    mObjectInfo[sporef_objid].connectingTo = NullServerID;
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
        SESSION_LOG(detailed,"Successfully connected " << sporef_obj << " to space node " << connectedTo<< " migrating to "<<mObjectInfo[sporef_obj].migratingTo);
        mObjectInfo[sporef_obj].connectedTo = connectedTo;
        mObjectInfo[sporef_obj].connectingTo = NullServerID;
        if (connectedTo == mObjectInfo[sporef_obj].migratingTo) {
            mObjectInfo[sporef_obj].migratingTo = NullServerID;
        }
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

        SESSION_LOG(detailed,"Successfully migrated " << sporef_obj << " to space node " << migratedTo);
        mObjectInfo[sporef_obj].connectedTo = migratedTo;
        mObjectInfo[sporef_obj].connectingTo = NullServerID;
        mObjectInfo[sporef_obj].migratingTo = NullServerID;
        mObjectServerMap[migratedTo].push_back(sporef_obj);

        //UUID dest_internal = getInternalID(ObjectReference(obj));
        parent->mObjectMigratedCallback(sporef_obj, migratedFrom, migratedTo);
        return migratedTo;
    }
    else { // What were we doing?
        SESSION_LOG(error,"Got connection response with no outstanding requests.");
        return NullServerID;
    }
}

void SessionManager::ObjectConnections::handleConnectError(const SpaceObjectReference& sporef) {
    mObjectInfo[sporef].connectingTo = NullServerID;
    // Since the connection failed, the session seqno is no longer valid
    clearSeqno(sporef);
    //ConnectingInfo ci;
    //mObjectInfo[sporef_objid].connectedCB(parent->mSpace, sporef_objid.object(), NullServerID, ci);
    disconnectWithCode(sporef,sporef,Disconnect::LoginDenied);
}

void SessionManager::ObjectConnections::handleConnectStream(const SpaceObjectReference& sporef_objid, ConnectionEvent after, bool do_cb) {
    if (do_cb) {
        mObjectInfo[sporef_objid].streamCreatedCB( mObjectInfo[sporef_objid].connectedAs, after );
    }
    else {
        mDeferredCallbacks.push_back(
            std::tr1::bind(
                mObjectInfo[sporef_objid].streamCreatedCB,
                mObjectInfo[sporef_objid].connectedAs,
                after
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
        disconnectWithCode(sporef_obj,mObjectInfo[sporef_obj].connectedAs, Disconnect::Forced);
    }
}
void SessionManager::ObjectConnections::disconnectWithCode(const SpaceObjectReference& sporef, const SpaceObjectReference& connectedAs, Disconnect::Code code) {
    ///DO NOT reorder this copy. connectedAs may come from a MFD item (eg remove(sporef will invalidate connectedAs) )
    SpaceObjectReference tmp_sporef=connectedAs==SpaceObjectReference::null()?sporef:connectedAs;
    //end DO NOT reorder :-) everything after this point should be ok.
    if (mObjectInfo.find(sporef)!=mObjectInfo.end()) {
        DisconnectedCallback disconFunc=mObjectInfo[sporef].disconnectedCB;
        remove(sporef);
        if (disconFunc)
            disconFunc(tmp_sporef, code);
        else
            SILOG(oh,error,"Disconnection callback for "<<sporef.toString()<<" is null");
        parent->mObjectDisconnectedCallback(sporef, code);
    }
}
void SessionManager::ObjectConnections::gracefulDisconnect(const SpaceObjectReference& sporef) {
    ObjectInfoMap::iterator wherei=mObjectInfo.find(sporef);
    if (wherei!=mObjectInfo.end()) {
        disconnectWithCode(sporef,wherei->second.connectedAs, Disconnect::Requested);
    }
}

ServerID SessionManager::ObjectConnections::getConnectedServer(const SpaceObjectReference& sporef_obj_id, bool allow_connecting) {
    // FIXME getConnectedServer during migrations?

    ServerID dest_server = mObjectInfo[sporef_obj_id].connectedTo;

    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[sporef_obj_id].connectingTo;

    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[sporef_obj_id].migratingTo;

    return dest_server;
}

ServerID SessionManager::ObjectConnections::getConnectingToServer(const SpaceObjectReference& sporef_obj_id) {
    return mObjectInfo[sporef_obj_id].connectingTo;
}

ServerID SessionManager::ObjectConnections::getMigratingToServer(const SpaceObjectReference& sporef_obj_id)
{
    return mObjectInfo[sporef_obj_id].migratingTo;
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
 : PollingService(ctx->mainStrand, "SessionManager Poll", Duration::seconds(1.f), ctx, "Session Manager"),
   OHDP::DelegateService( std::tr1::bind(&SessionManager::createDelegateOHDPPort, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2) ),
   mContext( ctx ),
   mSpace(space),
   mIOStrand( ctx->ioService->createStrand("SessionManager") ),
   mServerIDMap(sidmap),
   mObjectConnectedCallback(conn_cb),
   mObjectMigratedCallback(mig_cb),
   mObjectMessageHandlerCallback(msg_cb),
   mObjectDisconnectedCallback(disconn_cb),
   mObjectConnections(this),
   mTimeSyncClient(NULL),
   mShuttingDown(false)
#ifdef PROFILE_OH_PACKET_RTT
   ,
   mClearOutstandingCount(0),
   mLatencySum(),
   mLatencyCount(0),
   mTimeSeriesOHRTT(String("oh.server") + boost::lexical_cast<String>(ctx->id) + ".rtt_latency")
#endif
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
    PollingService::start();
}

void SessionManager::stop() {
    PollingService::stop();

    mShuttingDown = true;

    // Stop processing of all connections
    for (ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        conn->shutdown();
    }
}

void SessionManager::poll() {
#ifdef PROFILE_OH_PACKET_RTT
    mContext->timeSeries->report(
        mTimeSeriesOHRTT,
        (mLatencyCount > 0) ? mLatencySum.toMicroseconds() / mLatencyCount : 0.f
    );

    // Not perfect, and assumes we won't see > 5s latencies, but we
    // need to clear it out periodically to make sure we don't eat up
    // too much memory.
    mClearOutstandingCount++;
    if (mClearOutstandingCount == 5) {
        mOutstandingPackets.clear();
        mClearOutstandingCount = 0;
    }

    mLatencySum = Duration::microseconds(0);
    mLatencyCount = 0;
#endif
}

bool SessionManager::connect(
    const SpaceObjectReference& sporef_objid,
    const TimedMotionVector3f& init_loc, const TimedMotionQuaternion& init_orient, const BoundingSphere3f& init_bounds,
    const String& init_mesh, const String& init_phy, const String& init_query,
    ConnectedCallback connect_cb, MigratedCallback migrate_cb,
    StreamCreatedCallback stream_created_cb, DisconnectedCallback disconn_cb
)
{
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mObjectConnections.exists(sporef_objid)) {
        return false;
    }

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
    ci.query = init_query;
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
    SESSION_LOG(detailed, "Connection starting for " << sporef_objid);
    getAnySpaceConnection(
        std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_objid, _1, false)
    );
    return true;
}

void SessionManager::disconnect(const SpaceObjectReference& sporef_objid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    ServerID connected_to = mObjectConnections.getConnectedServer(sporef_objid);
    uint64 session_seqno = mObjectConnections.getSeqno(sporef_objid);
    // The caller may be conservative in calling disconnect, especially to make
    // sure disconnections are actually forced (e.g. for safety in the case of a
    // half-complete connection where the initial success is reported but
    // streams aren't done yet).
    if (connected_to == NullServerID)
        return;

    sendDisconnectMessage(sporef_objid, connected_to, session_seqno);

    // TODO(ewencp) we should probably keep around a record of this
    // and get an ack from the space server. Otherwise we could end up
    // with connections that get stuck if this OH remains connected to
    // the OH

    // Notify of disconnect (requested), then remove
    mObjectConnections.gracefulDisconnect(sporef_objid);
}

void SessionManager::sendDisconnectMessage(const SpaceObjectReference& sporef_objid, ServerID connected_to, uint64 session_seqno) {
    // Construct and send disconnect message.  This has to happen first so we still have
    // connection information so we know where to send the disconnect
    Sirikata::Protocol::Session::Container session_msg;
    if (session_seqno != 0) session_msg.set_seqno(session_seqno);
    Sirikata::Protocol::Session::IDisconnect disconnect_msg = session_msg.mutable_disconnect();
    disconnect_msg.set_object(sporef_objid.object().getAsUUID());
    disconnect_msg.set_reason("Quit");

    // We just need to make sure it gets on the queue, once its on the space
    // server guarantees processing since it is a session message
    sendRetryingMessage(
        sporef_objid, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg),
        connected_to, mContext->mainStrand, Duration::seconds(0.05)
    );
}

Duration SessionManager::serverTimeOffset() const {
    assert(mTimeSyncClient != NULL);
    assert(mTimeSyncClient->valid());
    return mTimeSyncClient->offset();
}

Duration SessionManager::clientTimeOffset() const {
    assert(mTimeSyncClient != NULL);
    assert(mTimeSyncClient->valid());
    return -(mTimeSyncClient->offset());
}

void SessionManager::retryOpenConnection(const SpaceObjectReference&sporef_uuid,ServerID sid) {
    using std::tr1::placeholders::_1;

    getSpaceConnection(
        sid,
        std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_uuid, _1, true)
    );
}

void SessionManager::openConnectionStartSession(const SpaceObjectReference& sporef_uuid, SpaceNodeConnection* conn, bool is_retry) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (conn == NULL) {
        SESSION_LOG(warn,"Couldn't initiate connection for " << sporef_uuid);
        // FIXME disconnect? retry?
	ConnectingInfo ci;
        mObjectConnections.getConnectCallback(sporef_uuid)(mSpace, ObjectReference::null(), NullServerID, ci);
        return;
    }

    SESSION_LOG(detailed, "Base connection to space server obtained, initiating session " << sporef_uuid);
    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    ConnectingInfo ci = mObjectConnections.connectingTo(sporef_uuid, conn->server());

    Sirikata::Protocol::Session::Container session_msg;
    uint64 seqno = is_retry ? mObjectConnections.getSeqno(sporef_uuid) : mObjectConnections.updateSeqno(sporef_uuid);
    session_msg.set_seqno(seqno);
    Sirikata::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    fillVersionInfo(connect_msg.mutable_version(), mContext);
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

    if (ci.mesh.size() > 0)
        connect_msg.set_mesh( ci.mesh );

    if (ci.physics.size() > 0)
        connect_msg.set_physics( ci.physics );

    if (ci.query.size() > 0)
        connect_msg.set_query_parameters( ci.query );

    if (!send(sporef_uuid, OBJECT_PORT_SESSION,
              UUID::null(), OBJECT_PORT_SESSION,
              serializePBJMessage(session_msg),
            conn->server()
            )) {
        mContext->mainStrand->post(
            Duration::seconds(0.05),
            std::tr1::bind(&SessionManager::retryOpenConnection,this,sporef_uuid,conn->server()),
            "&SessionManager::retryOpenConnection"
        );
    }
    else {
        // Setup a retry in case something gets dropped -- must check status and
        // retries entire connection process
        mContext->mainStrand->post(
            Duration::seconds(3),
            std::tr1::bind(&SessionManager::checkConnectedAndRetry, this, sporef_uuid, conn->server()),
            "SessionManager::checkConnectedAndRetry"
        );
    }
}

void SessionManager::checkConnectedAndRetry(const SpaceObjectReference& sporef_uuid, ServerID connTo) {
    // The object could have connected and disconnected quickly -- we need to
    // verify it's really still trying to connect
    if (mObjectConnections.exists(sporef_uuid) && mObjectConnections.getConnectingToServer(sporef_uuid) == connTo) {
        getSpaceConnection(
            connTo,
            std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_uuid, std::tr1::placeholders::_1, true)
        );
    }
}


void SessionManager::migrate(const SpaceObjectReference& sporef_obj_id, ServerID sid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;

    SESSION_LOG(detailed,"Starting migration of " << sporef_obj_id << " to " << sid);

    //forcibly close the SST connection for this object to its current previous
    //space server
    //ObjectReference objref(sporef_obj_id.object());
    if (mObjectToSpaceStreams.find(sporef_obj_id.object()) != mObjectToSpaceStreams.end())
    {
        SESSION_LOG(detailed, "deleting object-space streams  of " << sporef_obj_id << " to " << sid);
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
        SESSION_LOG(warn,"Couldn't open connection to server " << sid << " for migration of object " << sporef_obj_id);
        // FIXME disconnect? retry?
        return;
    }

    Sirikata::Protocol::Session::Container session_msg;
    // Migrations trigger a change in session seqno since we're
    // working with a new space server
    session_msg.set_seqno( mObjectConnections.updateSeqno(sporef_obj_id) );
    Sirikata::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    fillVersionInfo(connect_msg.mutable_version(), mContext);
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
                       retry),
                "SessionManager::getSpaceConnection"
            );

        SESSION_LOG(warn,"Could not send start migration message in"\
            "openConnectionStartMigration.  Re-trying");
    }

    // FIXME do something on failure
}

OHDP::DelegatePort* SessionManager::createDelegateOHDPPort(OHDP::DelegateService*, const OHDP::Endpoint& ept) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    return new OHDP::DelegatePort(
        (OHDP::DelegateService*)this,
        ept,
        std::tr1::bind(
            &SessionManager::delegateOHDPPortSend, this, ept, _1, _2
        )
    );
}

bool SessionManager::delegateOHDPPortSend(const OHDP::Endpoint& source_ep, const OHDP::Endpoint& dest_ep, MemoryReference payload) {
    // Many of these values don't matter, but we need to fill them in for
    // send(). Really all we care is that we get the UUIDs right (null for
    // source and dest), the ports form the source and dest end points, and
    // the dest node ID correct. The space is irrelevant (this SessionManager is
    // already tied to only one and they get dropped when encoding the
    // ObjectMessage) and the source NodeID is just null (always null for
    // local).
    return send(
        SpaceObjectReference(source_ep.space(), ObjectReference(UUID::null())), source_ep.port(),
        UUID::null(), dest_ep.port(),
        String((char*)payload.data(), payload.size()),
        (ServerID)dest_ep.node()
    );
}

void SessionManager::timeSyncUpdated() {
    assert( mTimeSyncClient->valid() );
    mObjectConnections.invokeDeferredCallbacks();
}

bool SessionManager::send(const SpaceObjectReference& sporef_src, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, const std::string& payload, ServerID dest_server) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mShuttingDown)
        return false;

    if (dest_server == NullServerID) {
        // Try looking it up
        dest_server = mObjectConnections.getConnectedServer(sporef_src);
        // And if we still don't have something, give up
        if (dest_server == NullServerID) {
            SESSION_LOG(error,"Tried to send message when not connected.");
            return false;
        }
    }

    ServerConnectionMap::iterator it = mConnections.find(dest_server);
    if (it == mConnections.end()) {
        SESSION_LOG(warn,"Tried to send message for object to unconnected server.");
        return false;
    }
    SpaceNodeConnection* conn = it->second;

    // FIXME would be nice not to have to do this alloc/dealloc
    ObjectMessage obj_msg;
    createObjectHostMessage(mContext->id, sporef_src, src_port, dest, dest_port, payload, &obj_msg);
    TIMESTAMP_CREATED((&obj_msg), Trace::CREATED);
    bool pushed = conn->push(obj_msg);
#ifdef PROFILE_OH_PACKET_RTT
    if (pushed) {
        mOutstandingPackets[obj_msg.unique()] = mContext->simTime();
    }
#endif
    return pushed;
}

void SessionManager::sendRetryingMessage(const SpaceObjectReference& sporef_src, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, const std::string& payload, ServerID dest_server, Network::IOStrand* strand, const Duration& rate) {
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
                strand, rate),
            "SessionManager::sendRetryingMessage"
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

    // Or if we are already working on getting one
    ServerConnectionMap::iterator connecting_it = mConnectingConnections.find(sid);
    if (connecting_it != mConnectingConnections.end()) {
        connecting_it->second->addCallback(cb);
        return;
    }

    // Otherwise start it up
    setupSpaceConnection(sid, cb);
}

void SessionManager::setupSpaceConnection(ServerID server, SpaceNodeConnection::GotSpaceConnectionCallback cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;

    // Mark us as looking this up, store our callback
    assert(mConnectingConnections.find(server) == mConnectingConnections.end());
    SpaceNodeConnection* conn = new SpaceNodeConnection(
        mContext,
        mIOStrand,
        mHandleReadProfiler,
        mStreamOptions,
        mSpace,
        server,
        static_cast<OHDP::Service*>(this),
        mContext->mainStrand->wrap(
            std::tr1::bind(&SessionManager::handleSpaceConnection,
                this,
                _1, _2, server
            )
        ),
        std::tr1::bind(&SessionManager::scheduleHandleServerMessages, this, _1)
    );
    conn->addCallback(std::tr1::bind(&SessionManager::handleSpaceSession, this, server, _1));
    conn->addCallback(cb);
    mConnectingConnections[server] = conn;

    // Lookup the server's address
    mServerIDMap->lookupExternal(
        server,
        mContext->mainStrand->wrap(
            std::tr1::bind(&SessionManager::finishSetupSpaceConnection, this,
                server, _1
            )
        )
    );
}

void SessionManager::finishSetupSpaceConnection(ServerID server, Address4 addr) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    assert(mConnectingConnections.find(server) != mConnectingConnections.end());
    SpaceNodeConnection* conn = mConnectingConnections[server];
    mConnectingConnections.erase(server);

    if (addr == Address4::Null)
    {
        SESSION_LOG(error,"No record of server " << server<<\
            " in SessionManager.cpp's serveridmap.  "\
            "Aborting space setup operation.");
        conn->failConnection();
        return;
    }

    Address addy(convertAddress4ToSirikata(addr));
    mConnections[server] = conn;
    conn->connect(addy);
    SESSION_LOG(detailed,"Trying to connect to " << addy.toString());
}

void SessionManager::handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                                       const std::string&reason,
                                       ServerID sid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    ServerConnectionMap::iterator conn_it = mConnections.find(sid);
    if (conn_it == mConnections.end())
        return;
    SpaceNodeConnection* conn = conn_it->second;

    SESSION_LOG(detailed,"Handling space connection event...");

    if (status == Sirikata::Network::Stream::Connected) {
        SESSION_LOG(detailed,"Successfully connected to " << sid);

        // Try to setup time syncing if it isn't on yet.
        if (mTimeSyncClient == NULL) {
            // The object IDs and ports used are bogus since the space server just
            // ignores them
            mTimeSyncClient = new TimeSyncClient(
                mContext, this, mSpace,
                Duration::seconds(5),
                std::tr1::bind(&SessionManager::timeSyncUpdated, this)
            );
            mContext->add(mTimeSyncClient);
        }
        mTimeSyncClient->addNode(OHDP::NodeID(sid));
    }
    else if (status == Sirikata::Network::Stream::ConnectionFailed) {
        SESSION_LOG(error,"Failed to connect to server " << sid << ": " << reason);
        delete conn;
        mConnections.erase(sid);
        return;
    }
    else if (status == Sirikata::Network::Stream::Disconnected) {
        SESSION_LOG(error,"Disconnected from server " << sid << ": " << reason);
        delete conn;
        mConnections.erase(sid);
        if (mTimeSyncClient != NULL)
            mTimeSyncClient->removeNode(OHDP::NodeID(sid));
        
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

void SessionManager::handleSpaceSession(ServerID sid, SpaceNodeConnection* conn) {
    if (conn != NULL)
        fireSpaceNodeSession(OHDP::SpaceNodeID(mSpace, OHDP::NodeID(sid)), conn->stream());
}

void SessionManager::scheduleHandleServerMessages(SpaceNodeConnection* conn) {
    mContext->mainStrand->post(
        std::tr1::bind(&SessionManager::handleServerMessages, this, conn),
        "SessionManager::handleServerMessages"
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
#ifdef PROFILE_OH_PACKET_RTT
        OutstandingPacketMap::iterator out_it = mOutstandingPackets.find(msg->unique());
        if (out_it != mOutstandingPackets.end()) {
            Time start_t = out_it->second;
            Time end_t = mContext->simTime();
            mLatencySum += end_t - start_t;
            mLatencyCount++;
            mOutstandingPackets.erase(out_it);
        }
#endif
        handleServerMessage(msg, conn->server());
    }

    if (!conn->empty())
        scheduleHandleServerMessages(conn);

    mHandleMessageProfiler->finished();
}

void SessionManager::handleServerMessage(ObjectMessage* msg, ServerID server_id) {
    TIMESTAMP_START(tstamp, msg);

    // Mark as received
    TIMESTAMP_END(tstamp, Trace::OH_RECEIVED);

    // As a special case, messages dealing with sessions are handled by the object host
    if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_SESSION) {
        handleSessionMessage(msg, server_id);
    }
    else if (msg->source_object() == UUID::null() && msg->dest_object() == UUID::null()) {
        // Non-session messages between the space and OH, i.e. OHDP. Note that
        // the Session messages must be handled *before* this case since they
        // would match this condition.
        OHDP::DelegateService::deliver(
            OHDP::Endpoint(mSpace, OHDP::NodeID(server_id), ODP::PortID(msg->source_port())),
            OHDP::Endpoint(mSpace, OHDP::NodeID::self(), ODP::PortID(msg->dest_port())),
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

void SessionManager::handleSessionMessage(Sirikata::Protocol::Object::ObjectMessage* msg, ServerID from_server) {
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

    // We have to deal with outdated retry messages (as well as just potentially
    // garbage requests). Each of these possibilities should have 2 paths: one
    // handles responses that match (object ID, session ID and current state all
    // match/make sense), the other handles mismatches (we weren't expecting the
    // reply so we ignore it or take some reasonable action like trying to kill
    // an unexpected session).
    if (session_msg.has_connect_response()) {
        Sirikata::Protocol::Session::ConnectResponse conn_resp = session_msg.connect_response();

        if (conn_resp.has_version())
            logVersionInfo(conn_resp.version());

        if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Success)
            handleSessionMessageConnectResponseSuccess(from_server, sporef_obj, session_msg);
        else if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Redirect)
            handleSessionMessageConnectResponseRedirect(from_server, sporef_obj, session_msg);
        else if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Error)
            handleSessionMessageConnectResponseError(from_server, sporef_obj, session_msg);
        else
            SESSION_LOG(error,"Unknown connection response code: " << (int)conn_resp.response());
    }

    if (session_msg.has_init_migration()) {
        handleSessionMessageInitMigration(from_server, sporef_obj, session_msg);
    }

    delete msg;
}

void SessionManager::handleSessionMessageConnectResponseSuccess(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg) {
    uint64 seqno = (session_msg.has_seqno() ? session_msg.seqno() : 0);

    Sirikata::Protocol::Session::ConnectResponse conn_resp = session_msg.connect_response();

    // This is the trickiest case because not handling it correctly can leave
    // us with the space having an active connection for an object that
    // the object host doesn't have state for, i.e. a sort of ghost object that
    // will stick around until the object host disconnects.
    //
    // Case 1: The response is only truly valid if:
    // 1. Object ID exists
    // 2. We're trying to connect or migrate to this server
    // 3. Matching session ID
    //
    // Case 2: However, we want to detect if it is just a retry which doesn't affect
    // anything:
    // 1. Object ID exists
    // 2. We're connected to this server
    // 3. Matching session ID.
    //
    // Case 3: In any other case, we have a response indicating a session we
    // don't know about.

    bool obj_exists = mObjectConnections.exists(sporef_obj);
    // Need to be careful about these since they are only valid if
    // obj_exists. Because they rely on obj_exists implicitly, we can elide
    // checks of obj_exists below.
    bool connected_to_server = obj_exists && (mObjectConnections.getConnectedServer(sporef_obj) == from_server);
    bool connecting_to_server = obj_exists && (mObjectConnections.getConnectingToServer(sporef_obj) == from_server);
    bool migrating_to_server = obj_exists && (mObjectConnections.getMigratingToServer(sporef_obj) == from_server);
    bool matches_seqno = obj_exists && (mObjectConnections.getSeqno(sporef_obj) == seqno);

    // Handle the 3 possible cases:
    if (connected_to_server && matches_seqno) {
        // Case 2: Existing connection, just reack in case the original ack got lost
        SESSION_LOG(detailed, "Reacknowledging successful connection for " << sporef_obj << " to server " << from_server << " after receiving duplicate connection success reply.");
        sendConnectSuccessAck(sporef_obj, from_server);
    }
    else if ((connecting_to_server || migrating_to_server) && matches_seqno) {
        // Case 1: Normal case of connecting or migrating, do normal success steps
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

        sendConnectSuccessAck(sporef_obj, connected_to);
    }
    else {
        // Case 3: Everything else. We don't want to leave an active but unwanted
        // connection open, so we explicitly request a disconnect. This is safe
        // because it's tagged with the same sequence number we just received,
        // so even if we are currently requesting a connection for this object,
        // it won't end up disconnecting that object, i.e. the space server
        // should end up killing..
        sendDisconnectMessage(sporef_obj, from_server, seqno);
    }
}

void SessionManager::handleSessionMessageConnectResponseRedirect(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg) {
    uint64 seqno = (session_msg.has_seqno() ? session_msg.seqno() : 0);

    // This only applies to us if:
    // 1. Object ID exists
    // 2. Connecting to this server (migrating doesn't make sense, shouldn't get redirect)
    // 3. Matching session ID
    if (!mObjectConnections.exists(sporef_obj) ||
        mObjectConnections.getConnectingToServer(sporef_obj) != from_server ||
        mObjectConnections.getSeqno(sporef_obj) != seqno)
    {
        SESSION_LOG(detailed, "Ignoring connection redirect response because matching object session couldn't be found. Response is probably just an outdated retry.");
        return;
    }

    Sirikata::Protocol::Session::ConnectResponse conn_resp = session_msg.connect_response();
    ServerID redirected = conn_resp.redirect();
    SESSION_LOG(detailed,"Object connection for " << sporef_obj << " redirected to " << redirected);
    // Old seqno is no longer relevant, clear it out to make way for the
    // new request
    mObjectConnections.clearSeqno(sporef_obj);
    // Get a connection to request
    getSpaceConnection(
        redirected,
        std::tr1::bind(&SessionManager::openConnectionStartSession, this, sporef_obj, std::tr1::placeholders::_1, false)
    );
}

void SessionManager::handleSessionMessageConnectResponseError(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg) {
    uint64 seqno = (session_msg.has_seqno() ? session_msg.seqno() : 0);

    // This only applies to us if:
    // 1. Object ID exists
    // 2. Connecting to this server or migrating to this server
    // 3. Matching session ID
    if (!mObjectConnections.exists(sporef_obj) ||
        !( mObjectConnections.getConnectingToServer(sporef_obj) == from_server || mObjectConnections.getMigratingToServer(sporef_obj) == from_server) ||
        mObjectConnections.getSeqno(sporef_obj) != seqno)
    {
        SESSION_LOG(detailed, "Ignoring connection error response because matching object session couldn't be found. Request is probably just an outdated retry.");
        return;
    }

    SESSION_LOG(error,"Error connecting " << sporef_obj << " to space");
    mObjectConnections.handleConnectError(sporef_obj);
}

void SessionManager::handleSessionMessageInitMigration(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg) {
    uint64 seqno = (session_msg.has_seqno() ? session_msg.seqno() : 0);

    // Sanity check the request. We should:
    //  1. Exist
    //  2. Be connected to this server
    //  3. Have matching session IDs
    if (!mObjectConnections.exists(sporef_obj) ||
        mObjectConnections.getConnectedServer(sporef_obj) != from_server ||
        mObjectConnections.getSeqno(sporef_obj) != seqno)
    {
        SESSION_LOG(detailed, "Ignoring init migration request because matching object session couldn't be found. Request is probably just an outdated retry.");
        return;
    }

    Sirikata::Protocol::Session::InitiateMigration init_migr = session_msg.init_migration();
    SESSION_LOG(insane,"Received migration request for " << sporef_obj << " to " << init_migr.new_server());
    migrate(sporef_obj, init_migr.new_server());
}


void SessionManager::sendConnectSuccessAck(const SpaceObjectReference& sporef, ServerID connected_to) {
    // Send an ack so the server (our first conn or after migrating) can start sending data to us
    Sirikata::Protocol::Session::Container ack_msg;
    ack_msg.set_seqno( mObjectConnections.getSeqno(sporef) );
    Sirikata::Protocol::Session::IConnectAck connect_ack_msg = ack_msg.mutable_connect_ack();
    sendRetryingMessage(
        sporef, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(ack_msg),
        connected_to,
        mContext->mainStrand,
        Duration::seconds(0.05)
    );
}

void SessionManager::handleObjectFullyConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const ConnectingInfo& ci, ConnectedCallback real_cb) {
    // Do the callback even if the connection failed so the object is notified.

    SpaceObjectReference spaceobj(space, obj);

    ConnectionInfo conn_info;
    conn_info.server = server;
    conn_info.loc = ci.loc;
    conn_info.orient = ci.orient;
    conn_info.bounds = ci.bounds;
    conn_info.mesh = ci.mesh;
    conn_info.physics = ci.physics;
    conn_info.query = ci.query;
    real_cb(space, obj, conn_info);

    // But don't continue and try to do more work since we don't have a connection
    if (server == NullServerID)
        return;

    mContext->sstConnMgr()->connectStream(
        SSTEndpoint(spaceobj, 0), // Local port is random
        SSTEndpoint(SpaceObjectReference(space, ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
        std::tr1::bind( &SessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj, Connected)
    );
}

void SessionManager::handleObjectFullyMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server, MigratedCallback real_cb) {
    SpaceObjectReference spaceobj(space, obj);

    real_cb(space, obj, server);

    mContext->sstConnMgr()->connectStream(
        SSTEndpoint(spaceobj, 0), // Local port is random
        SSTEndpoint(SpaceObjectReference(space, ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
        std::tr1::bind( &SessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj, Migrated)
    );
}

SessionManager::SSTStreamPtr SessionManager::getSpaceStream(const ObjectReference& objectID) {
  if (mObjectToSpaceStreams.find(objectID) != mObjectToSpaceStreams.end()) {
    return mObjectToSpaceStreams[objectID];
  }

  return SSTStreamPtr();
}


void SessionManager::spaceConnectCallback(int err, SSTStreamPtr s, SpaceObjectReference spaceobj, ConnectionEvent after) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    SESSION_LOG(detailed, "SST object-space connect callback for " << spaceobj.toString() << " : " << err);

    if (err != SST_IMPL_SUCCESS)
    {

        SESSION_LOG(detailed,"Error connecting stream from oh to space.  "\
            "Error code: "<<err<<".  Retrying.");

        // retry creating an SST stream from the space server to object 'obj'.
      mContext->sstConnMgr()->connectStream(
            SSTEndpoint(spaceobj, 0), // Local port is random
            SSTEndpoint(SpaceObjectReference(spaceobj.space(), ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
            std::tr1::bind( &SessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj, after)
        );
        return;
    }

    mObjectToSpaceStreams[spaceobj.object()] = s;


    assert(mTimeSyncClient != NULL);
    bool time_synced = mTimeSyncClient->valid();
    mObjectConnections.handleConnectStream(spaceobj, after, time_synced);
}

} // namespace Sirikata
