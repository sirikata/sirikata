/*  cbr
 *  ObjectHost.cpp
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

#include "ObjectHost.hpp"
#include "Statistics.hpp"
#include "Object.hpp"
#include "sirikata/network/StreamFactory.hpp"
#include "sirikata/network/Stream.hpp"
#include "sirikata/util/PluginManager.hpp"
#include "ServerIDMap.hpp"
#include "Random.hpp"
#include "Options.hpp"
#include <boost/bind.hpp>

#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)

using namespace Sirikata;
using namespace Sirikata::Network;

namespace CBR {

ObjectHost::SpaceNodeConnection::SpaceNodeConnection(ObjectHostContext* ctx, IOStrand* ioStrand, OptionSet* streamOptions, ServerID sid, ReceiveCallback rcb)
 : mContext(ctx),
   parent(ctx->objectHost),
   server(sid),
   socket(Sirikata::Network::StreamFactory::getSingleton().getConstructor(GetOption("ohstreamlib")->as<String>())(&ioStrand->service(),streamOptions)),
   connecting(false),
   tag_enqueued(ctx, Trace::OH_ENQUEUED, Trace::OH_DROPPED),
   send_queue(16*1024 /* FIXME */, std::tr1::bind(&ObjectMessage::size, std::tr1::placeholders::_1)),
   tag_dequeued(ctx, Trace::OH_DEQUEUED, Trace::OH_DROPPED),
   streamTx(ctx, socket, ioStrand, Duration::milliseconds((int64)0), Trace::OH_HIT_NETWORK),
   receive_queue(16*1024 /* FIXME */, std::tr1::bind(&ObjectMessage::size, std::tr1::placeholders::_1)),
   mReceiveCB(rcb)
{
    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(Sirikata::DynamicLibrary::filename(GetOption("ohstreamlib")->as<String>())),0);

    tag_enqueued.connect(0, &send_queue, 0);
    send_queue.connect(0, &tag_dequeued, 0);
    tag_dequeued.connect(0, &streamTx, 0);
    streamTx.run();
}

ObjectHost::SpaceNodeConnection::~SpaceNodeConnection() {
    delete socket;
}

bool ObjectHost::SpaceNodeConnection::push(ObjectMessage* msg) {
    return tag_enqueued.push(0,msg);
}

ObjectMessage* ObjectHost::SpaceNodeConnection::pull() {
    return receive_queue.pull();
}

Sirikata::Network::Stream::ReceivedResponse ObjectHost::SpaceNodeConnection::handleRead(Chunk& chunk) {
    // Parse
    ObjectMessage* msg = new ObjectMessage();
    bool parse_success = msg->ParseFromArray(chunk.data(), chunk.size());
    assert(parse_success == true);

    // Push might delete it, record for timestamping
    uint64 msg_unique = msg->unique();
    uint16 msg_source_port = msg->source_port();
    uint16 msg_dest_port = msg->dest_port();

    // Mark as received
    mContext->trace()->timestampMessage(
        mContext->simTime(),
        msg_unique,
        Trace::OH_NET_RECEIVED,
        msg_source_port,
        msg_dest_port
    );

    // NOTE: We can't record drops here or we incur a lot of overhead in parsing...
    bool pushed = receive_queue.push(msg);

    if (pushed) {
        // handleConnectionRead() could be called from any thread/strand. Everything that is not
        // thread safe that could result from a new message needs to happen in the main strand,
        // so just post the whole handler there.
        mReceiveCB(this);
    }
    else {
        mContext->trace()->timestampMessage(
            mContext->simTime(),
            msg_unique,
            Trace::OH_DROPPED,
            msg_source_port,
            msg_dest_port
        );
    }

    // No matter what, we've "handled" the data, either for real or by dropping
    return Sirikata::Network::Stream::AcceptedData;
}


void ObjectHost::SpaceNodeConnection::shutdown() {
    streamTx.shutdown();
    socket->close();
}



// ObjectInfo Implementation
ObjectHost::ObjectConnections::ObjectInfo::ObjectInfo()
 : object(NULL),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0),
   migratedCB(0)
{
}

ObjectHost::ObjectConnections::ObjectInfo::ObjectInfo(Object* obj)
 : object(obj),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0),
   migratedCB(0)
{
}



// ObjectConnections Implementation

ObjectHost::ObjectConnections::ObjectConnections()
 : mLastRRObject(UUID::null())
{
}

