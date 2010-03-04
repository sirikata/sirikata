/*  cbr
 *  ObjectHost.hpp
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

#ifndef _CBR_OBJECT_HOST_HPP_
#define _CBR_OBJECT_HOST_HPP_

#include "ObjectHostContext.hpp"
#include "QueueRouterElement.hpp"
#include "PollingService.hpp"
#include "TimeProfiler.hpp"
#include "Message.hpp"
#include "SSTImpl.hpp"

#include <sirikata/util/SerializationCheck.hpp>



namespace CBR {

class Object;
class ServerIDMap;

class ObjectHost : public Service {
public:

    typedef std::tr1::function<void(ServerID)> SessionCallback;
    // Callback indicating that a connection to the server was made and it is available for sessions
    typedef SessionCallback ConnectedCallback;
    // Callback indicating that a connection is being migrated to a new server.  This occurs as soon
    // as the object host starts the transition and no additional notification is given since, for all
    // intents and purposes this is the point at which the transition happens
    typedef SessionCallback MigratedCallback;
    typedef std::tr1::function<void()> StreamCreatedCallback;

    // FIXME the ServerID is used to track unique sources, we need to do this separately for object hosts
    ObjectHost(ObjectHostContext* ctx, Trace* trace, ServerIDMap* sidmap);

    ~ObjectHost();

    const ObjectHostContext* context() const;

    // NOTE: The public interface is only safe to access from the main strand.

    /** Connect the object to the space with the given starting parameters. */
    void connect(Object* obj, const SolidAngle& init_sa, ConnectedCallback connected_cb,
		 MigratedCallback migrated_cb, StreamCreatedCallback stream_created_cb);
    void connect(Object* obj, ConnectedCallback connected_cb, MigratedCallback migrated_cb,
		 StreamCreatedCallback stream_created_cb);
    /** Disconnect the object from the space. */
    void disconnect(Object* obj);

    bool send(const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload);

    bool send(const uint16 src_port, const UUID& src, const uint16 dest_port, const UUID& dest,const std::string& payload);

    /* Ping Utility Methods. */
    bool ping(const Time& t, const Object *src, const UUID&dest, double distance=-0);

    boost::shared_ptr<Stream<UUID> > getSpaceStream(const UUID& objectID);

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

    struct SpaceNodeConnection;
    struct ConnectingInfo;

    // Service Implementation
    virtual void start();
    virtual void stop();


    // Private version of send that doesn't verify src UUID, allows us to masquerade for session purposes
    // The allow_connecting parameter allows you to use a connection over which the object is still opening
    // a connection.  This is safe since it can only be used by this class (since this is private), so it will
    // only be used to deal with session management.
    // If dest_server is NullServerID, then getConnectedServer is used to determine where to send the packet.
    // This is used to possibly exchange data between the main and IO strands, so it acquires locks.
    bool send(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server = NullServerID);



    // Starting point for handling of all messages from the server -- either handled as a special case, such as
    // for session management, or dispatched to the object
    void handleServerMessage(SpaceNodeConnection* conn);

    // Handles session messages received from the server -- connection replies, migration requests, etc.
    void handleSessionMessage(CBR::Protocol::Object::ObjectMessage* msg);
    void retryOpenConnection(const UUID&uuid,ServerID sid);


    // Utility method which keeps trying to resend a message
    void sendRetryingMessage(const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server, IOStrand* strand, const Duration& rate);

    /** SpaceNodeConnection initiation. */

    // Get an existing space connection or initiate a new one at random
    // which can be used for bootstrapping connections
    typedef std::tr1::function<void(SpaceNodeConnection*)> GotSpaceConnectionCallback;
    void getAnySpaceConnection(GotSpaceConnectionCallback cb);
    // Get the connection to the specified space node
    void getSpaceConnection(ServerID sid, GotSpaceConnectionCallback cb);

    // Set up a space connection to the given server
    void setupSpaceConnection(ServerID server, GotSpaceConnectionCallback cb);

    // Handle a connection event, i.e. the socket either successfully connected or failed
    void handleSpaceConnection(const Sirikata::Network::Stream::ConnectionStatus status,
                               const std::string&reason,
                               ServerID sid);


    /** Object session initiation. */

    // Private utility method that the public versions all use to initialize connection struct
  void openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, bool regquery, const SolidAngle& init_sa, ConnectedCallback connect_cb, MigratedCallback migrate_cb, StreamCreatedCallback);

    // Final callback in session initiation -- we have all the info and now just have to return it to the object
    void openConnectionStartSession(const UUID& uuid, SpaceNodeConnection* conn);


    /** Object session migration. */

    // Start the migration process for the object to the given server.
    void migrate(const UUID& obj_id, ServerID sid);

    // Callback that indicates we have a connection to the new server and can now start the migration to it.
    void openConnectionStartMigration(const UUID& uuid, ServerID sid, SpaceNodeConnection* conn);


    OptionSet* mStreamOptions;


    // THREAD SAFE
    // These may be accessed safely by any thread

    ObjectHostContext* mContext;

    IOService* mIOService;
    IOStrand* mIOStrand;
    IOWork* mIOWork;
    Thread* mIOThread;

    ServerIDMap* mServerIDMap;
    Duration mSimDuration;

    TimeProfiler::Stage* mHandleMessageProfiler;

    Sirikata::SerializationCheck mSerialization;

    // Main strand only

    // Connections to servers
    struct SpaceNodeConnection {
        typedef std::tr1::function<void(SpaceNodeConnection*)> ReceiveCallback;

        SpaceNodeConnection(ObjectHostContext* ctx, IOStrand* ioStrand, OptionSet *streamOptions, ServerID sid, ReceiveCallback rcb);
        ~SpaceNodeConnection();

        // Thread Safe
        ObjectHostContext* mContext;
        ObjectHost* parent;
        ServerID server;
        Sirikata::Network::Stream* socket;

        // Push a packet to be sent out
        bool push(ObjectMessage* msg);

        // Pull a packet from the receive queue
        ObjectMessage* pull();

        void shutdown();


        // Callback for when the connection receives data
        Sirikata::Network::Stream::ReceivedResponse handleRead(Sirikata::Network::Chunk& chunk);

        // Main Strand
        std::vector<GotSpaceConnectionCallback> connectCallbacks;
        bool connecting;

        // IO Strand
        QueueRouterElement<ObjectMessage> receive_queue;

        ReceiveCallback mReceiveCB;
    };
    // Only main strand accesses and manipulates the map, although other strand
    // may access the SpaceNodeConnection*'s.
    typedef std::map<ServerID, SpaceNodeConnection*> ServerConnectionMap;
    ServerConnectionMap mConnections;


    // Info associated with opening connections
    struct ConnectingInfo {
        TimedMotionVector3f loc;
        BoundingSphere3f bounds;
        bool regQuery;
        SolidAngle queryAngle;
    };


    // Objects connections, maintains object connections and mapping
    class ObjectConnections {
    public:
        ObjectConnections();

        // Add the object, completely disconnected, to the index
        void add(Object* obj, ConnectingInfo ci, ConnectedCallback connect_cb, MigratedCallback migrate_cb,
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


        // Get object for the object ID
        Object* object(const UUID& obj_id);
        // Lookup the server the object is connected to.  With allow_connecting, allows using
        // the server currently being connected to, not just one where a session has been
        // established
        ServerID getConnectedServer(const UUID& obj_id, bool allow_connecting = false);

        // Select random objects uniformly, uniformly from server, using round robin
        Object* randomObject(bool null_if_disconnected = false);
        Object* randomObject(ServerID whichServer, bool null_if_disconnected = false);
        Object* roundRobinObject(ServerID whichServer, bool null_if_disconnected = false);
        ServerID numServerIDs()const;
    private:
        struct ObjectInfo {
            ObjectInfo(Object* obj);
            ObjectInfo(); // Don't use, necessary for std::map

            Object* object;

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

        UUID mLastRRObject;
        size_t mLastRRIndex;
    };
    ObjectConnections mObjectConnections;

    bool mShuttingDown;
    uint64 mPingId;
    typedef std::tr1::function<void(const CBR::Protocol::Object::ObjectMessage&)> ObjectMessageCallback;
    std::tr1::unordered_map<uint64, ObjectMessageCallback > mRegisteredServices;


    void spaceConnectCallback(int err, boost::shared_ptr< Stream<UUID> > s, UUID obj);
    std::map<UUID, boost::shared_ptr<Stream<UUID> > > mObjectToSpaceStreams;

public:
    ObjectConnections*getObjectConnections(){return &mObjectConnections;}
    ///Register to intercept all incoming messages on a given port
    bool registerService(uint64 port, const ObjectMessageCallback&cb);
    ///Unregister to intercept all incoming messages on a given port
    bool unregisterService(uint64 port);
}; // class ObjectHost

} // namespace CBR


#endif //_CBR_OBJECT_HOST_HPP_
