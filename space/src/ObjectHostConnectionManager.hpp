/*  Sirikata
 *  ObjectHostConnectionManager.hpp
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

#ifndef _SIRIKATA_OBJECT_HOST_CONNECTION_MANAGER_HPP_
#define _SIRIKATA_OBJECT_HOST_CONNECTION_MANAGER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/SpaceContext.hpp>
#include "SpaceNetwork.hpp"
#include <sirikata/space/ServerMessage.hpp>
#include <sirikata/core/network/Address4.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/network/StreamListener.hpp>

namespace Sirikata {

/** ObjectHostConnectionManager handles the networking aspects of interacting
 *  with object hosts.  It listens for connections, maintains per object
 *  connections, and handles shipping messages out to the network.
 */
class ObjectHostConnectionManager {
    struct ObjectHostConnection;
public:
    // Opaque wrapper around ObjectHostConnection*, which maps a connection held
    // by a user to an ObjectHostConnection that data can be sent over.  This
    // provides zero overhead lookup but makes it opaque to outsiders.
    class ConnectionID {
      public:
        ConnectionID();
        ConnectionID(const ConnectionID& rhs);
        ConnectionID& operator=(const ConnectionID& rhs);

        bool operator==(const ConnectionID& rhs) const;
        bool operator!=(const ConnectionID& rhs) const;
      private:
        friend class ObjectHostConnectionManager;
        friend class ObjectHostConnection;

        ConnectionID(ObjectHostConnection* _conn);

        ObjectHostConnection* conn;
    };


    /** Callback generated when an object message is received over a connection.  This callback can be generated
     *  from any strand -- use strand->wrap to ensure its handled in your strand.
     */
    typedef std::tr1::function<bool(ConnectionID, Sirikata::Protocol::Object::ObjectMessage*)> MessageReceivedCallback;
    /** Callback generated when the underlying connection is closed, which will
     *  trigger all objects on that connection to disconnect.
     */
    typedef std::tr1::function<void(ConnectionID)> ConnectionClosedCallback;

    ObjectHostConnectionManager(SpaceContext* ctx, const Address4& listen_addr, MessageReceivedCallback msg_cb, ConnectionClosedCallback closed_cb);
    ~ObjectHostConnectionManager();

    /** NOTE: Must be used from within the main strand.  Currently this is required since we have the return value... */
    WARN_UNUSED
    bool send(const ConnectionID& conn_id, Sirikata::Protocol::Object::ObjectMessage* msg);

    void shutdown();

    Network::IOStrand* const netStrand() const {
        return mIOStrand;
    }
private:
    SpaceContext* mContext;

    Network::IOService* mIOService; // FIXME we should be able to use main IOService, but need underlying connections to be stranded
    Network::IOStrand* mIOStrand;
    Network::IOWork* mIOWork;
    Thread* mIOThread;

    Sirikata::Network::StreamListener* mAcceptor;

    struct ObjectHostConnection {
        ObjectHostConnection(Sirikata::Network::Stream* str);
        ~ObjectHostConnection();

        ConnectionID conn_id();

        Sirikata::Network::Stream* socket;
    };
    typedef std::set<ObjectHostConnection*> ObjectHostConnectionSet;
    ObjectHostConnectionSet mConnections;

    MessageReceivedCallback mMessageReceivedCallback;
    ConnectionClosedCallback mConnectionClosedCallback;


    /** Listen for and handle new connections. */
    void listen(const Address4& listen_addr); // sets up the acceptor, starts the listening cycle
    void handleNewConnection(Sirikata::Network::Stream* str, Sirikata::Network::Stream::SetCallbacks& sc);

    /** Reading and writing handling for ObjectHostConnections. */

    // Handle connection events for entire connections
    void handleConnectionEvent(ObjectHostConnection* conn, Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason);

    // Handle async reading callbacks for this connection
    void handleConnectionRead(ObjectHostConnection* conn, Sirikata::Network::Chunk& chunk, const Sirikata::Network::Stream::PauseReceiveCallback& pause);


    // Utility methods which we can post to the main strand to ensure they operate safely.
    void insertConnection(ObjectHostConnection* conn);
    void destroyConnection(ObjectHostConnection* conn);
    void closeAllConnections();
};

} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_HOST_CONNECTION_MANAGER_HPP_