void ObjectHost::ObjectConnections::add(Object* obj, ConnectingInfo ci, ConnectedCallback connect_cb, MigratedCallback migrate_cb) {
    // Make sure we have this object's info stored
    ObjectInfoMap::iterator it = mObjectInfo.find(obj->uuid());
    assert (it == mObjectInfo.end());
    it = mObjectInfo.insert( ObjectInfoMap::value_type( obj->uuid(), ObjectInfo(obj) ) ).first;
    ObjectInfo& obj_info = it->second;
    obj_info.connectingInfo = ci;
    obj_info.connectedCB = connect_cb;
    obj_info.migratedCB = migrate_cb;
}

ObjectHost::ConnectingInfo& ObjectHost::ObjectConnections::connectingTo(const UUID& objid, ServerID connecting_to) {
    mObjectInfo[objid].connectingTo = connecting_to;
    return mObjectInfo[objid].connectingInfo;
}

void ObjectHost::ObjectConnections::startMigration(const UUID& objid, ServerID migrating_to) {
    mObjectInfo[objid].migratingTo = migrating_to;

    // Update object indices
    std::vector<UUID>& objects = mObjectServerMap[mObjectInfo[objid].connectedTo];
    std::vector<UUID>::iterator where = std::find(objects.begin(), objects.end(), objid);
    if (where != objects.end()) {
        objects.erase(where);
    }

    // Notify the object
    mObjectInfo[objid].migratedCB(migrating_to);
}

ObjectHost::ConnectedCallback& ObjectHost::ObjectConnections::getConnectCallback(const UUID& objid) {
    return mObjectInfo[objid].connectedCB;
}

ServerID ObjectHost::ObjectConnections::handleConnectSuccess(const UUID& obj) {
    if (mObjectInfo[obj].connectingTo != NullServerID) { // We were connecting to a server
        ServerID connectedTo = mObjectInfo[obj].connectingTo;
        OH_LOG(debug,"Successfully connected " << obj.toString() << " to space node " << connectedTo);
        mObjectInfo[obj].connectedTo = connectedTo;
        mObjectInfo[obj].connectingTo = NullServerID;
        mObjectServerMap[connectedTo].push_back(obj);
        mObjectInfo[obj].connectedCB(connectedTo);
        return connectedTo;
    }
    else if (mObjectInfo[obj].migratingTo != NullServerID) { // We were migrating
        ServerID migratedTo = mObjectInfo[obj].migratingTo;
        OH_LOG(debug,"Successfully migrated " << obj.toString() << " to space node " << migratedTo);
        mObjectInfo[obj].connectedTo = migratedTo;
        mObjectInfo[obj].migratingTo = NullServerID;
        mObjectServerMap[migratedTo].push_back(obj);
        return migratedTo;
    }
    else { // What were we doing?
        OH_LOG(error,"Got connection response with no outstanding requests.");
        return NullServerID;
    }
}

void ObjectHost::ObjectConnections::handleConnectError(const UUID& objid) {
    mObjectInfo[objid].connectingTo = NullServerID;
    mObjectInfo[objid].connectedCB(NullServerID);
}

void ObjectHost::ObjectConnections::remove(const UUID& objid) {
    // Update object indices
    std::vector<UUID>& objects = mObjectServerMap[mObjectInfo[objid].connectedTo];
    std::vector<UUID>::iterator where = std::find(objects.begin(), objects.end(), objid);
    if (where != objects.end()) {
        objects.erase(where);
    }

    // Remove from main object set
    mObjectInfo.erase(objid);
}

Object* ObjectHost::ObjectConnections::object(const UUID& obj_id) {
    return mObjectInfo[obj_id].object;
}

ServerID ObjectHost::ObjectConnections::getConnectedServer(const UUID& obj_id, bool allow_connecting) {
    // FIXME getConnectedServer during migrations?

    ServerID dest_server = mObjectInfo[obj_id].connectedTo;

    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[obj_id].connectingTo;

    return dest_server;
}

