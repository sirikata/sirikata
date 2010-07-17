/*  Sirikata
 *  SessionManager.hpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#ifndef _SIRIKATA_LIBOH_SESSION_MANAGER_HPP_
#define _SIRIKATA_LIBOH_SESSION_MANAGER_HPP_

#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/oh/SpaceNodeConnection.hpp>
#include <sirikata/core/network/SSTImpl.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/ObjectReference.hpp>

namespace Sirikata {

class ServerIDMap;

/** SessionManager provides most of the session management functionality for
 * object hosts. It uses internal object host IDs (UUIDs) to track objects
 * requests and open sessions, and also handles migrating connections between
 * space servers.
 */
class SIRIKATA_OH_EXPORT SessionManager : public Service {
  public:
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> SessionCallback;
    // Callback indicating that a connection to the server was made and it is available for sessions
    typedef SessionCallback ConnectedCallback;
    // Callback indicating that a connection is being migrated to a new server.  This occurs as soon
    // as the object host starts the transition and no additional notification is given since, for all
    // intents and purposes this is the point at which the transition happens
    typedef SessionCallback MigratedCallback;
    typedef std::tr1::function<void()> StreamCreatedCallback;

    typedef std::tr1::function<void(const Sirikata::Protocol::Object::ObjectMessage&)> ObjectMessageCallback;

    // Notifies the ObjectHost class of a new object connection: void(object, connectedTo)
    typedef std::tr1::function<void(const UUID&,ServerID)> ObjectConnectedCallback;
    // Notifies the ObjectHost class of a migrated object:
    // void(object, migratedFrom, migratedTo)
    typedef std::tr1::function<void(const UUID&,ServerID,ServerID)> ObjectMigratedCallback;
    // Returns a message to the object host for handling.
    typedef std::tr1::function<void(const UUID&, Sirikata::Protocol::Object::ObjectMessage*)> ObjectMessageHandlerCallback;

    SessionManager(ObjectHostContext* ctx, const SpaceID& space, ServerIDMap* sidmap, ObjectConnectedCallback, ObjectMigratedCallback, ObjectMessageHandlerCallback);
    ~SessionManager();

    // NOTE: The public interface is only safe to access from the main strand.

    /** Connect the object to the space with the given starting parameters. */
    void connect(
        const UUID& objid,
        const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds,
        bool regquery, const SolidAngle& init_sa, const String& init_mesh,
        ConnectedCallback connect_cb, MigratedCallback migrate_cb, StreamCreatedCallback
    );
    /** Disconnect the object from the space. */
    void disconnect(const UUID& id);

    // Private version of send that doesn't verify src UUID, allows us to masquerade for session purposes
    // The allow_connecting parameter allows you to use a connection over which the object is still opening
    // a connection.  This is safe since it can only be used by this class (since this is private), so it will
    // only be used to deal with session management.
    // If dest_server is NullServerID, then getConnectedServer is used to determine where to send the packet.
    // This is used to possibly exchange data between the main and IO strands, so it acquires locks.
    bool send(const UUID& objid, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server = NullServerID);

    boost::shared_ptr<Stream<UUID> > getSpaceStream(const UUID& objectID);

    // Service Implementation
    virtual void start();
    virtual void stop();

private:
    // Implementation Note: mIOStrand is a bit misleading. All the "real" IO is isolated to that strand --
    // reads and writes to the actual sockets are handled in mIOStrand. But that is all that is handled
    // there. Since creating/connecting/disconnecting/destroying SpaceNodeConnections is cheap and relatively
    // rare, we keep these in the main strand, allowing us to leave the SpaceNodeConnection map without a lock.
    // Note that the SpaceNodeConnections may themselves be accessed from multiple threads.
    //
    // The data exchange between the strands happens in two places. When sending, it occurs in the connections
    // queue, which is thread safe.  When receiving, it occurs by posting a handler for the parsed message
    // to the main thread.
    //
    // Note that this means the majority of this class is executed in the main strand. Only reading and writing
    // are separated out, which allows us to ensure the network will be serviced as fast as possible, but
    // doesn't help if our limiting factor is the speed at which this input/output can be handled.
    //
    // Note also that this class does *not* handle multithreaded input -- currently all access of public
    // methods should be performed from the main strand.

    struct ConnectingInfo;

    // Schedules received server messages for handling
    void scheduleHandleServerMessages(SpaceNodeConnection* conn);
    void handleServerMessages(SpaceNodeConnection* conn);
    // Starting point for handling of all messages from the server -- either handled as a special case, such as
    // for session management, or dispatched to the object
    void handleServerMessage(ObjectMessage* msg);

    // Handles session messages received from the server -- connection replies, migration requests, etc.
    void handleSessionMessage(Sirikata::Protocol::Object::ObjectMessage* msg);
    void retryOpenConnection(const UUID&uuid,ServerID sid);


    // Utility method which keeps trying to resend a message
    void sendRetryingMessage(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server, Network::IOStrand* strand, const Duration& rate);

    /** SpaceNodeConnection initiation. */

    // Get an existing space connection or initiate a new one at random
    // which can be used for bootstrapping connections
    void getAnySpaceConnection(SpaceNodeConnection::GotSpaceConnectionCallback cb);
    // Get the connection to the specified space node
    void getSpaceConnection(ServerID sid, SpaceNodeConnection::GotSpaceConnectionCallback cb);

