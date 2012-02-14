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
#include <sirikata/core/network/Address4.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/core/network/IOWork.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/space/ObjectHostConnectionID.hpp>
#include <sirikata/core/ohdp/SST.hpp>

namespace Sirikata {

// Opaque connection class
class ObjectHostConnection {
private:
    friend class ObjectHostConnectionManager;
    friend class ObjectHostConnectionID;

    ObjectHostConnection(ShortObjectHostConnectionID sid, Sirikata::Network::Stream* str);
    ~ObjectHostConnection();

    ShortObjectHostConnectionID short_id;
    Sirikata::Network::Stream* socket;
    OHDPSST::Stream::Ptr base_stream;
};

/** ObjectHostConnectionManager handles the networking aspects of interacting
 *  with object hosts.  It listens for connections, maintains per object
 *  connections, and handles shipping messages out to the network.
 */
class SIRIKATA_SPACE_EXPORT ObjectHostConnectionManager {
public:
    class SIRIKATA_SPACE_EXPORT Listener {
    public:
        virtual ~Listener();

        // Note: Because we use forwarding performed by
        // onObjectHostMessageReceived in routing messages to initialize SST,
        // but the onObjectHostConnected callback waits for the SST connection
        // to initialize, these callbacks *will not* be invoked in what you
        // might consider to be the natural order.  They are grouped as they
        // should be treated: onObjectHostMessageReceived is independent of the
        // other two, and the connected/disconnected callbacks should be treated
        // simply as a sort of session manaagement.

        virtual bool onObjectHostMessageReceived(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id, Sirikata::Protocol::Object::ObjectMessage*) = 0;

        virtual void onObjectHostConnected(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id, OHDPSST::Stream::Ptr stream) = 0;
        virtual void onObjectHostDisconnected(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id) = 0;
    };

    ObjectHostConnectionManager(SpaceContext* ctx, const Address4& listen_addr, OHDP::Service* ohdp_service, Listener* listener);
    ~ObjectHostConnectionManager();

    // Return true if the connection ID is valid. This is useful in determining
    // whether a send is failing because the connection has closed or just
    // because some queue is full.
    bool validConnection(const ObjectHostConnectionID& conn_id) const;
    bool validConnection(const ShortObjectHostConnectionID& short_conn_id) const;

    /** NOTE: Must be used from within the main strand.  Currently this is required since we have the return value... */
    WARN_UNUSED
    bool send(const ObjectHostConnectionID& conn_id, Sirikata::Protocol::Object::ObjectMessage* msg);

    WARN_UNUSED
    bool send(const ShortObjectHostConnectionID short_conn_id, Sirikata::Protocol::Object::ObjectMessage* msg);

    void shutdown();

    Network::IOStrand* const netStrand() const {
        return mIOStrand;
    }
private:
    SpaceContext* mContext;
    Network::IOStrand* mIOStrand;
    Sirikata::Network::StreamListener* mAcceptor;

    typedef std::set<ObjectHostConnection*> ObjectHostConnectionSet;
    ObjectHostConnectionSet mConnections;

    ShortObjectHostConnectionID mShortIDSource;
    typedef std::tr1::unordered_map<ShortObjectHostConnectionID, ObjectHostConnection*> ShortIDConnectionMap;
    ShortIDConnectionMap mShortConnections;

    Listener* mListener;

    OHDPSST::BaseDatagramLayer::Ptr mOHSSTDatagramLayer;

    static ObjectHostConnectionID conn_id(ObjectHostConnection* c);

    /** Listen for and handle new connections. */
    void listen(const Address4& listen_addr); // sets up the acceptor, starts the listening cycle
    void handleNewConnection(Sirikata::Network::Stream* str, Sirikata::Network::Stream::SetCallbacks& sc);

    /** Reading and writing handling for ObjectHostConnections. */

    // Handle connection events for entire connections
    void handleConnectionEvent(ObjectHostConnection* conn, Sirikata::Network::Stream::ConnectionStatus status, const std::string& reason);

    void newOHStream(int err, OHDPSST::Stream::Ptr s);

    // Handle async reading callbacks for this connection
    void handleConnectionRead(ObjectHostConnection* conn, Sirikata::Network::Chunk& chunk, const Sirikata::Network::Stream::PauseReceiveCallback& pause);

    bool sendHelper(ObjectHostConnection* conn, Sirikata::Protocol::Object::ObjectMessage* msg);

    // Utility methods which we can post to the main strand to ensure they operate safely.
    void insertConnection(ObjectHostConnection* conn);
    void destroyConnection(ObjectHostConnection* conn);
    void closeAllConnections();
};

} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_HOST_CONNECTION_MANAGER_HPP_