Object* ObjectHost::ObjectConnections::roundRobinObject(ServerID whichServer, bool null_if_disconnected) {
    if (mObjectInfo.size()==0) return NULL;
    UUID myrand(mLastRRObject);
    ObjectInfoMap::iterator i=mObjectInfo.end();
    if (whichServer==NullServerID){
        i=mObjectInfo.find(myrand);
        if (i!=mObjectInfo.end()) ++i;
        if (i==mObjectInfo.end()) i=mObjectInfo.begin();
    }else {
        std::vector<UUID> *osm=&mObjectServerMap[whichServer];
        ++mLastRRIndex;
        if (mLastRRIndex>=osm->size()) {
            mLastRRIndex=0;
        }
        if (osm->size()) {
            i=mObjectInfo.find((*osm)[mLastRRIndex]);
        }
    }
    if (i!=mObjectInfo.end()) {
        mLastRRObject=i->first;
        return ( i->second.connectedTo == NullServerID ? NULL : i->second.object );
    }
    return NULL;
}

Object* ObjectHost::ObjectConnections::randomObject(bool null_if_disconnected) {
    if (mObjectServerMap.size()==0) return NULL;
    int iteratorLevel=rand()%mObjectServerMap.size();
    ObjectServerMap::iterator i=mObjectServerMap.begin();
    for (int index=0;index<iteratorLevel;++index,++i) {
    }
    std::vector<UUID> *uuidMap=&i->second;
    if (uuidMap->size()) {
        ObjectInfoMap::iterator i=mObjectInfo.find((*uuidMap)[rand()%uuidMap->size()]);
        if (i!=mObjectInfo.end())
            return ( i->second.connectedTo == NullServerID ? NULL : i->second.object );
    }
    return NULL;
}

Object* ObjectHost::ObjectConnections::randomObject(ServerID whichServer, bool null_if_disconnected) {
    if (whichServer==NullServerID) return randomObject(null_if_disconnected);
    ObjectServerMap::iterator i=mObjectServerMap.find(whichServer);
    if (i==mObjectServerMap.end())
        return NULL;
    std::vector<UUID> *uuidMap=&i->second;
    if (uuidMap->size()) {
        ObjectInfoMap::iterator i=mObjectInfo.find((*uuidMap)[rand()%uuidMap->size()]);
        if (i!=mObjectInfo.end())
            return ( i->second.connectedTo == NullServerID ? NULL : i->second.object );
    }
    return NULL;
}




// ObjectHost Implementation

ObjectHost::ObjectHost(ObjectHostContext* ctx, Trace* trace, ServerIDMap* sidmap)
 : Service(),
   mContext( ctx ),
   mIOService( IOServiceFactory::makeIOService() ),
   mIOStrand( mIOService->createStrand() ),
   mIOWork(NULL),
   mIOThread(NULL),
   mServerIDMap(sidmap),
   mShuttingDown(false)
{
    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(Sirikata::DynamicLibrary::filename(GetOption("ohstreamlib")->as<String>())),0);

    mStreamOptions=Sirikata::Network::StreamFactory::getSingleton().getOptionParser(GetOption("ohstreamlib")->as<String>())(GetOption("ohstreamoptions")->as<String>());




    mHandleMessageProfiler = mContext->profiler->addStage("Handle Server Message");

    mContext->objectHost = this;
}


ObjectHost::~ObjectHost() {
    // Close all connections
    for (ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        delete conn;
    }
    mConnections.clear();


    delete mHandleMessageProfiler;

    delete mIOStrand;
    IOServiceFactory::destroyIOService(mIOService);
}

const ObjectHostContext* ObjectHost::context() const {
    return mContext;
}

void ObjectHost::start() {

    mIOWork = new IOWork( mIOService, "ObjectHost Work" );
    mIOThread = new Thread( std::tr1::bind(&IOService::run, mIOService) );
}

void ObjectHost::stop() {
    mShuttingDown = true;

    delete mIOWork;
    mIOWork = NULL;
    // Stop processing of all connections
    for (ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        conn->shutdown();
    }
    mIOThread->join();
    delete mIOThread;
    mIOThread = NULL;

}

void ObjectHost::connect(Object* obj, const SolidAngle& init_sa, ConnectedCallback connect_cb, MigratedCallback migrate_cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    TimedMotionVector3f init_loc = obj->location();
    BoundingSphere3f init_bounds = obj->bounds();

    openConnection(obj, init_loc, init_bounds, true, init_sa, connect_cb, migrate_cb);
}

void ObjectHost::connect(Object* obj, ConnectedCallback connect_cb, MigratedCallback migrate_cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    TimedMotionVector3f init_loc = obj->location();
    BoundingSphere3f init_bounds = obj->bounds();

    openConnection(obj, init_loc, init_bounds, false, SolidAngle::Max, connect_cb, migrate_cb);
}