    // Set up a space connection to the given server
    void setupSpaceConnection(ServerID server, SpaceNodeConnection::GotSpaceConnectionCallback cb);

    // Handle a connection event, i.e. the socket either successfully connected or failed
    void handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                               const std::string&reason,
                               ServerID sid);


    /** Object session initiation. */

    // Final callback in session initiation -- we have all the info and now just have to return it to the object
    void openConnectionStartSession(const UUID& uuid, SpaceNodeConnection* conn);


    /** Object session migration. */

    // Start the migration process for the object to the given server.
    void migrate(const UUID& obj_id, ServerID sid);

    // Callback that indicates we have a connection to the new server and can now start the migration to it.
    void openConnectionStartMigration(const UUID& uuid, ServerID sid, SpaceNodeConnection* conn);


    // Currently the SST code requires an ObjectMessageRouter per connection,
    // which used to be handled by the Object.  Since SessionManager has no
    // access to that outside state, we instead construct an ObjectMessageRouter
    // for the Object.
    struct ObjectConnectionRouter : public ObjectMessageRouter {
    public:
        ObjectConnectionRouter(SessionManager* _parent, const UUID& _uuid);
        virtual bool route(Sirikata::Protocol::Object::ObjectMessage*);

    private:
        SessionManager* parent;
        UUID uuid;
    };

    OptionSet* mStreamOptions;

    // THREAD SAFE
    // These may be accessed safely by any thread

    ObjectHostContext* mContext;
    SpaceID mSpace;

    Network::IOService* mIOService;
    Network::IOStrand* mIOStrand;
    Network::IOWork* mIOWork;
    Thread* mIOThread;

    ServerIDMap* mServerIDMap;

    TimeProfiler::Stage* mHandleReadProfiler;
    TimeProfiler::Stage* mHandleMessageProfiler;

    Sirikata::SerializationCheck mSerialization;

    // Main strand only

    // Callbacks for parent ObjectHost
    ObjectConnectedCallback mObjectConnectedCallback;
    ObjectMigratedCallback mObjectMigratedCallback;
    ObjectMessageHandlerCallback mObjectMessageHandlerCallback;

    // Only main strand accesses and manipulates the map, although other strand
    // may access the SpaceNodeConnection*'s.
    typedef std::tr1::unordered_map<ServerID, SpaceNodeConnection*> ServerConnectionMap;
    ServerConnectionMap mConnections;


    // Info associated with opening connections
    struct ConnectingInfo {
        TimedMotionVector3f loc;
        BoundingSphere3f bounds;
        bool regQuery;
        SolidAngle queryAngle;
        String mesh;
    };


    // Objects connections, maintains object connections and mapping
    class ObjectConnections {
    public:
        ObjectConnections(SessionManager* _parent);

        // Add the object, completely disconnected, to the index
        void add(const UUID& objid, ConnectingInfo ci, ConnectedCallback connect_cb, MigratedCallback migrate_cb,
	       StreamCreatedCallback stream_created_cb);

        // Mark the object as connecting to the given server
        ConnectingInfo& connectingTo(const UUID& obj, ServerID connecting_to);

        // Start a migration to a new server, return the MigratedCallback for the object
        void startMigration(const UUID& objid, ServerID migrating_to);

        WARN_UNUSED
        ConnectedCallback& getConnectCallback(const UUID& objid);

        // Marks as connected and returns the server connected to
        ServerID handleConnectSuccess(const UUID& obj);

        void handleConnectError(const UUID& objid);

        void handleConnectStream(const UUID& objid);

        void remove(const UUID& obj);

        // Lookup the server the object is connected to.  With allow_connecting, allows using
        // the server currently being connected to, not just one where a session has been
        // established
        ServerID getConnectedServer(const UUID& obj_id, bool allow_connecting = false);

        UUID getInternalID(const ObjectReference& space_objid) const;
    private:
        SessionManager* parent;

        struct ObjectInfo {
            ObjectInfo();

            ConnectingInfo connectingInfo;

            // Server currently being connected to
            ServerID connectingTo;
            // Server currently connected to
            ServerID connectedTo;
            // Server we're trying to migrate to
            ServerID migratingTo;

            ConnectedCallback connectedCB;
            MigratedCallback migratedCB;
  	    StreamCreatedCallback streamCreatedCB;
        };
        typedef std::tr1::unordered_map<ServerID, std::vector<UUID> > ObjectServerMap;
        ObjectServerMap mObjectServerMap;
        typedef std::tr1::unordered_map<UUID, ObjectInfo, UUID::Hasher> ObjectInfoMap;
        ObjectInfoMap mObjectInfo;

        // A reverse index allows us to lookup an objects internal ID
        typedef std::tr1::unordered_map<ObjectReference, UUID, ObjectReference::Hasher> InternalIDMap;
        InternalIDMap mInternalIDs;
    };
    ObjectConnections mObjectConnections;
    friend class ObjectConnections;

    bool mShuttingDown;

    std::map<UUID, ObjectConnectionRouter*> mObjectConnectionRouters;
    void spaceConnectCallback(int err, boost::shared_ptr< Stream<UUID> > s, UUID obj);
    std::map<UUID, boost::shared_ptr<Stream<UUID> > > mObjectToSpaceStreams;

}; // class SessionManager

} // namespace Sirikata


#endif //_SIRIKATA_LIBOH_SESSION_MANAGER_HPP_
