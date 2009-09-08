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
#include "ObjectFactory.hpp"
#include "ServerIDMap.hpp"

#include <boost/bind.hpp>

#define OH_LOG(level,msg) SILOG(oh,level,"[OH] " << msg)

namespace CBR {

ObjectHost::SpaceNodeConnection::SpaceNodeConnection(boost::asio::io_service& ios, ServerID sid)
 : server(sid),
   is_writing(false)
{
    socket = new boost::asio::ip::tcp::socket(ios);
    connecting = false;
}

ObjectHost::SpaceNodeConnection::~SpaceNodeConnection() {
    delete socket;

    while(!queue.empty()) {
        delete queue.front();
        queue.pop();
    }
}

ObjectHost::ObjectInfo::ObjectInfo()
 : object(NULL),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0)
{
}

ObjectHost::ObjectInfo::ObjectInfo(Object* obj)
 : object(obj),
   connectingTo(0),
   connectedTo(0),
   migratingTo(0)
{
}


ObjectHost::ObjectHost(ObjectHostID _id, ObjectFactory* obj_factory, Trace* trace, ServerIDMap* sidmap, const Time& curt)
 : mContext( new ObjectHostContext(_id, curt) ),
   mServerIDMap(sidmap)
{
    mContext->objectHost = this;
    mContext->objectFactory = obj_factory;
    mContext->trace = trace;
}

ObjectHost::~ObjectHost() {
    delete mContext;
}

const ObjectHostContext* ObjectHost::context() const {
    return mContext;
}

void ObjectHost::openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, const SolidAngle& init_sa, ConnectedCallback cb) {
    // Make sure we have this object's info stored
    ObjectInfoMap::iterator it = mObjectInfo.find(obj->uuid());
    if (it == mObjectInfo.end()) {
        it = mObjectInfo.insert( ObjectInfoMap::value_type( obj->uuid(), ObjectInfo(obj) ) ).first;
    }
    mObjectInfo[obj->uuid()].connecting.loc = init_loc;
    mObjectInfo[obj->uuid()].connecting.bounds = init_bounds;
    mObjectInfo[obj->uuid()].connecting.queryAngle = init_sa;
    mObjectInfo[obj->uuid()].connecting.cb = cb;

    // Get a connection to request
    getSpaceConnection(
        boost::bind(&ObjectHost::openConnectionStartSession, this, obj->uuid(), _1)
    );
}

void ObjectHost::openConnectionStartSession(const UUID& uuid, SpaceNodeConnection* conn) {
    if (conn == NULL) {
        OH_LOG(warn,"Couldn't initiate connection for " << uuid.toString());
        // FIXME disconnect? retry?
        mObjectInfo[uuid].connecting.cb(NullServerID);
        return;
    }

    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    mObjectInfo[uuid].connectingTo = conn->server;

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(CBR::Protocol::Session::Connect::Fresh);
    connect_msg.set_object(uuid);
    CBR::Protocol::Session::ITimedMotionVector loc = connect_msg.mutable_loc();
    loc.set_t( mObjectInfo[uuid].connecting.loc.updateTime() );
    loc.set_position( mObjectInfo[uuid].connecting.loc.position() );
    loc.set_velocity( mObjectInfo[uuid].connecting.loc.velocity() );
    connect_msg.set_bounds( mObjectInfo[uuid].connecting.bounds );
    connect_msg.set_query_angle( mObjectInfo[uuid].connecting.queryAngle.asFloat() );

    send(
        uuid, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg),
        conn->server
    );
    // FIXME do something on failure
}


void ObjectHost::migrate(const UUID& obj_id, ServerID sid) {
    OH_LOG(insane,"Starting migration of " << obj_id.toString() << " to " << sid);

    mObjectInfo[obj_id].migratingTo = sid;

    // Get or start the connection we need to start this migration
    getSpaceConnection(
        sid,
        boost::bind(&ObjectHost::openConnectionStartMigration, this, obj_id, sid, _1)
    );
}

void ObjectHost::openConnectionStartMigration(const UUID& obj_id, ServerID sid, SpaceNodeConnection* conn) {
    if (conn == NULL) {
        OH_LOG(warn,"Couldn't open connection to server " << sid << " for migration of object " << obj_id.toString());
        // FIXME disconnect? retry?
        return;
    }

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_type(CBR::Protocol::Session::Connect::Migration);
    connect_msg.set_object(obj_id);
    send(
        obj_id, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg),
        sid
    );

    // FIXME do something on failure
}


ServerID ObjectHost::getConnectedServer(const UUID& obj_id, bool allow_connecting) {
    // FIXME getConnectedServer during migrations?

    ServerID dest_server = mObjectInfo[obj_id].connectedTo;

    if (dest_server == NullServerID && allow_connecting)
        dest_server = mObjectInfo[obj_id].connectingTo;

    return dest_server;
}

