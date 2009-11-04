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
#include "TimeProfiler.hpp"
#include "QueueRouterElement.hpp"
#include "BandwidthShaper.hpp"
#include "StreamTxElement.hpp"
#include "PollingService.hpp"
#include "TimeProfiler.hpp"
#include "Message.hpp"

namespace CBR {

class Object;
class ServerIDMap;

class ObjectHost : public PollingService {
public:
    // Callback indicating that a connection to the server was made and it is available for sessions
    typedef std::tr1::function<void(ServerID)> ConnectedCallback;

    // FIXME the ServerID is used to track unique sources, we need to do this separately for object hosts
    ObjectHost(ObjectHostContext* ctx, ObjectFactory* obj_factory, Trace* trace, ServerIDMap* sidmap);

    const ObjectHostContext* context() const;

    void openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, const SolidAngle& init_sa, ConnectedCallback cb);
    void openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, ConnectedCallback cb);

    // FIXME should not be infinite queue and should report push error
    bool send(const Time&t, const Object* src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload);
    void sendTestMessage(const Time&t, float idealDistance);
    void ping(const Object *src, const UUID&dest, double distance=-0);
    void randomPing(const Time&t);

    Object* randomObject();
    Object* randomObject(ServerID whichServer);
    Object * roundRobinObject(ServerID whichServer);
    UUID mLastRRObject;
    size_t mLastRRIndex;
    OptionSet*mStreamOptions;
private:
    struct SpaceNodeConnection;

    /** Work for each iteration of PollingService. */
    virtual void poll();

    // Utility method to lookup the ServerID an object is connected to
    ServerID getConnectedServer(const UUID& obj_id, bool allow_connecting = false);

    // Private version of send that doesn't verify src UUID, allows us to masquerade for session purposes
    // The allow_connecting parameter allows you to use a connection over which the object is still opening
    // a connection.  This is safe since it can only be used by this class (since this is private), so it will
    // only be used to deal with session management.
    bool send(const Time&t, const UUID& src, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload, ServerID dest_server);


    // Starting point for handling of all messages from the server -- either handled as a special case, such as
    // for session management, or dispatched to the object
    void handleServerMessage(SpaceNodeConnection* conn, CBR::Protocol::Object::ObjectMessage* msg);

    // Handles session messages received from the server -- connection replies, migration requests, etc.
    void handleSessionMessage(CBR::Protocol::Object::ObjectMessage* msg);


    /** SpaceNodeConnection initiation. */

    // Get an existing space connection or initiate a new one at random
    // which can be used for bootstrapping connections
    typedef std::tr1::function<void(SpaceNodeConnection*)> GotSpaceConnectionCallback;
    void getSpaceConnection(GotSpaceConnectionCallback cb);
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
    void openConnection(Object* obj, const TimedMotionVector3f& init_loc, const BoundingSphere3f& init_bounds, bool regquery, const SolidAngle& init_sa, ConnectedCallback cb);

    // Final callback in session initiation -- we have all the info and now just have to return it to the object
    void openConnectionStartSession(const UUID& uuid, SpaceNodeConnection* conn);


    /** Object session migration. */

    // Start the migration process for the object to the given server.
    void migrate(const UUID& obj_id, ServerID sid);

    // Callback that indicates we have a connection to the new server and can now start the migration to it.
    void openConnectionStartMigration(const UUID& uuid, ServerID sid, SpaceNodeConnection* conn);



    /** Reading and writing handling for SpaceNodeConnections. */
    // Handle async reading callbacks for this connection
    bool handleConnectionRead(SpaceNodeConnection* conn,Sirikata::Network::Chunk& chunk);


    ObjectHostContext* mContext;
    ServerIDMap* mServerIDMap;
    Duration mSimDuration;
    TimeProfiler::Stage* mProfiler;

    // Connections to servers
    struct SpaceNodeConnection {
        SpaceNodeConnection(ObjectHostContext* ctx, OptionSet *streamOptions, ServerID sid);
        ~SpaceNodeConnection();

        ServerID server;
        Sirikata::Network::Stream* socket;

        std::vector<GotSpaceConnectionCallback> connectCallbacks;

        QueueRouterElement<std::string> queue;
        BandwidthShaper<std::string> rateLimiter;
        StreamTxElement<std::string> streamTx;
        bool connecting;
    };
    typedef std::map<ServerID, SpaceNodeConnection*> ServerConnectionMap;
    ServerConnectionMap mConnections;

    // Objects connections
    struct ObjectInfo {
        ObjectInfo(Object* obj);
        ObjectInfo(); // Don't use, necessary for std::map

        Object* object;

        // Info associated with opening connections
        struct ConnectingInfo {
            TimedMotionVector3f loc;
            BoundingSphere3f bounds;
            bool regQuery;
            SolidAngle queryAngle;

            ConnectedCallback cb;
        };
        ConnectingInfo connecting;
        ServerID connectingTo;
        // Server currently connected to
        ServerID connectedTo;
        // Server we're trying to migrate to
        ServerID migratingTo;
    };
    typedef std::tr1::unordered_map<ServerID, std::vector<UUID> > ObjectServerMap;
    ObjectServerMap mObjectServerMap;
    typedef std::tr1::unordered_map<UUID, ObjectInfo, UUID::Hasher> ObjectInfoMap;
    ObjectInfoMap mObjectInfo;
    uint64 mPingId;
}; // class ObjectHost

} // namespace CBR


#endif //_CBR_OBJECT_HOST_HPP_