void ObjectHost::disconnect(Object* obj) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    UUID objid = obj->uuid();

    // Construct and send disconnect message.  This has to happen first so we still have
    // connection information so we know where to send the disconnect
    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IDisconnect disconnect_msg = session_msg.mutable_disconnect();
    disconnect_msg.set_object(objid);
    disconnect_msg.set_reason("Quit");

    send(objid, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg)
    );
    // FIXME do something on failure

    mObjectConnections.remove(objid);
}

void ObjectHost::openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, bool regQuery, const SolidAngle& init_sa, ConnectedCallback connect_cb, MigratedCallback migrate_cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;

    ConnectingInfo ci;
    ci.loc = init_loc;
    ci.bounds = init_bounds;
    ci.regQuery = regQuery;
    ci.queryAngle = init_sa;

    mObjectConnections.add(obj, ci, connect_cb, migrate_cb);

    // Get a connection to request
    getAnySpaceConnection(
        std::tr1::bind(&ObjectHost::openConnectionStartSession, this, obj->uuid(), _1)
    );
}
void ObjectHost::retryOpenConnection(const UUID&uuid,ServerID sid) {
    using std::tr1::placeholders::_1;

    getSpaceConnection(
        sid,
        std::tr1::bind(&ObjectHost::openConnectionStartSession, this, uuid, _1)
    );
}
void ObjectHost::openConnectionStartSession(const UUID& uuid, SpaceNodeConnection* conn) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (conn == NULL) {
        OH_LOG(warn,"Couldn't initiate connection for " << uuid.toString());
        // FIXME disconnect? retry?
        mObjectConnections.getConnectCallback(uuid)(NullServerID);
        return;
    }

    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    ConnectingInfo ci = mObjectConnections.connectingTo(uuid, conn->server);

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(CBR::Protocol::Session::Connect::Fresh);
    connect_msg.set_object(uuid);
    CBR::Protocol::Session::ITimedMotionVector loc = connect_msg.mutable_loc();
    loc.set_t( ci.loc.updateTime() );
    loc.set_position( ci.loc.position() );
    loc.set_velocity( ci.loc.velocity() );
    connect_msg.set_bounds( ci.bounds );
    if (ci.regQuery)
        connect_msg.set_query_angle( ci.queryAngle.asFloat() );

    if (!send(uuid, OBJECT_PORT_SESSION,
              UUID::null(), OBJECT_PORT_SESSION,
              serializePBJMessage(session_msg),
              conn->server
            )) {
        mContext->mainStrand->post(Duration::seconds(0.05),std::tr1::bind(&ObjectHost::retryOpenConnection,this,uuid,conn->server));
    }
}


void ObjectHost::migrate(const UUID& obj_id, ServerID sid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;

    OH_LOG(insane,"Starting migration of " << obj_id.toString() << " to " << sid);

    mObjectConnections.startMigration(obj_id, sid);

    // Get or start the connection we need to start this migration
    getSpaceConnection(
        sid,
        std::tr1::bind(&ObjectHost::openConnectionStartMigration, this, obj_id, sid, _1)
    );
}

void ObjectHost::openConnectionStartMigration(const UUID& obj_id, ServerID sid, SpaceNodeConnection* conn) {
    using std::tr1::placeholders::_1;
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (conn == NULL) {
        OH_LOG(warn,"Couldn't open connection to server " << sid << " for migration of object " << obj_id.toString());
        // FIXME disconnect? retry?
        return;
    }

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(CBR::Protocol::Session::Connect::Migration);
    connect_msg.set_object(obj_id);
    if (!send(
            obj_id, OBJECT_PORT_SESSION,
            UUID::null(), OBJECT_PORT_SESSION,
            serializePBJMessage(session_msg),
            sid
            ))    {
        std::tr1::function<void(SpaceNodeConnection*)> retry(std::tr1::bind(&ObjectHost::openConnectionStartMigration,
                                                                          this,
                                                                          obj_id,
                                                                          sid,
                                                                          _1));
        mContext->mainStrand
            ->post(Duration::seconds(.05),
                   std::tr1::bind(&ObjectHost::getSpaceConnection,
                                  this,
                                  sid,
                                  retry));
    }

    // FIXME do something on failure
}


bool ObjectHost::send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    return send(src->uuid(), src_port, dest, dest_port, payload);
}

