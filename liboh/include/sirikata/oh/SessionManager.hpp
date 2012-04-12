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
#include <sirikata/core/odp/SST.hpp>
#include <sirikata/core/util/MotionVector.hpp>
#include <sirikata/core/util/MotionQuaternion.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/ohdp/DelegateService.hpp>
#include <sirikata/core/sync/TimeSyncClient.hpp>
#include <sirikata/core/network/Address4.hpp>

#include <sirikata/oh/DisconnectCodes.hpp>
#include <sirikata/oh/SpaceNodeSession.hpp>

namespace Sirikata {

class ServerIDMap;

namespace Protocol {
namespace Session {
class Container;
}
}

/** SessionManager provides most of the session management functionality for
 * object hosts. It uses internal object host IDs (UUIDs) to track objects
 * requests and open sessions, and also handles migrating connections between
 * space servers.
 *
 *  SessionManager is also an OHDP::Service, allowing communication with
 *  individual space servers (e.g. used internally for time sync). It is also
 *  exposed so other services can communicate directly with space servers.
 */
class SIRIKATA_OH_EXPORT SessionManager
    : public PollingService,
      public OHDP::DelegateService,
      public SpaceNodeSessionManager
{
  public:

    struct ConnectionInfo {
        ServerID server;
        TimedMotionVector3f loc;
        TimedMotionQuaternion orient;
        BoundingSphere3f bounds;
        String mesh;
        String physics;
        String query;
    };

    enum ConnectionEvent {
        Connected,
        Migrated,
        Disconnected
    };

    // Callback indicating that a connection to the server was made
    // and it is available for sessions
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, const ConnectionInfo&)> ConnectedCallback;
    // Callback indicating that a connection is being migrated to a new server.  This occurs as soon
    // as the object host starts the transition and no additional notification is given since, for all
    // intents and purposes this is the point at which the transition happens
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> MigratedCallback;
    typedef std::tr1::function<void(const SpaceObjectReference&, ConnectionEvent after)> StreamCreatedCallback;
    typedef std::tr1::function<void(const SpaceObjectReference&, Disconnect::Code)> DisconnectedCallback;

    typedef std::tr1::function<void(const Sirikata::Protocol::Object::ObjectMessage&)> ObjectMessageCallback;

    // Notifies the ObjectHost class of a new object connection: void(object, connectedTo)
    typedef std::tr1::function<void(const SpaceObjectReference&,ServerID)> ObjectConnectedCallback;
    // Notifies the ObjectHost class of a migrated object:
    // void(object, migratedFrom, migratedTo)
    typedef std::tr1::function<void(const SpaceObjectReference&,ServerID,ServerID)> ObjectMigratedCallback;
    // Returns a message to the object host for handling.
    typedef std::tr1::function<void(const SpaceObjectReference&, Sirikata::Protocol::Object::ObjectMessage*)> ObjectMessageHandlerCallback;
    // Notifies the ObjectHost of object connection that was closed, including a
    // reason.
    typedef std::tr1::function<void(const SpaceObjectReference&, Disconnect::Code)> ObjectDisconnectedCallback;

    // SST stream related typedefs
    typedef SST::Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;
    typedef SSTStream::EndpointType SSTEndpoint;
    typedef OHDPSST::Stream OHSSTStream;
    typedef OHSSTStream::Ptr OHSSTStreamPtr;
    typedef OHDPSST::Endpoint OHSSTEndpoint;


    SessionManager(ObjectHostContext* ctx, const SpaceID& space, ServerIDMap* sidmap, ObjectConnectedCallback, ObjectMigratedCallback, ObjectMessageHandlerCallback, ObjectDisconnectedCallback);
    ~SessionManager();

    // NOTE: The public interface is only safe to access from the main strand.

    /** Connect the object to the space with the given starting parameters.
    * \returns true if no other objects on this OH are trying to connect with this ID
    */
    bool connect(
        const SpaceObjectReference& sporef_objid,
        const TimedMotionVector3f& init_loc,
        const TimedMotionQuaternion& init_orient,
        const BoundingSphere3f& init_bounds,
        const String& init_mesh, const String& init_phy,
        const String& init_query, const String& init_zernike,
        ConnectedCallback connect_cb, MigratedCallback migrate_cb,
        StreamCreatedCallback stream_cb, DisconnectedCallback disconnected_cb
    );
    /** Disconnect the object from the space. */
    void disconnect(const SpaceObjectReference& id);

