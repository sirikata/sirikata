/*  Sirikata
 *  Server.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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


#ifndef _SIRIKATA_SERVER_HPP_
#define _SIRIKATA_SERVER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include <sirikata/space/SpaceContext.hpp>

#include <sirikata/space/ObjectHostConnectionManager.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/core/queue/SizedThreadSafeQueue.hpp>

#include <sirikata/core/util/MotionVector.hpp>

#include "Protocol_Session.pbj.hpp"
#include "Protocol_Migration.pbj.hpp"

#include <sirikata/space/ObjectSegmentation.hpp>

#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/ohdp/DelegateService.hpp>

#include <sirikata/core/sync/TimeSyncServer.hpp>


namespace Sirikata
{
class Authenticator;

class Forwarder;
class LocalForwarder;

class LocationService;
class Proximity;
class MigrationMonitor;

class CoordinateSegmentation;
class ObjectSegmentation;

class ObjectConnection;
class ObjectHostConnectionManager;

class ObjectHostSessionManager;
class ObjectSessionManager;

  /** Handles all the basic services provided for objects by a server,
   *  including routing and message delivery, proximity services, and
   *  object -> server mapping.  This is a singleton for each simulated
   *  server.  Other servers are referenced by their ServerID.
   */

class Server :
        public MessageRecipient, public Service,
        public OSegWriteListener,
        public ODP::DelegateService, public OHDP::DelegateService,
        ObjectHostConnectionManager::Listener
{
public:
    Server(SpaceContext* ctx, Authenticator* auth, Forwarder* forwarder, LocationService* loc_service, CoordinateSegmentation* cseg, Proximity* prox, ObjectSegmentation* oseg, Address4 oh_listen_addr, ObjectHostSessionManager* oh_sess_mgr, ObjectSessionManager* obj_sess_mgr);
    ~Server();

    virtual void receiveMessage(Message* msg);
private:
    // Service Implementation
    void start();
    void stop();

    // OSegWriteListener Interface
    virtual void osegMigrationAcknowledged(const UUID& id);
    virtual void osegAddNewFinished(const UUID& id, OSegAddNewStatus status);


    // ODP::DelegateService dependencies
    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService*, const SpaceObjectReference& sor, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);

    // OHDP::DelegateService dependencies
    OHDP::DelegatePort* createDelegateOHDPPort(OHDP::DelegateService*, const OHDP::Endpoint& ept);
    bool delegateOHDPPortSend(const OHDP::Endpoint& source_ep, const OHDP::Endpoint& dest_ep, MemoryReference payload);

    // ObjectHostConnectionManager::Listener Interface:

    virtual void onObjectHostConnected(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id, OHDPSST::Stream::Ptr stream);
    // Callback which handles messages from object hosts -- mostly just does sanity checking
    // before using the forwarder to do routing.  Operates in the
    // network strand to allow for fast forwarding, see
    // handleObjectHostMessageRouting for continuation in main strand
    virtual bool onObjectHostMessageReceived(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id, Sirikata::Protocol::Object::ObjectMessage*);
    // Disconnection events, forwarded to
    // handleObjectHostConnectionClosed in main strand
    virtual void onObjectHostDisconnected(const ObjectHostConnectionID& conn_id, const ShortObjectHostConnectionID short_conn_id);


    // Handle a migration event generated by the MigrationMonitor
    void handleMigrationEvent(const UUID& objid);

    // Starts the process of trying to send migration messages, or continues one if it's already running.
    void startSendMigrationMessages();

    // Try to send outstanding migration messages.  This chains automatically until the queue is emptied.
    void trySendMigrationMessages();


    // Send a session message directly to the object via the OH connection manager, bypassing any restrictions on
    // the current state of the connection.  Keeps retrying until the message gets through.
    void sendSessionMessageWithRetry(const ObjectHostConnectionID& conn, Sirikata::Protocol::Object::ObjectMessage* msg, const Duration& retry_rate);


    // Checks if an object is connected to this server
    bool isObjectConnected(const UUID& object_id) const;
    // Checks if an object is current connecting to this server (post authentication)
    bool isObjectConnecting(const UUID& object_id) const;


    // Handle an object host closing its connection
    void handleObjectHostConnectionClosed(const ObjectHostConnectionID& conn_id);
    // Schedule main thread to handle oh message routing
    void scheduleObjectHostMessageRouting();
    void handleObjectHostMessageRouting();
    // Perform forwarding for a message on the front of mRouteObjectMessage from the object host which
    // couldn't be forwarded directly by the networking code
    // (i.e. needs routing to another node)
    bool handleSingleObjectHostMessageRouting();

    // Handle Session messages from an object
    void handleSessionMessage(const ObjectHostConnectionID& oh_conn_id, Sirikata::Protocol::Object::ObjectMessage* msg);
    // Handle Connect message from object
    void handleConnect(const ObjectHostConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container, const Sirikata::Protocol::Session::Connect& connect_msg, uint64 seqno);
    void handleConnectAuthResponse(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id, const Sirikata::Protocol::Session::Connect& connect_msg, uint64 seqno, bool authenticated);

    void sendConnectSuccess(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id, uint64 session_request_seqno);
    void sendConnectError(const ObjectHostConnectionID& oh_conn_id, const UUID& obj_id, uint64 session_request_seqno);

    // Handle connection ack message from object
    void handleConnectAck(const ObjectHostConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container, uint64 session_request_seqno);

    // Handle Migrate message from object
    void handleMigrate(const ObjectHostConnectionID& oh_conn_id, const Sirikata::Protocol::Object::ObjectMessage& container, const Sirikata::Protocol::Session::Connect& migrate_msg, uint64 seqno);

    // Performs actual migration after all the necessary information is available.
    void handleMigration(const UUID& obj_id);

    // Handle a disconnection.
    void handleDisconnect(UUID obj_id, ObjectConnection* conn, uint64 session_request_seqno);

    //finally deletes any object connections to obj_id
    void killObjectConnection(const UUID& obj_id);

    void finishAddObject(const UUID& obj_id, OSegAddNewStatus status);

    bool checkAlreadyMigrating(const UUID& obj_id);
    void processAlreadyMigrating(const UUID& obj_id);

    void newStream(int err, SST::Stream<SpaceObjectReference>::Ptr s);

    SpaceContext* mContext;

    TimeSyncServer* mTimeSyncServer;

    Authenticator* mAuthenticator;
    LocationService* mLocationService;
    CoordinateSegmentation* mCSeg;
    Proximity* mProximity;
    ObjectSegmentation* mOSeg;
    LocalForwarder* mLocalForwarder;
    Forwarder* mForwarder;
    MigrationMonitor* mMigrationMonitor;
    ObjectHostSessionManager* mOHSessionManager;
    ObjectSessionManager* mObjectSessionManager;

    Router<Message*>* mMigrateServerMessageService;

    bool mMigrationSendRunning; // Indicates whether an event chain for sending outstanding migration messages is running.
                                // Note that ideally this could be replaced by just using our own internal queue
                                // to hold messages since migration messsages aren't going to get stale

    bool mShutdownRequested;

    ObjectHostConnectionManager* mObjectHostConnectionManager;

    typedef std::tr1::unordered_map<UUID, ObjectConnection*, UUID::Hasher> ObjectConnectionMap;

    ObjectConnectionMap mObjects; // NOTE: only Forwarder and LocalForwarder
                                  // should actually use the connection, this is
                                  // only still a map to handle migrations
                                  // properly
    // Information to be able to respond to a migration request *from
    // the object*.
    typedef ObjectConnectionMap MigrationRequestMap;
    MigrationRequestMap mObjectsAwaitingMigration;


    typedef std::tr1::unordered_map<UUID, Sirikata::Protocol::Migration::MigrationMessage*, UUID::Hasher> ObjectMigrationMap;
    ObjectMigrationMap mObjectMigrations;

    typedef std::tr1::unordered_set<ObjectConnection*> ObjectConnectionSet;
    ObjectConnectionSet mClosingConnections; // Connections that are closing but need to finish delivering some messages

    //std::map<UUID,ObjectConnection*>
    struct MigratingObjectConnectionsData
    {
      ObjectConnection* obj_conner;
      int milliseconds;
      ServerID migratingTo;
      TimedMotionVector3f loc;
      BoundingSphere3f bnds;
      uint64 uniqueConnId;
      bool serviceConnection;
    };

      typedef std::queue<Message*> MigrateMessageQueue;
      // Outstanding MigrateMessages, which get objects to other servers.
      MigrateMessageQueue mMigrateMessages;

    //    ObjectConnectionMap mMigratingConnections;//bftm add
    typedef std::map<UUID,MigratingObjectConnectionsData> MigConnectionsMap;
    MigConnectionsMap mMigratingConnections;//bftm add
    Timer mMigrationTimer;

    struct StoredConnection
    {
      ObjectHostConnectionID    conn_id;
      Sirikata::Protocol::Session::Connect             conn_msg;
        // Sequence number from session request so we can uniquely
        // identify the request
        uint64 session_seqno;
    };

    typedef std::map<UUID, StoredConnection> StoredConnectionMap;
    StoredConnectionMap  mStoredConnectionData;
    struct ConnectionIDObjectMessagePair{
        ObjectHostConnectionID conn_id;
        Sirikata::Protocol::Object::ObjectMessage* obj_msg;
        ConnectionIDObjectMessagePair(ObjectHostConnectionID conn_id, Sirikata::Protocol::Object::ObjectMessage*msg) {
            this->conn_id=conn_id;
            this->obj_msg=msg;
        }
        size_t size() const{
            return 1;
        }
    };

    // FIXME Another place where needing a size queue and notifications causes
    // double locking...
    boost::mutex mRouteObjectMessageMutex;
    Sirikata::SizedThreadSafeQueue<ConnectionIDObjectMessagePair>mRouteObjectMessage;

    // TimeSeries identifiers. Must include the ServerID for uniqueness, so we
    // cache them so TimeSeries reports are fast
    String mTimeSeriesObjects;

}; // class Server

} // namespace Sirikata

#endif //_SIRIKATA_SERVER_HPP_
