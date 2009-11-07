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
#include "Options.hpp"
#include "Message.hpp"
#include <sirikata/network/IOService.hpp>
#include <sirikata/network/IOServiceFactory.hpp>
#include <sirikata/network/StreamListenerFactory.hpp>
#include <sirikata/util/PluginManager.hpp>
#include "Statistics.hpp"
#define SPACE_LOG(level,msg) SILOG(space,level,"[SPACE] " << msg)

namespace CBR {

ObjectHostConnectionManager::ObjectHostConnection::ObjectHostConnection(const ConnectionID& conn_id, Sirikata::Network::Stream* str)
 : id(conn_id),
   socket(str)
{
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

bool ObjectHostConnectionManager::send(const ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg) {
    ObjectHostConnectionMap::iterator conn_it = mConnections.find(conn_id);
    if (conn_it == mConnections.end()) {

        SPACE_LOG(error,"Tried to send to unconnected object host.");
        return false;
    }

    ObjectHostConnection* conn = conn_it->second;
    String* data = serializeObjectHostMessage(*msg);
    bool sent = conn->socket->send( Sirikata::MemoryReference( &((*data)[0]), data->size() ), Sirikata::Network::ReliableOrdered );
    delete data;
    if (sent)
        delete msg;
    return sent;
}

void ObjectHostConnectionManager::listen(const Address4& listen_addr) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(Sirikata::DynamicLibrary::filename("tcpsst")),0);

    assert(mAcceptor == NULL);
    mAcceptor=Sirikata::Network::StreamListenerFactory::getSingleton()
        .getConstructor(GetOption("ohstreamlib")->as<String>())
          (mContext->ioService,
           Sirikata::Network::StreamListenerFactory::getSingleton()
            .getOptionParser(GetOption("ohstreamlib")->as<String>())
               (GetOption("ohstreamoptions")->as<String>()));
    Sirikata::Network::Address addr(convertAddress4ToSirikata(listen_addr));
    mAcceptor->listen(
        addr,
        std::tr1::bind(&ObjectHostConnectionManager::handleNewConnection,this,_1,_2)
    );
}

void ObjectHostConnectionManager::shutdown() {
    // Shut down the listener
    mAcceptor->close();

    // Close each connection
    for(ObjectHostConnectionMap::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        ObjectHostConnection* conn = it->second;
        conn->socket->close();
    }
}

void ObjectHostConnectionManager::handleNewConnection(Sirikata::Network::Stream* str, Sirikata::Network::Stream::SetCallbacks& set_callbacks) {
    using std::tr1::placeholders::_1;

    // For some mysterious reason, Sirikata::Network::Stream uses this callback with a NULL stream to indicate
    // all streams from a connection are gone and the underlying connection is shutting down. Why we would care
    // about this I do not know, but we handle it because it may be used in Sirikata somewhere.
    if (str == NULL)
        return;

    SPACE_LOG(debug,"New object host connection handled");

    // Add the new connection to our index, set read callbacks
    ObjectHostConnection* conn = new ObjectHostConnection(getNewConnectionID(), str);
    mConnections[conn->id] = conn;
    set_callbacks(
        &Sirikata::Network::Stream::ignoreConnectionStatus,
        std::tr1::bind(&ObjectHostConnectionManager::handleConnectionRead,
            this,
            conn,
            _1),
        &Sirikata::Network::Stream::ignoreReadySend
    );
}

bool ObjectHostConnectionManager::handleConnectionRead(ObjectHostConnection* conn, Sirikata::Network::Chunk& chunk) {
    SPACE_LOG(insane, "Handling connection read: " << chunk.size() << " bytes");
    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    bool parse_success = obj_msg->ParseFromArray(chunk.data(),chunk.size());
    assert(parse_success == true);
    mContext->trace()->timestampMessage(mContext->time,  obj_msg->unique(),Trace::HANDLE_OBJECT_HOST_MESSAGE,obj_msg->source_port(),obj_msg->dest_port(),  SERVER_PORT_OBJECT_MESSAGE_ROUTING);
    handleObjectHostMessage(conn->id, obj_msg);
    return true;
}

void ObjectHostConnectionManager::handleObjectHostMessage(const ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg) {
    mMessageReceivedCallback(conn_id, msg);
}

} // namespace CBR
