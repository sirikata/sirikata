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
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/core/network/Address.hpp>
#include <sirikata/core/util/ListenerProvider.hpp>
#include <sirikata/core/service/Service.hpp>
#include <sirikata/oh/SessionManager.hpp>
#include <sirikata/core/ohdp/Service.hpp>
#include <sirikata/oh/SpaceNodeSession.hpp>
#include <sirikata/oh/ObjectNodeSession.hpp>

namespace Sirikata {
class ProxyManager;
class PluginManager;
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

namespace OH {
class Storage;
class PersistedObjectSet;
class ObjectQueryProcessor;
}

class SIRIKATA_OH_EXPORT ObjectHost
    : public ConnectionEventProvider,
      public Service,
      public OHDP::Service,
      public SpaceNodeSessionManager, private SpaceNodeSessionListener,
      public ObjectNodeSessionProvider
{

    ObjectHostContext* mContext;

    typedef std::tr1::unordered_map<SpaceID,SessionManager*,SpaceID::Hasher> SpaceSessionManagerMap;

    typedef std::tr1::unordered_map<SpaceObjectReference, HostedObjectPtr, SpaceObjectReference::Hasher> HostedObjectMap;

    OH::Storage* mStorage;
    OH::PersistedObjectSet* mPersistentSet;
    OH::ObjectQueryProcessor* mQueryProcessor;

    SpaceSessionManagerMap mSessionManagers;

    uint32 mActiveHostedObjects;
    HostedObjectMap mHostedObjects;

    typedef std::tr1::unordered_map<String, ObjectScriptManager*> ScriptManagerMap;
    ScriptManagerMap mScriptManagers;

    std::tr1::unordered_map<String,OptionSet*> mSpaceConnectionProtocolOptions;
    ///options passed to initialization of scripts (usually path information)
    std::map<std::string, std::string > mSimOptions;
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID, const TimedMotionVector3f&, const TimedMotionQuaternion&, const BoundingSphere3f&, const String&, const String&)> SessionConnectedCallback;

public:
    String getSimOptions(const String&);
    struct ConnectionInfo {
        ServerID server;
        TimedMotionVector3f loc;
        TimedMotionQuaternion orient;
        BoundingSphere3f bnds;
        String mesh;
        String physics;
        String query;
    };

    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> SessionCallback;
    // Callback indicating that a connection to the server was made and it is available for sessions
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ConnectionInfo)> ConnectedCallback;
    // Callback indicating that a connection is being migrated to a new server.  This occurs as soon
    // as the object host starts the transition and no additional notification is given since, for all
    // intents and purposes this is the point at which the transition happens
    typedef std::tr1::function<void(const SpaceID&, const ObjectReference&, ServerID)> MigratedCallback;
    typedef std::tr1::function<void(const SpaceObjectReference&, SessionManager::ConnectionEvent after)> StreamCreatedCallback;
    // Notifies the ObjectHost of object connection that was closed, including a
    // reason.
    typedef std::tr1::function<void(const SpaceObjectReference&, Disconnect::Code)> DisconnectedCallback;

    /** Caller is responsible for starting a thread
     *
     * @param ioServ IOService to run this object host on
     * @param options a string containing the options to pass to the object host
     */
    ObjectHost(ObjectHostContext* ctx, Network::IOService*ioServ, const String&options);
    /// The ObjectHost must be destroyed after all HostedObject instances.
    ~ObjectHost();

    /** Create an object with the specified script. This version allows you to
     *  specify the unique identifier manually, so it should only be used if you
     *  need an exact ID, e.g. if you are restoring an object.
     *
     *  \param _id a unique identifier for this object. Only use this
     *  \param script_type type of script to instantiate, e.g. 'js'
     *  \param script_opts options to pass to the created script
     *  \param script_contents contents of the script, i.e. the script text to eval
     */
    virtual HostedObjectPtr createObject(const UUID &_id, const String& script_type, const String& script_opts, const String& script_contents);
    /** Create an object with the specified script. The object will be
     *  automatically assigned a unique identifier.
     *
     *  \param script_type type of script to instantiate, e.g. 'js'
     *  \param script_opts options to pass to the created script
     *  \param script_contents contents of the script, i.e. the script text to eval
     */
    virtual HostedObjectPtr createObject(const String& script_type, const String& script_opts, const String& script_contents);

    virtual const String& defaultScriptType() const = 0;
    virtual const String& defaultScriptOptions() const = 0;
    virtual const String& defaultScriptContents() const = 0;

    // Space API - Provide info for ObjectHost to communicate with spaces
    void addServerIDMap(const SpaceID& space_id, ServerIDMap* sidmap);

    // Get and set the storage backend to use for persistent object storage.
    void setStorage(OH::Storage* storage) { mStorage = storage; }
    OH::Storage* getStorage() { return mStorage; }

    // Get and set the storage backend to use for the set of persistent objects.
    void setPersistentSet(OH::PersistedObjectSet* persistentset) { mPersistentSet = persistentset; }
    OH::PersistedObjectSet* getPersistedObjectSet() { return mPersistentSet; }

    // Get and set the storage backend to use for queries.
    void setQueryProcessor(OH::ObjectQueryProcessor* proc) { mQueryProcessor = proc; }
    OH::ObjectQueryProcessor* getQueryProcessor() { return mQueryProcessor; }

    // Primary HostedObject API

    /** Connect the object to the space with the given starting parameters.
    *   returns true if the connection was initiated and no other objects are
    *   using this ID to connect.
    */
    bool connect(
        HostedObjectPtr ho, // requestor, or can be NULL
        const SpaceObjectReference& sporef, const SpaceID& space,
        const TimedMotionVector3f& loc,
        const TimedMotionQuaternion& orient,
        const BoundingSphere3f& bnds,
        const String& mesh,
        const String& physics,
        const String& query,
        ConnectedCallback connected_cb,
        MigratedCallback migrated_cb, StreamCreatedCallback stream_created_cb,
        DisconnectedCallback disconnected_cb
    );

    /** Use this function to request the object host to send a disconnect message
     *  to space for object.
     */
    void disconnectObject(const SpaceID& space, const ObjectReference& oref);

    /** Get offset of server time from client time for the given space. Should
     * only be called by objects with an active connection to that space.
     */
    Duration serverTimeOffset(const SpaceID& space) const;
    /** Get offset of client time from server time for the given space. Should
     * only be called by objects with an active connection to that space. This
     * is just a utility, is always -serverTimeOffset(). */
    Duration clientTimeOffset(const SpaceID& space) const;

    /** Convert a local time into a time for the given space.
     *  \param space the space to translate to
     *  \param t the local time to convert
     */
    Time spaceTime(const SpaceID& space, const Time& t);
    /** Get the current time in the given space */
    Time currentSpaceTime(const SpaceID& space);
    /** Convert a time in the given space to a local time.
     *  \param space the space to translate from
     *  \param t the time in the space to convert to a local time
     */
    Time localTime(const SpaceID& space, const Time& t);
    /** Get the current local time. */
    Time currentLocalTime();

    /** Primary ODP send function. */
    bool send(SpaceObjectReference& sporefsrc, const SpaceID& space, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, const std::string& payload);
    bool send(SpaceObjectReference& sporefsrc, const SpaceID& space, const ObjectMessagePort src_port, const UUID& dest, const ObjectMessagePort dest_port, MemoryReference payload);




    /** Register object by private UUID, so that it is possible to
        talk to objects/services which are not part of any space.
        Done automatically by HostedObject::initialize* functions.
    */
    void registerHostedObject(const SpaceObjectReference &sporef_uuid, const HostedObjectPtr& obj);
    /// Unregister a private UUID. Done automatically by ~HostedObject.
    void unregisterHostedObject(const SpaceObjectReference& sporef_uuid, HostedObject *obj);
    /* Notify the ObjectHost that . Only called by HostedObject. */
    void hostedObjectDestroyed(const UUID& objid);

    /** Lookup HostedObject by one of it's presence IDs. This may return an
     *  empty pointer if no objects have tried to connect. The returned object
     *  may also still be in the process of connecting that presence.
     */
    HostedObjectPtr getHostedObject(const SpaceObjectReference &id) const;

    /** Lookup the SST stream for a particular object. */
    typedef SST::Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;
    SSTStreamPtr getSpaceStream(const SpaceID& space, const ObjectReference& internalID);

    // Service Interface
    virtual void start();
    virtual void stop();

    // OHDP::Service Interface
    virtual OHDP::Port* bindOHDPPort(const SpaceID& space, const OHDP::NodeID& node, OHDP::PortID port);
    virtual OHDP::Port* bindOHDPPort(const SpaceID& space, const OHDP::NodeID& node);
    virtual OHDP::PortID unusedOHDPPort(const SpaceID& space, const OHDP::NodeID& node);
    virtual void registerDefaultOHDPHandler(const MessageHandler& cb);

    // The object host will instantiate script managers and pass them user
    // specified flags.  These can then be reused by calling this method to get
    // at them.
    ObjectScriptManager* getScriptManager(const String& id);

  private:

    // SpaceNodeSessionListener Interface -- forwards on to real listeners
    virtual void onSpaceNodeSession(const OHDP::SpaceNodeID& id, OHDPSST::Stream::Ptr sn_stream) { fireSpaceNodeSession(id, sn_stream); }
    virtual void onSpaceNodeSessionEnded(const OHDP::SpaceNodeID& id) { fireSpaceNodeSessionEnded(id); }

    // Session Management Implementation
    void handleObjectConnected(const SpaceObjectReference& sporef_internalID, ServerID server);
    void handleObjectMigrated(const SpaceObjectReference& sporef_internalID, ServerID from, ServerID to);
    void handleObjectMessage(const SpaceObjectReference& sporef_internalID, const SpaceID& space, Sirikata::Protocol::Object::ObjectMessage* msg);
    void handleObjectDisconnected(const SpaceObjectReference& sporef_internalID, Disconnect::Code);

    // Wrappers so we can forward events to interested parties. For Connected
    // callback, also allows us to convert ConnectionInfo.
    void wrappedConnectedCallback(HostedObjectWPtr ho_weak, const SpaceID& space, const ObjectReference& obj, const SessionManager::ConnectionInfo& ci, ConnectedCallback cb);
    void wrappedStreamCreatedCallback(HostedObjectWPtr ho_weak, const SpaceObjectReference& sporef, SessionManager::ConnectionEvent after, StreamCreatedCallback cb);
    void wrappedDisconnectedCallback(HostedObjectWPtr ho_weak, const SpaceObjectReference& sporef, Disconnect::Code cause, DisconnectedCallback);

    // Checks serialization of access to SessionManagers
    Sirikata::SerializationCheck mSessionSerialization;

    void handleDefaultOHDPMessageHandler(const OHDP::Endpoint& src, const OHDP::Endpoint& dst, MemoryReference payload);

    OHDP::MessageHandler mDefaultOHDPMessageHandler;
}; // class ObjectHost

} // namespace Sirikata

#endif //_SIRIKATA_OBJECT_HOST_HPP
