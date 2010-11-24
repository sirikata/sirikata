/*  Sirikata liboh -- Object Host
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

#ifndef _SIRIKATA_OBJECT_HOST_HPP_
#define _SIRIKATA_OBJECT_HOST_HPP_

#include <sirikata/oh/Platform.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/network/Address.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/oh/SessionManager.hpp>

namespace Sirikata {
class ProxyManager;
class PluginManager;
class SpaceIDMap;
class SpaceConnection;
class ConnectionEventListener;
class ObjectScriptManager;

class ServerIDMap;

namespace Task {
class WorkQueue;
}
class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

typedef Provider< ConnectionEventListener* > ConnectionEventProvider;


class SIRIKATA_OH_EXPORT ObjectHost : public ConnectionEventProvider, public Service {

    ObjectHostContext* mContext;
    SpaceIDMap *mSpaceIDMap;

    typedef std::tr1::unordered_map<SpaceID,SessionManager*,SpaceID::Hasher> SpaceSessionManagerMap;

    typedef std::tr1::unordered_map<UUID, HostedObjectPtr, UUID::Hasher> HostedObjectMap;

    SpaceSessionManagerMap mSessionManagers;

    HostedObjectMap mHostedObjects;
    PluginManager *mScriptPlugins;
    std::tr1::unordered_map<String,OptionSet*> mSpaceConnectionProtocolOptions;

    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID, const TimedMotionVector3f&, const TimedMotionQuaternion&, const BoundingSphere3f&, const String&)> SessionConnectedCallback;
public:
    struct ConnectionInfo {
        ServerID server;
        TimedMotionVector3f loc;
        TimedMotionQuaternion orient;
        BoundingSphere3f bnds;
        String mesh;
    };

    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> SessionCallback;
    // Callback indicating that a connection to the server was made and it is available for sessions
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ConnectionInfo)> ConnectedCallback;
    // Callback indicating that a connection is being migrated to a new server.  This occurs as soon
    // as the object host starts the transition and no additional notification is given since, for all
    // intents and purposes this is the point at which the transition happens
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> MigratedCallback;
    typedef std::tr1::function<void(const SpaceObjectReference&)> StreamCreatedCallback;
    // Notifies the ObjectHost of object connection that was closed, including a
    // reason.
    typedef std::tr1::function<void(const SpaceObjectReference&, Disconnect::Code)> DisconnectedCallback;

    //FIXME: this call will have to go away sooner or later.
    SpaceID getDefaultSpace();

    /** Caller is responsible for starting a thread
     *
     * @param spaceIDMap space ID map used to resolve space IDs to servers
     * @param ioServ IOService to run this object host on
     * @param options a string containing the options to pass to the object host
     */
    ObjectHost(ObjectHostContext* ctx, SpaceIDMap *spaceIDMap, Network::IOService*ioServ, const String&options);
    /// The ObjectHost must be destroyed after all HostedObject instances.
    ~ObjectHost();

    // Space API - Provide info for ObjectHost to communicate with spaces
    void addServerIDMap(const SpaceID& space_id, ServerIDMap* sidmap);

    // Primary HostedObject API

    /** Connect the object to the space with the given starting parameters. */
    void connect(
        HostedObjectPtr obj, const SpaceID& space,
        const TimedMotionVector3f& loc,
        const TimedMotionQuaternion& orient,
        const BoundingSphere3f& bnds,
        const String& mesh,
        const SolidAngle& init_sa,
        ConnectedCallback connected_cb,
        MigratedCallback migrated_cb, StreamCreatedCallback stream_created_cb,
        DisconnectedCallback disconnected_cb
    );

    /** Disconnect the object from the space. */
    void disconnect(HostedObjectPtr obj, const SpaceID& space);

    /** Get offset of server time from client time for the given space. Should
     * only be called by objects with an active connection to that space.
     */
    Duration serverTimeOffset(const SpaceID& space) const;
    /** Get offset of client time from server time for the given space. Should
     * only be called by objects with an active connection to that space. This
     * is just a utility, is always -serverTimeOffset(). */
    Duration clientTimeOffset(const SpaceID& space) const;

    /** Primary ODP send function. */
    bool send(HostedObjectPtr src, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, const std::string& payload);
    bool send(HostedObjectPtr src, const SpaceID& space, const uint16 src_port, const UUID& dest, const uint16 dest_port, MemoryReference payload);



    /** Register object by private UUID, so that it is possible to
        talk to objects/services which are not part of any space.
        Done automatically by HostedObject::initialize* functions.
    */
    void registerHostedObject(const HostedObjectPtr &obj);
    /// Unregister a private UUID. Done automatically by ~HostedObject.
    void unregisterHostedObject(const UUID &objID);

    /** Lookup HostedObject by private UUID. */
    HostedObjectPtr getHostedObject(const UUID &id) const;

    /** Lookup the SST stream for a particular object. */
    typedef Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;
    SSTStreamPtr getSpaceStream(const SpaceID& space, const UUID& internalID);

    /// Returns the SpaceID -> Network::Address lookup map.
    SpaceIDMap*spaceIDMap(){return mSpaceIDMap;}


    virtual void start();
    virtual void stop();

    PluginManager *getScriptPluginManager(){return mScriptPlugins;}
    /// DEPRECATED
    ProxyManager *getProxyManager(const SpaceID&space) const;


    //void updateAddressable() const;


		void persistEntityState(const String&);

  private:
    // Session Management Implementation
    void handleObjectConnected(const UUID& internalID, ServerID server);
    void handleObjectMigrated(const UUID& internalID, ServerID from, ServerID to);
    void handleObjectMessage(const UUID& internalID, const SpaceID& space, Sirikata::Protocol::Object::ObjectMessage* msg);
    void handleObjectDisconnected(const UUID& internalID, Disconnect::Code);

    // Wrapper to convert callback to use ConnectionInfo
    void wrappedConnectedCallback(const SpaceID& space, const ObjectReference& obj, ServerID server, const TimedMotionVector3f& loc, const TimedMotionQuaternion& orient, const BoundingSphere3f& bnds, const String& mesh, ConnectedCallback cb);

    // Checks serialization of access to SessionManagers
    Sirikata::SerializationCheck mSessionSerialization;

}; // class ObjectHost

} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_HOST_HPP