    /** Get offset of server time from client time for the given space. Should
     * only be called by objects with an active connection to that space.
     */
    Duration serverTimeOffset() const;
    /** Get offset of client time from server time for the given space. Should
     * only be called by objects with an active connection to that space. This
     * is just a utility, is always -serverTimeOffset(). */
    Duration clientTimeOffset() const;

    // Private version of send that doesn't verify src UUID, allows us to masquerade for session purposes
    // The allow_connecting parameter allows you to use a connection over which the object is still opening
    // a connection.  This is safe since it can only be used by this class (since this is private), so it will
    // only be used to deal with session management.
    // If dest_server is NullServerID, then getConnectedServer is used to determine where to send the packet.
    // This is used to possibly exchange data between the main and IO strands, so it acquires locks.
    //
    // The allow_connecting flag should only be used internally and allows
    // sending packets over a still-connecting session. This is only used to
    // allow this to act as an OHDP::Service while still in the connecting phase
    // (no callback from SpaceNodeConnection yet) so we can build OHDP::SST
    // streams as part of the connection process.
    bool send(const SpaceObjectReference& sporef_objid, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, const std::string& payload, ServerID dest_server = NullServerID);

    SSTStreamPtr getSpaceStream(const ObjectReference& objectID);

    // Service Implementation
    virtual void start();
    virtual void stop();
    // PollingService Implementation
    virtual void poll();

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
    void handleServerMessages(Liveness::Token alive, SpaceNodeConnection* conn);
    // Starting point for handling of all messages from the server -- either handled as a special case, such as
    // for session management, or dispatched to the object
    void handleServerMessage(ObjectMessage* msg, ServerID sid);

    // Handles session messages received from the server -- connection replies, migration requests, etc.
    void handleSessionMessage(Sirikata::Protocol::Object::ObjectMessage* msg, ServerID from_server);
    // Handlers for specific parts of session messages
    void handleSessionMessageConnectResponseSuccess(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg);
    void handleSessionMessageConnectResponseRedirect(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg);
    void handleSessionMessageConnectResponseError(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg);
    void handleSessionMessageInitMigration(ServerID from_server, const SpaceObjectReference& sporef_obj, Sirikata::Protocol::Session::Container& session_msg);


