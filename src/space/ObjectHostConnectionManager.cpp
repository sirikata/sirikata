/*  cbr
 *  ObjectHostConnectionManager.cpp
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

#include "ObjectHostConnectionManager.hpp"
#include "Message.hpp"
#include <boost/bind.hpp>

#define SPACE_LOG(level,msg) SILOG(space,level,"[SPACE] " << msg)

namespace CBR {

ObjectHostConnectionManager::ObjectHostConnection::ObjectHostConnection(boost::asio::io_service& ios, const ConnectionID& conn_id)
 : id(conn_id),
   is_writing(false)
{
    socket = new boost::asio::ip::tcp::socket(ios);
}

ObjectHostConnectionManager::ObjectHostConnection::~ObjectHostConnection() {
    delete socket;
}


ObjectHostConnectionManager::ObjectHostConnectionManager(SpaceContext* ctx, const Address4& listen_addr, MessageReceivedCallback cb)
 : mContext(ctx),
   mAcceptor(NULL),
   mMessageReceivedCallback(cb)
{
    listen(listen_addr);
}

ObjectHostConnectionManager::~ObjectHostConnectionManager() {
    delete mAcceptor;
}


ObjectHostConnectionManager::ConnectionID ObjectHostConnectionManager::getNewConnectionID() {
    // FIXME better and atomic way for generating these
    static ConnectionID src = 0;

    return ++src;
}

void ObjectHostConnectionManager::service() {
    // Set up writers which are not writing yet
    for(ObjectHostConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        ObjectHostConnection* conn = it->second;
        startWriting(conn);
    }

    // Service the IOService.
    // This will allow both the readers and writers to make progress.
    mIOService.poll();
}

void ObjectHostConnectionManager::send(const ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg) {
    ObjectHostConnectionMap::iterator conn_it = mConnections.find(conn_id);
    if (conn_it == mConnections.end()) {

        SPACE_LOG(error,"Tried to send to unconnected object host.");
        return;
    }

    ObjectHostConnection* conn = conn_it->second;
    conn->queue.push( serializeObjectHostMessage(*msg) );
    delete msg;
}

void ObjectHostConnectionManager::listen(const Address4& listen_addr) {
    using namespace boost::asio::ip;

    assert(mAcceptor == NULL);

    // Note: We don't set the IP address because asio refuses to work
    // properly with it, saying nothing here, but refusing object host
    // connections
    //tcp::endpoint endpt( address_v4(ntohl(listen_addr.ip)), (unsigned short)listen_addr.port);
    tcp::endpoint endpt( tcp::v4(), (unsigned short)listen_addr.port);
    mAcceptor = new tcp::acceptor( mIOService, endpt);

    startListening();
}

void ObjectHostConnectionManager::startListening() {
    ObjectHostConnection* new_conn = new ObjectHostConnection(mIOService, getNewConnectionID());
    mAcceptor->async_accept(
        *(new_conn->socket),
        boost::bind(&ObjectHostConnectionManager::handleNewConnection, this,
            boost::asio::placeholders::error,
            new_conn)
    );
}

void ObjectHostConnectionManager::handleNewConnection(const boost::system::error_code& error, ObjectHostConnection* new_conn) {
    SPACE_LOG(debug,"New object host connection handled");

    // Add the new connection to our index and start reading from it
    mConnections[new_conn->id] = new_conn;
    startReading(new_conn);

    // Continue listening for new connections
    startListening();
}


void ObjectHostConnectionManager::startReading(ObjectHostConnection* conn) {
    boost::asio::async_read(
        *(conn->socket),
        conn->read_buf,
        boost::asio::transfer_at_least(1), // NOTE: This is important, defaults to transfer_all...
        boost::bind( &ObjectHostConnectionManager::handleConnectionRead, this,
            boost::asio::placeholders::error, conn
        )
    );
}

void ObjectHostConnectionManager::handleConnectionRead(const boost::system::error_code& err, ObjectHostConnection* conn) {
    if (err) {
        // FIXME some kind of error, need to handle this
        SPACE_LOG(error,"Error in connection read" << err << " - " << err.message());
        return;
    }

    SPACE_LOG(insane, "Handling connection read: " << conn->read_buf.size() << " bytes");

    // dump data
    uint32 buf_size = conn->read_buf.size();
    char* buf = new char[buf_size];
    std::istream is (&conn->read_buf);
    is.read(buf, buf_size);

    conn->read_avail += std::string(buf, buf_size);

    delete []buf;

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

        handleObjectHostMessage(conn->id, obj_msg);
    }

    // continue reading
    startReading(conn);
}

// Start async writing for this connection if it has data to be sent
void ObjectHostConnectionManager::startWriting(ObjectHostConnection* conn) {
    if (!conn->queue.empty() && !conn->is_writing) {
        conn->is_writing = true;
        {
            // Get more data into the buffer
            std::ostream write_stream(&conn->write_buf);
            while(!conn->queue.empty()) {
                std::string* data = conn->queue.front();
                conn->queue.pop();
                write_stream.write( data->c_str(), data->size() );
                delete data;
            }
            write_stream.flush();
        }
        // And start the write
        boost::asio::async_write(
            *(conn->socket),
            conn->write_buf,
            boost::bind( &ObjectHostConnectionManager::handleConnectionWrite, this,
                boost::asio::placeholders::error, conn)
        );
    }
}

// Handle the async writing callback for this connection
void ObjectHostConnectionManager::handleConnectionWrite(const boost::system::error_code& err, ObjectHostConnection* conn) {
    conn->is_writing = false;

    if (err) {
        // FIXME some kind of error, need to handle this
        SPACE_LOG(error,"Error in connection write: " << err << " - " << err.message());
        return;
    }

    startWriting(conn);
}


void ObjectHostConnectionManager::handleObjectHostMessage(const ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg) {
    mMessageReceivedCallback(conn_id, msg);
}

} // namespace CBR
