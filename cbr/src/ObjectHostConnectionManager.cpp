/*  Sirikata
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

#include "ObjectHostConnectionManager.hpp"
#include <sirikata/cbrcore/Options.hpp>
#include <sirikata/cbrcore/Message.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOStrand.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/util/PluginManager.hpp>
#include <sirikata/cbrcore/Statistics.hpp>
#define SPACE_LOG(level,msg) SILOG(space,level,"[SPACE] " << msg)

namespace Sirikata {

ObjectHostConnectionManager::ConnectionID::ConnectionID()
        : conn(NULL)
{
}

ObjectHostConnectionManager::ConnectionID::ConnectionID(ObjectHostConnection* _conn)
        : conn(_conn)
{
}

ObjectHostConnectionManager::ConnectionID::ConnectionID(const ConnectionID& rhs)
        : conn(rhs.conn)
{
}

ObjectHostConnectionManager::ConnectionID& ObjectHostConnectionManager::ConnectionID::operator=(const ConnectionID& rhs) {
    conn = rhs.conn;
    return *this;
}




ObjectHostConnectionManager::ObjectHostConnection::ObjectHostConnection(Sirikata::Network::Stream* str)
        : socket(str)
{
}

ObjectHostConnectionManager::ObjectHostConnection::~ObjectHostConnection() {
    delete socket;
}

ObjectHostConnectionManager::ConnectionID ObjectHostConnectionManager::ObjectHostConnection::conn_id() {
    return ConnectionID(this);
}


ObjectHostConnectionManager::ObjectHostConnectionManager(SpaceContext* ctx, const Address4& listen_addr, MessageReceivedCallback cb)
 : mContext(ctx),
   mIOService( IOServiceFactory::makeIOService() ),
   mIOStrand( mIOService->createStrand() ),
   mIOWork(NULL),
   mIOThread(NULL),
   mAcceptor(NULL),
   mMessageReceivedCallback(cb)
{
    mIOWork = new IOWork( mIOService, "ObjectHostConnectionManager Work" );
    mIOThread = new Thread( std::tr1::bind(&IOService::runNoReturn, mIOService) );

    listen(listen_addr);
}

ObjectHostConnectionManager::~ObjectHostConnectionManager() {
    delete mAcceptor;

    if (mIOWork != NULL)
        delete mIOWork;

    delete mIOStrand;
    IOServiceFactory::destroyIOService(mIOService);
}


bool ObjectHostConnectionManager::send(const ConnectionID& conn_id, Sirikata::Protocol::Object::ObjectMessage* msg) {
    ObjectHostConnection* conn = conn_id.conn;

    if (conn == NULL) {
        SPACE_LOG(error,"Tried to send over invalid connection.");
        return false;
    }

    // If its not in the connection list we're probably chasing bad pointers
    assert( mConnections.find(conn) != mConnections.end() );

    String data;
    serializePBJMessage(&data, *msg);
    bool sent = conn->socket->send( Sirikata::MemoryReference(data), Sirikata::Network::ReliableOrdered );

    if (sent) {
        TIMESTAMP(msg, Trace::SPACE_TO_OH_ENQUEUED);
        delete msg;
    }
    return sent;
}

void ObjectHostConnectionManager::listen(const Address4& listen_addr) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    static Sirikata::PluginManager sPluginManager;
    static int tcpSstLoaded=(sPluginManager.load(Sirikata::DynamicLibrary::filename("tcpsst")),0);

    String oh_stream_lib = GetOption("ohstreamlib")->as<String>();
    String oh_stream_options = GetOption("ohstreamoptions")->as<String>();

    assert(mAcceptor == NULL);
    mAcceptor=Sirikata::Network::StreamListenerFactory::getSingleton()
        .getConstructor(oh_stream_lib)
          (mIOService,
           Sirikata::Network::StreamListenerFactory::getSingleton()
              .getOptionParser(oh_stream_lib)
               (oh_stream_options));
    Sirikata::Network::Address addr(convertAddress4ToSirikata(listen_addr));
    mAcceptor->listen(
        addr,
        //mIOStrand->wrap(
            std::tr1::bind(&ObjectHostConnectionManager::handleNewConnection,this,_1,_2)
        //) // FIXME can't wrap here yet because of the SetCallbacks parameter -- it requires that we use it immediately
        // and wrapping makes this impossible
    );
}

void ObjectHostConnectionManager::shutdown() {
    // Shut down the listener
    mAcceptor->close();

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectHostConnectionManager::closeAllConnections, this)
    );

    // Get rid of bogus work which ensures we keep the service running
    delete mIOWork;
    mIOWork = NULL;

    // Wait for the other thread to finish operations
    mIOThread->join();
    delete mIOThread;
    mIOThread = NULL;
}

void ObjectHostConnectionManager::handleNewConnection(Sirikata::Network::Stream* str, Sirikata::Network::Stream::SetCallbacks& set_callbacks) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    // For some mysterious reason, Sirikata::Network::Stream uses this callback with a NULL stream to indicate
    // all streams from a connection are gone and the underlying connection is shutting down. Why we would care
    // about this I do not know, but we handle it because it may be used in Sirikata somewhere.
    if (str == NULL)
        return;

    SPACE_LOG(debug,"New object host connection handled");

    // Add the new connection to our index, set read callbacks
    ObjectHostConnection* conn = new ObjectHostConnection(str);
    set_callbacks(
        &Sirikata::Network::Stream::ignoreConnectionCallback,
        std::tr1::bind(&ObjectHostConnectionManager::handleConnectionRead,
            this,
            conn,
            _1, _2), // FIXME this should be wrapped by mIOStrand, but the return value is a problem
        &Sirikata::Network::Stream::ignoreReadySendCallback
    );

    mContext->mainStrand->post(
        std::tr1::bind(&ObjectHostConnectionManager::insertConnection, this, conn)
    );
}

void ObjectHostConnectionManager::handleConnectionRead(ObjectHostConnection* conn, Sirikata::Network::Chunk& chunk, const Sirikata::Network::Stream::PauseReceiveCallback& pause) {
    SPACE_LOG(insane, "Handling connection read: " << chunk.size() << " bytes");

    Sirikata::Protocol::Object::ObjectMessage* obj_msg = new Sirikata::Protocol::Object::ObjectMessage();
    bool parse_success = obj_msg->ParseFromArray(&(*chunk.begin()),chunk.size());
    assert(parse_success == true);

    TIMESTAMP(obj_msg, Trace::HANDLE_OBJECT_HOST_MESSAGE);

    mMessageReceivedCallback(conn->conn_id(), obj_msg);

    // We either got it or dropped it, either way it was accepted.  Don't do
    // anything with pause parameter.
}
void ObjectHostConnectionManager::insertConnection(ObjectHostConnection* conn) {
    mConnections.insert(conn);
}

void ObjectHostConnectionManager::closeAllConnections() {
    // Close each connection
    for(ObjectHostConnectionSet::iterator it = mConnections.begin(); it != mConnections.end(); it++) {
        ObjectHostConnection* conn = (*it);
        conn->socket->close();
    }
}


} // namespace Sirikata
