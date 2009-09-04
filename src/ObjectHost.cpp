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
    connectedTo(0)
{
}

ObjectHost::ObjectInfo::ObjectInfo(Object* obj)
 : object(obj),
   connectedTo(0)
{
}


ObjectHost::ObjectHost(ObjectHostID _id, ObjectFactory* obj_factory, Trace* trace, ServerIDMap* sidmap)
 : mContext( new ObjectHostContext(_id) ),
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

    // Sequence for connection
    // 1. Get or initiate random space node connection
    // 1. Ask server which server we *should* be talking to
    // 2. Open connection to that server
    // 3. Initiate session

    // Get a connection to request
    getSpaceConnection(
        boost::bind(&ObjectHost::openConnectionStartSession, this, obj->uuid(), init_loc, init_bounds, init_sa, cb, _1) // FIXME we should send loc request and redirect based on response here
    );
}

void ObjectHost::openConnectionStartSession(const UUID& uuid, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, const SolidAngle& init_sa, ConnectedCallback cb, SpaceNodeConnection* conn) {
    // Send connection msg, store callback info so it can be called when we get a response later in a service call
    mObjectInfo[uuid].connectionCallback = cb;

    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IConnect connect_msg = session_msg.mutable_connect();
    connect_msg.set_object(uuid);
    CBR::Protocol::Session::ITimedMotionVector loc = connect_msg.mutable_loc();
    loc.set_t(init_loc.updateTime());
    loc.set_position(init_loc.position());
    loc.set_velocity(init_loc.velocity());
    connect_msg.set_bounds(init_bounds);
    connect_msg.set_query_angle(init_sa.asFloat());

    send(
        uuid, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg)
    );
    // FIXME do something on failure
}

bool ObjectHost::send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    return send(src->uuid(), src_port, dest, dest_port, payload);
}

bool ObjectHost::send(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload) {
    ServerID dest_server = mObjectInfo[src].connectedTo;

    ServerConnectionMap::iterator it = mConnections.find(dest_server);
    if (it == mConnections.end()) {
        SILOG(cbr,warn,"Tried to send message for object to unconnected server.");
        return false;
    }
    SpaceNodeConnection* conn = it->second;

    CBR::Protocol::Object::ObjectMessage obj_msg;
    obj_msg.set_source_object(src);
    obj_msg.set_source_port(src_port);
    obj_msg.set_dest_object(dest);
    obj_msg.set_dest_port(dest_port);
    obj_msg.set_unique(GenerateUniqueID(mContext->id));
    obj_msg.set_payload( payload );

    // FIXME we need a small header for framing purposes
    std::string real_payload = serializePBJMessage(obj_msg);
    char payload_size_buffer[ sizeof(uint32) + sizeof(uint8) ];
    *((uint32*)payload_size_buffer) = real_payload.size(); // FIXME endian
    *(payload_size_buffer + sizeof(uint32)) = 0; // null terminator
    std::string* final_payload = new std::string( std::string(payload_size_buffer) + real_payload );

    conn->queue.push(final_payload);

    return true;
}

void ObjectHost::migrate(Object* src, ServerID sid) {
    assert(false); // FIXME update, must go to sid
    CBR::Protocol::Session::Container session_msg;
    CBR::Protocol::Session::IMigrate migrate_msg = session_msg.mutable_migrate();
    migrate_msg.set_object(src->uuid());
    std::string session_serialized = serializePBJMessage(session_msg);
    bool success = this->send(
        src, OBJECT_PORT_SESSION,
        UUID::null(), OBJECT_PORT_SESSION,
        serializePBJMessage(session_msg)
    );
    // FIXME do something on failure
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
            if (rand_conn != NULL && !rand_conn->connecting) {
                cb(rand_conn);
                return;
            }
        }
    }

    // Otherwise, initiate one at random
    ServerID server_id = (ServerID)1; // FIXME should be selected at random somehow, and shouldn't already be in the connection map
    setupSpaceConnection(server_id, cb);
}

void ObjectHost::setupSpaceConnection(ServerID server, GotSpaceConnectionCallback cb) {
    using namespace boost::asio::ip;

    SpaceNodeConnection* conn = new SpaceNodeConnection(mIOService, server);
    mConnections[server] = conn;

    // Lookup the server's address
    Address4* addr = mServerIDMap->lookupExternal(server);

    // Try to initiate connection
    tcp::endpoint endpt( address_v4(ntohl(addr->ip)), (unsigned short)addr->port);
    conn->connecting = true;
    conn->socket->async_connect(
        endpt,
        boost::bind(&ObjectHost::handleSpaceConnection, this, boost::asio::placeholders::error, server, cb)
    );
}

void ObjectHost::handleSpaceConnection(const boost::system::error_code& err, ServerID sid, GotSpaceConnectionCallback cb) {
    SpaceNodeConnection* conn = mConnections[sid];

    if (err) {
        SILOG(cbr,error,"Failed to connect to server " << sid << ": " << err);
        delete conn;
        mConnections.erase(sid);
        cb(NULL);
    }

    conn->connecting = false;

    // Set up reading for this connection
    startReading(conn);

    // Tell the requester that the connection is available
    cb(conn);
}

void ObjectHost::startReading(SpaceNodeConnection* conn) {
    boost::asio::async_read(
        *(conn->socket),
        conn->read_buf,
        boost::bind( &ObjectHost::handleConnectionRead, this,
            boost::asio::placeholders::error, conn
        )
    );
}

void ObjectHost::handleConnectionRead(const boost::system::error_code& err, SpaceNodeConnection* conn) {
    if (err) {
        // FIXME some kind of error, need to handle this
        SILOG(cbr,error,"Error in connection read\n");
        return;
    }

    // dump data
    uint32 buf_size = conn->read_buf.size();
    char* buf = new char[buf_size];
    std::istream is (&conn->read_buf);
    is.read(buf, buf_size);

    conn->read_avail += std::string(buf, buf_size);

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

        mObjectInfo[ obj_msg->dest_object() ].object->receiveMessage(obj_msg);
    }

    // continue reading
    startReading(conn);
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
        SILOG(cbr,error,"Error in connection write\n");
        return;
    }

    startWriting(conn);
}


} // namespace CBR
