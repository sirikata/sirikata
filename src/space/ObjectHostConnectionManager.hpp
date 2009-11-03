/*  cbr
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

#ifndef _CBR_OBJECT_HOST_CONNECTION_MANAGER_HPP_
#define _CBR_OBJECT_HOST_CONNECTION_MANAGER_HPP_

#include "Utility.hpp"
#include "SpaceContext.hpp"
#include "Network.hpp"
#include "Message.hpp"
#include <sirikata/network/IOService.hpp>
#include <sirikata/network/StreamListener.hpp>

namespace CBR {

/** ObjectHostConnectionManager handles the networking aspects of interacting
 *  with object hosts.  It listens for connections, maintains per object
 *  connections, and handles shipping messages out to the network.
 */
class ObjectHostConnectionManager {
public:
    typedef uint64 ConnectionID; // Used to identify the OH connection to the rest of the system
    typedef std::tr1::function<void(const ConnectionID&, CBR::Protocol::Object::ObjectMessage*)> MessageReceivedCallback;

    ObjectHostConnectionManager(SpaceContext* ctx, const Address4& listen_addr, MessageReceivedCallback cb);
    ~ObjectHostConnectionManager();

    WARN_UNUSED
    bool send(const ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg);

    void shutdown();
private:
    SpaceContext* mContext;

    Sirikata::Network::StreamListener* mAcceptor;

    struct ObjectHostConnection {
        ObjectHostConnection(const ConnectionID& conn_id, Sirikata::Network::Stream* str);
        ~ObjectHostConnection();

        ConnectionID id;
        Sirikata::Network::Stream* socket;
    };
    typedef std::map<ConnectionID, ObjectHostConnection*> ObjectHostConnectionMap;
    ObjectHostConnectionMap mConnections;

    MessageReceivedCallback mMessageReceivedCallback;


    ConnectionID getNewConnectionID();

    /** Listen for and handle new connections. */
    void listen(const Address4& listen_addr); // sets up the acceptor, starts the listening cycle
    void handleNewConnection(Sirikata::Network::Stream* str, Sirikata::Network::Stream::SetCallbacks& sc);

    /** Reading and writing handling for ObjectHostConnections. */

    // Handle async reading callbacks for this connection
    bool handleConnectionRead(ObjectHostConnection* conn, Sirikata::Network::Chunk& chunk);

    /** Handle messages, either directly, e.g. for sessions, or by dispatching them. */
    void handleObjectHostMessage(const ConnectionID& conn_id, CBR::Protocol::Object::ObjectMessage* msg);
};

} // namespace CBR

#endif //_CBR_OBJECT_HOST_CONNECTION_MANAGER_HPP_