bool ObjectHost::send(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    if (mShuttingDown)
        return false;

    if (dest_server == NullServerID) {
        // Try looking it up
        dest_server = mObjectConnections.getConnectedServer(src);
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
    ObjectMessage* obj_msg = createObjectHostMessage(mContext->id, src, src_port, dest, dest_port, payload);
    mContext->trace()->timestampMessage(mContext->simTime(),obj_msg->unique(),Trace::CREATED,src_port, dest_port);
    return conn->push( obj_msg );
}

void ObjectHost::sendRetryingMessage(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server, IOStrand* strand, const Duration& rate) {
    bool sent = send(
        src, src_port,
        dest, dest_port,
        payload,
        dest_server);

    if (!sent) {
        strand->post(
            rate,
            std::tr1::bind(&ObjectHost::sendRetryingMessage, this,
                src, src_port,
                dest, dest_port,
                payload,
                dest_server,
                strand, rate)
        );
    }
}


bool ObjectHost::ping(const Time& t, const Object*src, const UUID&dest, double distance) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    CBR::Protocol::Object::Ping ping_msg;
    ping_msg.set_ping(t);
    if (distance>=0)
        ping_msg.set_distance(distance);
    ping_msg.set_id(mPingId++);
    ping_msg.set_dat1(0);
    ping_msg.set_dat2(0);
    ping_msg.set_dat3(0);
    ping_msg.set_dat4(0);
    ping_msg.set_dat5(0);
    ping_msg.set_dat6(0);
    ping_msg.set_dat7(0);
    ping_msg.set_dat8(0);
    ServerID destServer = mObjectConnections.getConnectedServer(src->uuid());

    if (destServer!=NullServerID) {
        return send(src->uuid(),OBJECT_PORT_PING,dest,OBJECT_PORT_PING,serializePBJMessage(ping_msg),destServer);
    }
    return false;
}

bool ObjectHost::randomPing(const Time& t) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    Object* a = mObjectConnections.randomObject(true);
    Object* b = mObjectConnections.randomObject(true);

    if (a != NULL && b != NULL)
        return ping(t, a,b->uuid(),(a->location().extrapolate(t).position()-b->location().extrapolate(t).position()).length());

    return false;
}


void ObjectHost::getAnySpaceConnection(GotSpaceConnectionCallback cb) {
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
            if (!rand_conn->connecting) {
                cb(rand_conn);
                return;
            }
            else {
                rand_conn->connectCallbacks.push_back(cb);
                return;
            }
        }
    }

    // Otherwise, initiate one at random
    ServerID server_id = (ServerID)1; // FIXME should be selected at random somehow, and shouldn't already be in the connection map
    getSpaceConnection(server_id, cb);
}

void ObjectHost::getSpaceConnection(ServerID sid, GotSpaceConnectionCallback cb) {
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

void ObjectHost::setupSpaceConnection(ServerID server, GotSpaceConnectionCallback cb) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    SpaceNodeConnection* conn = new SpaceNodeConnection(
        mContext,
        mIOStrand,
        mStreamOptions,
        server,
        mContext->mainStrand->wrap(
            std::tr1::bind(&ObjectHost::handleServerMessage, this, _1)
        )
    );
    conn->connectCallbacks.push_back(cb);
    mConnections[server] = conn;

    // Lookup the server's address
    Address4* addr = mServerIDMap->lookupExternal(server);
    Address addy(convertAddress4ToSirikata(*addr));
    conn->socket->connect(addy,
        &Sirikata::Network::Stream::ignoreSubstreamCallback,
        mContext->mainStrand->wrap(
            std::tr1::bind(&ObjectHost::handleSpaceConnection,
                this,
                _1, _2, server
            )
        ),
        std::tr1::bind(&ObjectHost::SpaceNodeConnection::handleRead,
            conn,
            _1),
        conn->streamTx.readySendCallback()
    );

    OH_LOG(debug,"Trying to connect to " << addy.toString());
    conn->connecting = true;
}

void ObjectHost::handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                                       const std::string&reason,
                                       ServerID sid) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    ServerConnectionMap::iterator conn_it = mConnections.find(sid);
    if (conn_it == mConnections.end())
        return;
    SpaceNodeConnection* conn = conn_it->second;

    OH_LOG(debug,"Handling space connection...");
    if (status!=Sirikata::Network::Stream::Connected) {
        OH_LOG(error,"Failed to connect to server " << sid << ": " << reason);
        for(std::vector<GotSpaceConnectionCallback>::iterator it = conn->connectCallbacks.begin(); it != conn->connectCallbacks.end(); it++)
            (*it)(NULL);
        conn->connectCallbacks.clear();
        delete conn;
        mConnections.erase(sid);
        return;
    }

    OH_LOG(debug,"Successfully connected to " << sid);
    conn->connecting = false;

    // Tell all requesters that the connection is available
    for(std::vector<GotSpaceConnectionCallback>::iterator it = conn->connectCallbacks.begin(); it != conn->connectCallbacks.end(); it++)
        (*it)(conn);
    conn->connectCallbacks.clear();
}

