/*  Sirikata
 *  CoordinatorSessionManager.cpp
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

#include <sirikata/oh/CoordinatorSessionManager.hpp>
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
CoordinatorSessionManager::ObjectConnections::ObjectInfo::ObjectInfo()
 : connectingTo(0),
   connectedTo(0),
   connectedAs(SpaceObjectReference::null())
{
}

// ObjectConnections Implementation

CoordinatorSessionManager::ObjectConnections::ObjectConnections(CoordinatorSessionManager* _parent)
 : parent(_parent)
{
}

void CoordinatorSessionManager::ObjectConnections::add(
    const SpaceObjectReference& sporef_objid, ConnectingInfo ci, InternalConnectedCallback connect_cb
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
    // Add to reverse index // FIXME we need a real ObjectReference to use here
    //mInternalIDs[sporef_objid] = sporef_objid.object();
}

bool CoordinatorSessionManager::ObjectConnections::exists(const SpaceObjectReference& sporef_objid) {
    return mObjectInfo.find(sporef_objid) != mObjectInfo.end();
}

CoordinatorSessionManager::ConnectingInfo& CoordinatorSessionManager::ObjectConnections::connectingTo(const SpaceObjectReference& sporef_objid, ServerID connecting_to) {
    mObjectInfo[sporef_objid].connectingTo = connecting_to;
    return mObjectInfo[sporef_objid].connectingInfo;
}

CoordinatorSessionManager::InternalConnectedCallback& CoordinatorSessionManager::ObjectConnections::getConnectCallback(const SpaceObjectReference& sporef_objid) {
    return mObjectInfo[sporef_objid].connectedCB;
}

ServerID CoordinatorSessionManager::ObjectConnections::handleConnectSuccess(const SpaceObjectReference& sporef_obj, bool do_cb) {
    if (mObjectInfo[sporef_obj].connectingTo != NullServerID) { // We were connecting to a server
        ServerID connectedTo = mObjectInfo[sporef_obj].connectingTo;
        SESSION_LOG(detailed,"Successfully connected " << sporef_obj << " to space node " << connectedTo);
        mObjectInfo[sporef_obj].connectedTo = connectedTo;
        mObjectInfo[sporef_obj].connectingTo = NullServerID;
        mObjectServerMap[connectedTo].push_back(sporef_obj);

        mObjectInfo[sporef_obj].connectedAs = sporef_obj;

        parent->mSpaceObjectRef = sporef_obj;

        //SESSION_LOG(info,"Connection success: " << sporef_obj.toRawHexData());

        // FIXME shoudl be setting internal/external ID maps here
        // Look up internal ID so the OH can find the right object without
        // tracking space IDs
        //UUID dest_internal = getInternalID(ObjectReference(obj));
        parent->mObjectConnectedCallback(sporef_obj, connectedTo);
        return connectedTo;
    }
    else { // What were we doing?
        SESSION_LOG(error,"Got connection response with no outstanding requests.");
        return NullServerID;
    }
}

void CoordinatorSessionManager::ObjectConnections::handleConnectError(const SpaceObjectReference& sporef) {
    mObjectInfo[sporef].connectingTo = NullServerID;
    //ConnectingInfo ci;
    //mObjectInfo[sporef_objid].connectedCB(parent->mSpace, sporef_objid.object(), NullServerID, ci);
    disconnectWithCode(sporef,sporef,Disconnect::LoginDenied);
}

void CoordinatorSessionManager::ObjectConnections::handleConnectStream(const SpaceObjectReference& sporef_objid, ConnectionEvent after, bool do_cb) {
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

void CoordinatorSessionManager::ObjectConnections::remove(const SpaceObjectReference& sporef_objid) {
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

void CoordinatorSessionManager::ObjectConnections::handleUnderlyingDisconnect(ServerID sid, const String& reason) {
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
void CoordinatorSessionManager::ObjectConnections::disconnectWithCode(const SpaceObjectReference& sporef, const SpaceObjectReference& connectedAs, Disconnect::Code code) {
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
void CoordinatorSessionManager::ObjectConnections::gracefulDisconnect(const SpaceObjectReference& sporef) {
    ObjectInfoMap::iterator wherei=mObjectInfo.find(sporef);
    if (wherei!=mObjectInfo.end()) {
        disconnectWithCode(sporef,wherei->second.connectedAs, Disconnect::Requested);
    }
}

ServerID CoordinatorSessionManager::ObjectConnections::getConnectedServer(const SpaceObjectReference& sporef_obj_id, bool allow_connecting) {
    // FIXME getConnectedServer during migrations?

    ServerID dest_server = mObjectInfo[sporef_obj_id].connectedTo;
    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[sporef_obj_id].connectingTo;

    return dest_server;
}

ServerID CoordinatorSessionManager::ObjectConnections::getConnectingToServer(const SpaceObjectReference& sporef_obj_id) {
    return mObjectInfo[sporef_obj_id].connectingTo;
}

void CoordinatorSessionManager::ObjectConnections::invokeDeferredCallbacks() {
    for(DeferredCallbackList::iterator it = mDeferredCallbacks.begin(); it != mDeferredCallbacks.end(); it++)
        (*it)();
    mDeferredCallbacks.clear();
}

// CoordinatorSessionManager Implementation

CoordinatorSessionManager::CoordinatorSessionManager(
    ObjectHostContext* ctx, const SpaceID& space, ServerIDMap* sidmap,
    ObjectConnectedCallback conn_cb, ObjectMessageHandlerCallback msg_cb,
    ObjectDisconnectedCallback disconn_cb
)
 : PollingService(ctx->mainStrand, Duration::seconds(1.f), ctx, "Space Server"),
   OHDP::DelegateService( std::tr1::bind(&CoordinatorSessionManager::createDelegateOHDPPort, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2) ),
   mContext( ctx ),
   mSpace(space),
   mIOStrand( ctx->ioService->createStrand() ),
   mServerIDMap(sidmap),
   mObjectConnectedCallback(conn_cb),
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


CoordinatorSessionManager::~CoordinatorSessionManager() {
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

void CoordinatorSessionManager::start() {
    PollingService::start();
}

void CoordinatorSessionManager::stop() {
    PollingService::stop();

    mShuttingDown = true;

    // Stop processing of all connections
    for (ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        conn->shutdown();
    }
}

void CoordinatorSessionManager::poll() {
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

bool CoordinatorSessionManager::connect(const SpaceObjectReference& sporef_objid, const String& init_oh_name)
{
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mObjectConnections.exists(sporef_objid)) {
        return false;
    }

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    using std::tr1::placeholders::_3;

    ConnectingInfo ci;
    ci.name = init_oh_name;


    // connect_cb gets wrapped so we can start some automatic steps (initial
    // connection of sst stream to space) at the correc time
    mObjectConnections.add(
        sporef_objid, ci,
        std::tr1::bind(&CoordinatorSessionManager::handleObjectFullyConnected, this,
		       _1, _2, _3, ci
        )
    );

    // Get a connection to request
    SESSION_LOG(detailed, "Connection starting for " << sporef_objid);
    getAnySpaceConnection(
        std::tr1::bind(&CoordinatorSessionManager::openConnectionStartSession, this, sporef_objid, _1)
    );
    return true;
}

void CoordinatorSessionManager::disconnect(const SpaceObjectReference& sporef_objid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    ServerID connected_to = mObjectConnections.getConnectedServer(sporef_objid);
    // The caller may be conservative in calling disconnect, especially to make
    // sure disconnections are actually forced (e.g. for safety in the case of a
    // half-complete connection where the initial success is reported but
    // streams aren't done yet).
    if (connected_to == NullServerID)
        return;

    // Construct and send disconnect message.  This has to happen first so we still have
    // connection information so we know where to send the disconnect
    Sirikata::Protocol::Session::Container session_msg;
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

    // Notify of disconnect (requested), then remove
    mObjectConnections.gracefulDisconnect(sporef_objid);
}

Duration CoordinatorSessionManager::serverTimeOffset() const {
    assert(mTimeSyncClient != NULL);
    assert(mTimeSyncClient->valid());
    return mTimeSyncClient->offset();
}

Duration CoordinatorSessionManager::clientTimeOffset() const {
    assert(mTimeSyncClient != NULL);
    assert(mTimeSyncClient->valid());
    return -(mTimeSyncClient->offset());
}

void CoordinatorSessionManager::retryOpenConnection(const SpaceObjectReference&sporef_uuid,ServerID sid) {
    using std::tr1::placeholders::_1;

    getSpaceConnection(
        sid,
        std::tr1::bind(&CoordinatorSessionManager::openConnectionStartSession, this, sporef_uuid, _1)
    );
}
void CoordinatorSessionManager::openConnectionStartSession(const SpaceObjectReference& sporef_uuid, SpaceNodeConnection* conn) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (conn == NULL) {
        SESSION_LOG(warn,"Couldn't initiate connection for " << sporef_uuid);
        // FIXME disconnect? retry?
	ConnectingInfo ci;
        //mObjectConnections.getConnectCallback(sporef_uuid)(mSpace, ObjectReference::null(), NullServerID, ci);
        return;
    }

    SESSION_LOG(detailed, "Base connection to space server obtained, initiating session " << sporef_uuid);
    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    ConnectingInfo ci = mObjectConnections.connectingTo(sporef_uuid, conn->server());

    Sirikata::Protocol::Session::Container session_msg;
    Sirikata::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    fillVersionInfo(connect_msg.mutable_version(), mContext);
    connect_msg.set_type(Sirikata::Protocol::Session::Connect::Fresh);
    connect_msg.set_object(sporef_uuid.object().getAsUUID());
    connect_msg.set_oh_name(ci.name);

    if (!send(sporef_uuid, OBJECT_PORT_SESSION,
              UUID::null(), OBJECT_PORT_SESSION,
              serializePBJMessage(session_msg),
            conn->server()
            )) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&CoordinatorSessionManager::retryOpenConnection,this,sporef_uuid,conn->server()));
    }
    else {
        // Setup a retry in case something gets dropped -- must check status and
        // retries entire connection process
        mContext->mainStrand->post(
            Duration::seconds(3),
            std::tr1::bind(&CoordinatorSessionManager::checkConnectedAndRetry, this, sporef_uuid, conn->server())
        );
    }
}

void CoordinatorSessionManager::checkConnectedAndRetry(const SpaceObjectReference& sporef_uuid, ServerID connTo) {
    // The object could have connected and disconnected quickly -- we need to
    // verify it's really still trying to connect
    if (mObjectConnections.exists(sporef_uuid) && mObjectConnections.getConnectingToServer(sporef_uuid) == connTo) {
        getSpaceConnection(
            connTo,
            std::tr1::bind(&CoordinatorSessionManager::openConnectionStartSession, this, sporef_uuid, std::tr1::placeholders::_1)
        );
    }
}

OHDP::DelegatePort* CoordinatorSessionManager::createDelegateOHDPPort(OHDP::DelegateService*, const OHDP::Endpoint& ept) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    return new OHDP::DelegatePort(
        (OHDP::DelegateService*)this,
        ept,
        std::tr1::bind(
            &CoordinatorSessionManager::delegateOHDPPortSend, this, ept, _1, _2
        )
    );
}

bool CoordinatorSessionManager::delegateOHDPPortSend(const OHDP::Endpoint& source_ep, const OHDP::Endpoint& dest_ep, MemoryReference payload) {
    // Many of these values don't matter, but we need to fill them in for
    // send(). Really all we care is that we get the UUIDs right (null for
    // source and dest), the ports form the source and dest end points, and
    // the dest node ID correct. The space is irrelevant (this CoordinatorSessionManager is
    // already tied to only one and they get dropped when encoding the
    // ObjectMessage) and the source NodeID is just null (always null for
    // local).
    return send(
        SpaceObjectReference(source_ep.space(), ObjectReference(UUID::null())), (uint16)source_ep.port(),
        UUID::null(), (uint16)dest_ep.port(),
        String((char*)payload.data(), payload.size()),
        (ServerID)dest_ep.node()
    );
}

void CoordinatorSessionManager::timeSyncUpdated() {
    assert( mTimeSyncClient->valid() );
    mObjectConnections.invokeDeferredCallbacks();
}

bool CoordinatorSessionManager::send(const SpaceObjectReference& sporef_src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server) {
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

void CoordinatorSessionManager::sendRetryingMessage(const SpaceObjectReference& sporef_src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server, Network::IOStrand* strand, const Duration& rate) {
    bool sent = send(
        sporef_src, src_port,
        dest, dest_port,
        payload,
        dest_server);

    if (!sent) {
        strand->post(
            rate,
            std::tr1::bind(&CoordinatorSessionManager::sendRetryingMessage, this,
                sporef_src, src_port,
                dest, dest_port,
                payload,
                dest_server,
                strand, rate)
        );
    }
}

void CoordinatorSessionManager::migrateEntity(const SpaceObjectReference& sporef_objid, const UUID& uuid, const String& dest_name) {
	Sirikata::SerializationCheck::Scoped sc(&mSerialization);

	ServerID connected_to = mObjectConnections.getConnectedServer(sporef_objid);
	if (connected_to == NullServerID) return;

	Sirikata::Protocol::Session::Container session_msg;
	Sirikata::Protocol::Session::IOHMigration oh_migration_msg = session_msg.mutable_oh_migration();
	oh_migration_msg.set_type(Sirikata::Protocol::Session::OHMigration::Entity);
	oh_migration_msg.set_id(uuid);
	oh_migration_msg.set_oh_name(dest_name);

	sendRetryingMessage(
			sporef_objid, OBJECT_PORT_SESSION,
			UUID::null(), OBJECT_PORT_SESSION,
			serializePBJMessage(session_msg),
			connected_to, mContext->mainStrand, Duration::seconds(0.05));

	SESSION_LOG(info,"Send Request: entity "<<uuid.rawHexData()<<" migrate to OH "<<dest_name);
}

void CoordinatorSessionManager::getAnySpaceConnection(SpaceNodeConnection::GotSpaceConnectionCallback cb) {
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

void CoordinatorSessionManager::getSpaceConnection(ServerID sid, SpaceNodeConnection::GotSpaceConnectionCallback cb) {
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

void CoordinatorSessionManager::setupSpaceConnection(ServerID server, SpaceNodeConnection::GotSpaceConnectionCallback cb) {
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
            std::tr1::bind(&CoordinatorSessionManager::handleSpaceConnection,
                this,
                _1, _2, server
            )
        ),
        std::tr1::bind(&CoordinatorSessionManager::scheduleHandleServerMessages, this, _1)
    );
    conn->addCallback(std::tr1::bind(&CoordinatorSessionManager::handleSpaceSession, this, server, _1));
    conn->addCallback(cb);
    mConnectingConnections[server] = conn;

    // Lookup the server's address
    mServerIDMap->lookupExternal(
        server,
        mContext->mainStrand->wrap(
            std::tr1::bind(&CoordinatorSessionManager::finishSetupSpaceConnection, this,
                server, _1
            )
        )
    );
}

void CoordinatorSessionManager::finishSetupSpaceConnection(ServerID server, Address4 addr) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    assert(mConnectingConnections.find(server) != mConnectingConnections.end());
    SpaceNodeConnection* conn = mConnectingConnections[server];
    mConnectingConnections.erase(server);

    if (addr == Address4::Null)
    {
        SESSION_LOG(error,"No record of server " << server<<\
            " in CoordinatorSessionManager.cpp's serveridmap.  "\
            "Aborting space setup operation.");
        conn->failConnection();
        return;
    }

    Address addy(convertAddress4ToSirikata(addr));
    mConnections[server] = conn;
    conn->connect(addy);
    SESSION_LOG(info,"Trying to connect to " << addy.toString());
}

void CoordinatorSessionManager::handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
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
                std::tr1::bind(&CoordinatorSessionManager::timeSyncUpdated, this)
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

void CoordinatorSessionManager::handleSpaceSession(ServerID sid, SpaceNodeConnection* conn) {
    if (conn != NULL)
        fireSpaceNodeSession(OHDP::SpaceNodeID(mSpace, OHDP::NodeID(sid)), conn->stream());
}

void CoordinatorSessionManager::scheduleHandleServerMessages(SpaceNodeConnection* conn) {
    mContext->mainStrand->post(
        std::tr1::bind(&CoordinatorSessionManager::handleServerMessages, this, conn)
    );
}

void CoordinatorSessionManager::handleServerMessages(SpaceNodeConnection* conn) {
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

void CoordinatorSessionManager::handleServerMessage(ObjectMessage* msg, ServerID server_id) {
    TIMESTAMP_START(tstamp, msg);

    // Mark as received
    TIMESTAMP_END(tstamp, Trace::OH_RECEIVED);

    // As a special case, messages dealing with sessions are handled by the object host
    if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_SESSION) {
        handleSessionMessage(msg);
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

void CoordinatorSessionManager::handleSessionMessage(Sirikata::Protocol::Object::ObjectMessage* msg) {
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

        if (conn_resp.has_version())
            logVersionInfo(conn_resp.version());

        if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Success)
        {
            // To handle unreliability, we may get extra copies of the reply. If
            // we're already connected, ignore this.
            if ((mObjectConnections.getConnectedServer(sporef_obj) != NullServerID))
            {
                delete msg;
                return;
            }

	   		ServerID connected_to = mObjectConnections.handleConnectSuccess(sporef_obj, true);

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

        mContext->mainStrand->post(Duration::seconds(2), std::tr1::bind(&CoordinatorSessionManager::migrateEntity, this, sporef_obj, UUID::random(), "test_oh"));

        }
        else if (conn_resp.response() == Sirikata::Protocol::Session::ConnectResponse::Error) {
            SESSION_LOG(error,"Error connecting " << sporef_obj << " to space");
            mObjectConnections.handleConnectError(sporef_obj);
        }
        else {
            SESSION_LOG(error,"Unknown connection response code: " << (int)conn_resp.response());
        }
    }

    if (session_msg.has_oh_migration()) {
        Sirikata::Protocol::Session::OHMigration oh_migration = session_msg.oh_migration();
        if(oh_migration.type()==Sirikata::Protocol::Session::OHMigration::Object)
        	SESSION_LOG(info,"Received oh migration request of " << oh_migration.id() << " to this host");
        else if(oh_migration.type()==Sirikata::Protocol::Session::OHMigration::Entity){
        	UUID entity_id = oh_migration.id();
        	SESSION_LOG(info,"Receive OH migration request of entity "<<entity_id.rawHexData());
        }
    }

    delete msg;
}

void CoordinatorSessionManager::handleObjectFullyConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const ConnectingInfo& ci) {
    // Do the callback even if the connection failed so the object is notified.

    SpaceObjectReference spaceobj(space, obj);

    ConnectionInfo conn_info;
    conn_info.server = server;

    // But don't continue and try to do more work since we don't have a connection
    if (server == NullServerID)
        return;

    mContext->sstConnMgr()->connectStream(
        SSTEndpoint(spaceobj, 0), // Local port is random
        SSTEndpoint(SpaceObjectReference(space, ObjectReference::spaceServiceID()), OBJECT_SPACE_PORT),
        std::tr1::bind( &CoordinatorSessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj, Connected)
    );
}

CoordinatorSessionManager::SSTStreamPtr CoordinatorSessionManager::getSpaceStream(const ObjectReference& objectID) {
  if (mObjectToSpaceStreams.find(objectID) != mObjectToSpaceStreams.end()) {
    return mObjectToSpaceStreams[objectID];
  }

  return SSTStreamPtr();
}


void CoordinatorSessionManager::spaceConnectCallback(int err, SSTStreamPtr s, SpaceObjectReference spaceobj, ConnectionEvent after) {
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
            std::tr1::bind( &CoordinatorSessionManager::spaceConnectCallback, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2, spaceobj, after)
        );
        return;
    }

    mObjectToSpaceStreams[spaceobj.object()] = s;


    assert(mTimeSyncClient != NULL);
    bool time_synced = mTimeSyncClient->valid();
    mObjectConnections.handleConnectStream(spaceobj, after, time_synced);
}

} // namespace Sirikata