bool ObjectHost::send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    // FIXME getConnectedServer during migrations?
    return send(src->uuid(), src_port, dest, dest_port, payload, getConnectedServer(src->uuid()));
}

bool ObjectHost::send(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server) {
    if (dest_server == NullServerID) {
        OH_LOG(error,"Tried to send message when not connected.");
        return false;
    }

    ServerConnectionMap::iterator it = mConnections.find(dest_server);
    if (it == mConnections.end()) {
        OH_LOG(warn,"Tried to send message for object to unconnected server.");
        return false;
    }
    SpaceNodeConnection* conn = it->second;

    // FIXME would be nice not to have to do this alloc/dealloc
    CBR::Protocol::Object::ObjectMessage* obj_msg = createObjectMessage(mContext->id, src, src_port, dest, dest_port, payload);
    conn->queue.push( serializeObjectHostMessage(*obj_msg) );
    delete obj_msg;

    return true;
}

void ObjectHost::tick(const Time& t) {
    mContext->lastTime = mContext->time;
    mContext->time = t;

    mContext->objectFactory->tick();

    // Set up writers which are not writing yet
    for(ServerConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        SpaceNodeConnection* conn = it->second;
        startWriting(conn);
    }

    // Service the IOService.
    // This will allow both the readers and writers to make progress.
    mIOService.poll();

    //if (mOutgoingQueue.size() > 1000)
    //    SILOG(oh,warn,"[OH] Warning: outgoing queue size > 1000: " << mOutgoingQueue.size());
}


void ObjectHost::getSpaceConnection(GotSpaceConnectionCallback cb) {
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
    using namespace boost::asio::ip;

    SpaceNodeConnection* conn = new SpaceNodeConnection(mIOService, server);
    conn->connectCallbacks.push_back(cb);
    mConnections[server] = conn;

    // Lookup the server's address
    Address4* addr = mServerIDMap->lookupExternal(server);

    // Try to initiate connection
    tcp::endpoint endpt( address_v4(ntohl(addr->ip)), (unsigned short)addr->port);
    OH_LOG(debug,"Trying to connect to " << endpt);
    conn->connecting = true;
    conn->socket->async_connect(
        endpt,
        boost::bind(&ObjectHost::handleSpaceConnection, this, boost::asio::placeholders::error, server)
    );
}

void ObjectHost::handleSpaceConnection(const boost::system::error_code& err, ServerID sid) {
    SpaceNodeConnection* conn = mConnections[sid];

    OH_LOG(debug,"Handling space connection...");
    if (err) {
        OH_LOG(error,"Failed to connect to server " << sid << ": " << err << ", " << err.message());
        for(std::vector<GotSpaceConnectionCallback>::iterator it = conn->connectCallbacks.begin(); it != conn->connectCallbacks.end(); it++)
            (*it)(NULL);
        conn->connectCallbacks.clear();
        delete conn;
        mConnections.erase(sid);
        return;
    }

    OH_LOG(debug,"Successfully connected to " << sid);
    conn->connecting = false;

    // Set up reading for this connection
    startReading(conn);

    // Tell all requesters that the connection is available
    for(std::vector<GotSpaceConnectionCallback>::iterator it = conn->connectCallbacks.begin(); it != conn->connectCallbacks.end(); it++)
        (*it)(conn);
    conn->connectCallbacks.clear();
}

void ObjectHost::startReading(SpaceNodeConnection* conn) {
    boost::asio::async_read(
        *(conn->socket),
        conn->read_buf,
        boost::asio::transfer_at_least(1), // NOTE: This is important, defaults to transfer_all...
        boost::bind( &ObjectHost::handleConnectionRead, this,
            boost::asio::placeholders::error, conn
        )
    );
}

void ObjectHost::handleConnectionRead(const boost::system::error_code& err, SpaceNodeConnection* conn) {
    if (err) {
        // FIXME some kind of error, need to handle this
        OH_LOG(error,"Error in connection read:" << err << " = " << err.message());
        return;
    }

    // dump data
    uint32 buf_size = conn->read_buf.size();
    char* buf = new char[buf_size];
    std::istream is (&conn->read_buf);
    is.read(buf, buf_size);

    conn->read_avail += std::string(buf, buf_size);

    delete buf;

    // try to extract messages
    while( conn->read_avail.size() > sizeof(uint32) ) {
        uint32* msg_size_ptr = (uint32*)&(conn->read_avail[0]);
        uint32 msg_size = *msg_size_ptr;
        if (conn->read_avail.size() < sizeof(uint32) + msg_size)
            break; // No more full messages

        std::string real_payload = conn->read_avail.substr(sizeof(uint32), msg_size);
        conn->read_avail = conn->read_avail.substr(sizeof(uint32) + msg_size);

        CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
        bool parse_success = obj_msg->ParseFromString(real_payload);
        assert(parse_success == true);

        handleServerMessage( conn, obj_msg );
    }

    // continue reading
    startReading(conn);
}