void ObjectHost::handleServerMessage(SpaceNodeConnection* conn) {
    mHandleMessageProfiler->started();

    // Pull it off the queue
    ObjectMessage* msg = conn->pull();
    if (msg == NULL) {
        mHandleMessageProfiler->finished();
        return;
    }

    // Mark as received
    mContext->trace()->timestampMessage(
        mContext->simTime(),
        msg->unique(),
        Trace::OH_RECEIVED,
        msg->source_port(),
        msg->dest_port()
    );

    // Record info to mark destruction at the end of this method since the msg will have been deleted
    uint64 msg_uniq = msg->unique();
    uint16 msg_src_port = msg->source_port();
    uint16 msg_dst_port = msg->dest_port();

    if (msg->source_port()==OBJECT_PORT_PING&&msg->dest_port()==OBJECT_PORT_PING) {
        CBR::Protocol::Object::Ping ping_msg;
        ping_msg.ParseFromString(msg->payload());
        mContext->trace()->ping(ping_msg.ping(),msg->source_object(),mContext->simTime(),msg->dest_object(), ping_msg.has_id()?ping_msg.id():(uint64)-1,ping_msg.has_distance()?ping_msg.distance():-1,msg->unique());
        delete msg;
    }
    // As a special case, messages dealing with sessions are handled by the object host
    else if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_SESSION) {
        handleSessionMessage(msg);
    }
    else {
        // Otherwise, by default, we just ship it to the correct object
        Object* obj = mObjectConnections.object(msg->dest_object());
        if (obj != NULL)
            obj->receiveMessage(msg);
        else
            delete msg;
    }

    mContext->trace()->timestampMessage(mContext->simTime(), msg_uniq, Trace::DESTROYED, msg_src_port, msg_dst_port);

    mHandleMessageProfiler->finished();
}

void ObjectHost::handleSessionMessage(CBR::Protocol::Object::ObjectMessage* msg) {
    Sirikata::SerializationCheck::Scoped sc(&mSerialization);

    using std::tr1::placeholders::_1;

    CBR::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg->payload());
    assert(parse_success);

    assert(!session_msg.has_connect());

    if (session_msg.has_connect_response()) {
        CBR::Protocol::Session::ConnectResponse conn_resp = session_msg.connect_response();

        UUID obj = msg->dest_object();

        if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Success) {
            ServerID connected_to = mObjectConnections.handleConnectSuccess(obj);

            // Send an ack so the server (our first conn or after migrating) can start sending data to us
            CBR::Protocol::Session::Container ack_msg;
            CBR::Protocol::Session::IConnectAck connect_ack_msg = ack_msg.mutable_connect_ack();
            sendRetryingMessage(
                obj, OBJECT_PORT_SESSION,
                UUID::null(), OBJECT_PORT_SESSION,
                serializePBJMessage(ack_msg),
                connected_to,
                mContext->mainStrand,
                Duration::seconds(0.05)
            );
        }
        else if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Redirect) {
            ServerID redirected = conn_resp.redirect();
            OH_LOG(debug,"Object connection for " << obj.toString() << " redirected to " << redirected);
            // Get a connection to request
            getSpaceConnection(
                redirected,
                std::tr1::bind(&ObjectHost::openConnectionStartSession, this, obj, _1)
            );
        }
        else if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Error) {
            OH_LOG(error,"Error connecting " << obj.toString() << " to space");
            mObjectConnections.handleConnectError(obj);
        }
        else {
            OH_LOG(error,"Unknown connection response code: " << conn_resp.response());
        }
    }

    if (session_msg.has_init_migration()) {
        CBR::Protocol::Session::InitiateMigration init_migr = session_msg.init_migration();
        OH_LOG(insane,"Received migration request for " << msg->dest_object().toString() << " to " << init_migr.new_server());
        migrate(msg->dest_object(), init_migr.new_server());
    }

    delete msg;
}

} // namespace CBR