    // This gets invoked when the connection really is ready -- after
    // successful response and we have time sync info. It does some
    // additional setup work (sst stream) and then invokes the real callback
    void handleObjectFullyConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const ConnectingInfo& ci, ConnectedCallback real_cb);
    // This gets invoked after full migration occurs. It does additional setup
    // work (new sst stream to new space server) and invokes the real callback.
    void handleObjectFullyMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server, MigratedCallback real_cb);

    void retryOpenConnection(const SpaceObjectReference& sporef_uuid,ServerID sid);

    // Utility for sending the connection ack
    void sendConnectSuccessAck(const SpaceObjectReference& sporef, ServerID connected_to);

    // Utility for sending the disconnection request. Session seqno is passed
    // explicitly instead of looking it up via the sporef so that this can be
    // used to request disconnections for sessions we don't have a record for
    // (e.g. because they were cleared out due to object cleanup and then we got
    // the connection success response back).
    void sendDisconnectMessage(const SpaceObjectReference& sporef, ServerID connected_to, uint64 session_seqno);

    // Utility method which keeps trying to resend a message
    void sendRetryingMessage(const SpaceObjectReference& sporef_src, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, const std::string& payload, ServerID dest_server, Network::IOStrand* strand, const Duration& rate);

    /** SpaceNodeConnection initiation. */

    // Get an existing space connection or initiate a new one at random
    // which can be used for bootstrapping connections
    void getAnySpaceConnection(SpaceNodeConnection::GotSpaceConnectionCallback cb);
    // Get the connection to the specified space node
    void getSpaceConnection(ServerID sid, SpaceNodeConnection::GotSpaceConnectionCallback cb);

    // Set up a space connection to the given server
    void setupSpaceConnection(ServerID server, SpaceNodeConnection::GotSpaceConnectionCallback cb);
    void finishSetupSpaceConnection(ServerID server, Address4 addr);

    // Handle a connection event, i.e. the socket either successfully connected or failed
    void handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                               const std::string&reason,
                               ServerID sid);
    // Handle a session event, i.e. the SST stream conected.
    void handleSpaceSession(ServerID sid, SpaceNodeConnection* conn);


    /** Object session initiation. */

    // Final callback in session initiation -- we have all the info and now just
    // have to return it to the object. is_retry indicates whether this is
    // retrying (reuse the existing request ID) or new (generate a new session
    // ID/seqno)
    void openConnectionStartSession(const SpaceObjectReference& sporef_uuid, SpaceNodeConnection* conn, bool is_retry);

    // Timeout handler for initial session message -- checks if the connection
    // succeeded and, if necessary, retries. Requires a seqno so the identical
    // request can be identified if it was retransmitted because it took too
    // long to get a response but was received
    void checkConnectedAndRetry(const SpaceObjectReference& sporef_uuid, ServerID connTo);


    /** Object session migration. */

    // Start the migration process for the object to the given server.
    void migrate(const SpaceObjectReference& sporef_obj_id, ServerID sid);

    // Callback that indicates we have a connection to the new server and can now start the migration to it.
    void openConnectionStartMigration(const SpaceObjectReference& sporef_uuid, ServerID sid, SpaceNodeConnection* conn);


    /** Time Sync related utilities **/


    // OHDP::DelegateService dependencies
    OHDP::DelegatePort* createDelegateOHDPPort(OHDP::DelegateService*, const OHDP::Endpoint& ept);
    bool delegateOHDPPortSend(const OHDP::Endpoint& source_ep, const OHDP::Endpoint& dest_ep, MemoryReference payload);

    void timeSyncUpdated();

    OptionSet* mStreamOptions;

    // THREAD SAFE
    // These may be accessed safely by any thread

    ObjectHostContext* mContext;
    SpaceID mSpace;

    Network::IOStrand* mIOStrand;

    ServerIDMap* mServerIDMap;

    TimeProfiler::Stage* mHandleReadProfiler;
    TimeProfiler::Stage* mHandleMessageProfiler;

    Sirikata::SerializationCheck mSerialization;

    // Callbacks for parent ObjectHost
    ObjectConnectedCallback mObjectConnectedCallback;
    ObjectMigratedCallback mObjectMigratedCallback;
    ObjectMessageHandlerCallback mObjectMessageHandlerCallback;
    ObjectDisconnectedCallback mObjectDisconnectedCallback;

    // Only main strand accesses and manipulates the map, although other strand
    // may access the SpaceNodeConnection*'s.
    typedef std::tr1::unordered_map<ServerID, SpaceNodeConnection*> ServerConnectionMap;
    ServerConnectionMap mConnections;
    ServerConnectionMap mConnectingConnections;

    // Info associated with opening connections
    struct ConnectingInfo {
        TimedMotionVector3f loc;
        TimedMotionQuaternion orient;
        BoundingSphere3f bounds;
        String mesh;
        String physics;
        String query;
        String zernike;
    };
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID, const ConnectingInfo& ci)> InternalConnectedCallback;

    // Objects connections, maintains object connections and mapping
    class ObjectConnections {
    public:
        ObjectConnections(SessionManager* _parent);

        // Add the object, completely disconnected, to the index
        void add(
            const SpaceObjectReference& sporef_objid, ConnectingInfo ci,
            InternalConnectedCallback connect_cb, MigratedCallback migrate_cb,
            StreamCreatedCallback stream_created_cb, DisconnectedCallback disconnected_cb
        );

        bool exists(const SpaceObjectReference& sporef_objid);

        void clearSeqno(const SpaceObjectReference& sporef_objid);
        // Get the seqno for an outstanding request
        uint64 getSeqno(const SpaceObjectReference& sporef_objid);
        // Get a new seqno, and store it
        uint64 updateSeqno(const SpaceObjectReference& sporef_objid);

        // Mark the object as connecting to the given server
        ConnectingInfo& connectingTo(const SpaceObjectReference& obj, ServerID connecting_to);

        // Start a migration to a new server, return the MigratedCallback for the object
        void startMigration(const SpaceObjectReference& objid, ServerID migrating_to);

        WARN_UNUSED
        InternalConnectedCallback& getConnectCallback(const SpaceObjectReference& sporef_objid);

        // Marks as connected and returns the server connected to. do_cb
        // specifies whether the callback should be invoked or deferred
        ServerID handleConnectSuccess(const SpaceObjectReference& sporef_obj, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& mesh, const String& phy, bool do_cb);

        void handleConnectError(const SpaceObjectReference& sporef_objid);

        void handleConnectStream(const SpaceObjectReference& sporef_objid, ConnectionEvent after, bool do_cb);

        void remove(const SpaceObjectReference& obj);

        // Handle a disconnection triggered by the loss of the underlying
        // network connection, i.e. because the TCPSST connection was lost
        // rather than the space server closing an individual session.
        void handleUnderlyingDisconnect(ServerID sid, const String& reason);

        // Handle a graceful disconnection, notifying other objects
        void gracefulDisconnect(const SpaceObjectReference& sporef);

        void disconnectWithCode(const SpaceObjectReference& sporef, const SpaceObjectReference& connectedAs, Disconnect::Code code);
        // Lookup the server the object is connected to.  With allow_connecting, allows using
        // the server currently being connected to, not just one where a session has been
        // established
        ServerID getConnectedServer(const SpaceObjectReference& sporef_obj_id, bool allow_connecting = false);

        ServerID getConnectingToServer(const SpaceObjectReference& sporef_obj_id);

        ServerID getMigratingToServer(const SpaceObjectReference& sporef_obj_id);

        //UUID getInternalID(const ObjectReference& space_objid) const;

        // We have to defer some callbacks sometimes for time
        // synchronization. This invokes them, allowing the connection process
        // to continue.
        void invokeDeferredCallbacks();

    private:
        SessionManager* parent;

        // Source for seqnos for Session messages
        uint64 mSeqnoSource;

        struct ObjectInfo {
            ObjectInfo();

            ConnectingInfo connectingInfo;

            // This is essentially a unique session ID for this
            // object. Note that this shouldn't change as long as the
            // same HostedObject is requesting a session *with the
            // same space server*. So, it will remain the same for all
            // session requests while a HostedObject remains alive and
            // no migrations occur.  This allows us to disambiguate
            // replies from the space server which may come from
            // different requests while the ObjectHost remains
            // connected to the space server.
            uint64 seqno;

            // Server currently being connected to
            ServerID connectingTo;
            // Server currently connected to
            ServerID connectedTo;
            // Server we're trying to migrate to
            ServerID migratingTo;

            SpaceObjectReference connectedAs;

            InternalConnectedCallback connectedCB;
            MigratedCallback migratedCB;
  	    StreamCreatedCallback streamCreatedCB;
  	    DisconnectedCallback disconnectedCB;
        };
        typedef std::tr1::unordered_map<ServerID, std::vector<SpaceObjectReference> > ObjectServerMap;
        ObjectServerMap mObjectServerMap;
        typedef std::tr1::unordered_map<SpaceObjectReference, ObjectInfo, SpaceObjectReference::Hasher> ObjectInfoMap;
        ObjectInfoMap mObjectInfo;

        // A reverse index allows us to lookup an objects internal ID
        //typedef std::tr1::unordered_map<SpaceObjectReference, UUID, ObjectReference::Hasher> InternalIDMap;
        //InternalIDMap mInternalIDs;

        typedef std::tr1::function<void()> DeferredCallback;
        typedef std::vector<DeferredCallback> DeferredCallbackList;
        DeferredCallbackList mDeferredCallbacks;
    };
    ObjectConnections mObjectConnections;
    friend class ObjectConnections;

    TimeSyncClient* mTimeSyncClient;

    bool mShuttingDown;

    void spaceConnectCallback(int err, SSTStreamPtr s, SpaceObjectReference obj, ConnectionEvent after);
    std::map<ObjectReference, SSTStreamPtr> mObjectToSpaceStreams;

#ifdef PROFILE_OH_PACKET_RTT
    // Track outstanding packets for computing RTTs
    typedef std::tr1::unordered_map<uint64, Time> OutstandingPacketMap;
    OutstandingPacketMap mOutstandingPackets;
    uint8 mClearOutstandingCount;
    // And stats
    Duration mLatencySum;
    uint32 mLatencyCount;
    // And some helpers for reporting
    const String mTimeSeriesOHRTT;
#endif
}; // class SessionManager

} // namespace Sirikata


#endif //_SIRIKATA_LIBOH_SESSION_MANAGER_HPP_