void ObjectHost::handleServerMessage(SpaceNodeConnection* conn, CBR::Protocol::Object::ObjectMessage* msg) {
    // As a special case, messages dealing with sessions are handled by the object host
    if (msg->source_object() == UUID::null() && msg->dest_port() == OBJECT_PORT_SESSION) {
        handleSessionMessage(msg);
        return;
    }

    // Otherwise, by default, we just ship it to the correct object
    mObjectInfo[ msg->dest_object() ].object->receiveMessage(msg);
}

void ObjectHost::handleSessionMessage(CBR::Protocol::Object::ObjectMessage* msg) {
    CBR::Protocol::Session::Container session_msg;
    bool parse_success = session_msg.ParseFromString(msg->payload());
    assert(parse_success);

    assert(!session_msg.has_connect());

    if (session_msg.has_connect_response()) {
        CBR::Protocol::Session::IConnectResponse conn_resp = session_msg.connect_response();

        UUID obj = msg->dest_object();

        if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Success) {
            if (mObjectInfo[obj].connectingTo != NullServerID) { // We were connecting to a server
                ServerID connectedTo = mObjectInfo[obj].connectingTo;
                OH_LOG(debug,"Successfully connected " << obj.toString() << " to space node " << connectedTo);
                mObjectInfo[obj].connectedTo = connectedTo;
                mObjectInfo[obj].connectingTo = NullServerID;
                mObjectInfo[obj].connecting.cb(connectedTo);
                mObjectInfo[obj].connecting.cb = NULL;
            }
            else if (mObjectInfo[obj].migratingTo != NullServerID) { // We were migrating
                ServerID migratedTo = mObjectInfo[obj].migratingTo;
                OH_LOG(debug,"Successfully migrated " << obj.toString() << " to space node " << migratedTo);
                mObjectInfo[obj].connectedTo = migratedTo;
                mObjectInfo[obj].migratingTo = NullServerID;
            }
            else { // What were we doing?
                OH_LOG(error,"Got connection response with no outstanding requests.");
            }
        }
        else if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Redirect) {
            ServerID redirected = conn_resp.redirect();
            OH_LOG(debug,"Object connection for " << obj.toString() << " redirected to " << redirected);
            // Get a connection to request
            getSpaceConnection(
                redirected,
                boost::bind(&ObjectHost::openConnectionStartSession, this, obj, _1)
            );

        }
        else if (conn_resp.response() == CBR::Protocol::Session::ConnectResponse::Error) {
            OH_LOG(error,"Error connecting " << obj.toString() << " to space");
            mObjectInfo[obj].connectingTo = NullServerID;
            mObjectInfo[obj].connecting.cb(NullServerID);
            mObjectInfo[obj].connecting.cb = NULL;
        }
        else {
            OH_LOG(error,"Unknown connection response code: " << conn_resp.response());
        }
    }

    if (session_msg.has_init_migration()) {
        CBR::Protocol::Session::IInitiateMigration init_migr = session_msg.init_migration();
        OH_LOG(insane,"Received migration request for " << msg->dest_object().toString() << " to " << init_migr.new_server());
        migrate(msg->dest_object(), init_migr.new_server());
    }

    delete msg;
}

// Start async writing for this connection if it has data to be sent
void ObjectHost::startWriting(SpaceNodeConnection* conn) {
    if (!conn->queue.empty() && !conn->is_writing) {
        conn->is_writing = true;
        // Get more data into the buffer
        std::ostream write_stream(&conn->write_buf);
        while(!conn->queue.empty()) {
            std::string* data = conn->queue.front();
            conn->queue.pop();
            write_stream.write( data->c_str(), data->size() );
            delete data;
        }

        // And start the write
        OH_LOG(insane,"Starting write of " << conn->write_buf.size() << " bytes");
        boost::asio::async_write(
            *(conn->socket),
            conn->write_buf,
            boost::bind( &ObjectHost::handleConnectionWrite, this,
                boost::asio::placeholders::error, conn)
        );
    }
}

// Handle the async writing callback for this connection
void ObjectHost::handleConnectionWrite(const boost::system::error_code& err, SpaceNodeConnection* conn) {
    conn->is_writing = false;

    if (err) {
        // FIXME some kind of error, need to handle this
        OH_LOG(error,"Error in connection write\n");
        return;
    }

    startWriting(conn);
}


} // namespace CBR
