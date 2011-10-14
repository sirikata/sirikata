/*  Sirikata liboh -- Object Host
 *  HostedObject.hpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
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
#ifndef _SIRIKATA_HOSTED_OBJECT_HPP_
#define _SIRIKATA_HOSTED_OBJECT_HPP_

#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/VWObject.hpp>
#include <sirikata/core/util/Platform.hpp>
#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>

#include <map>
#include <list>
#include <utility>

#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/core/transfer/URI.hpp>

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>

#include <sirikata/proxyobject/OrphanLocUpdateManager.hpp>

namespace Sirikata {

namespace Protocol {
namespace Loc {
class LocationUpdate;
}
namespace Prox {
class ProximityResults;
}
}

class ProxyObject;
class ProxyObject;
struct LightInfo;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;

class ObjectScript;
class HostedObject;
class PerPresenceData;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

class SIRIKATA_OH_EXPORT HostedObject : public VWObject, public Service {
//------- Private inner classes

    struct PrivateCallbacks;
protected:

//------- Members
    ObjectHostContext* mContext;
    const UUID mID;

  private:

    ObjectHost *mObjectHost;
    ObjectScript *mObjectScript;
    typedef std::map<SpaceObjectReference, PerPresenceData*> PresenceDataMap;
    PresenceDataMap mPresenceData;

    int mNextSubscriptionID;
    bool destroyed;

    ODP::DelegateService* mDelegateODPService;

    OrphanLocUpdateManager mOrphanLocUpdates;

//------- Constructors/Destructors

private:
    friend class ::Sirikata::SelfWeakPtr<VWObject>;
    friend class PerPresenceData;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    /** Create a new HostedObject. script_type and script_opts are treated as
     * "use defaults" if you pass NULL and "use no script" if you pass empty
     * strings.  Invalid values are equivalent to passing NULL, since no scripts
     * can be instantiated -- there is no fallback to the default if you
     * explicitly specify a script type and options.
     *
     * \param _id A unique identifier for this object within this object
     * host. You can specify it at construction so that objects can be restored
     * from permanent storage.  You should always specify a non-null value, even
     * if you need to manually allocate a new random identifier.
     */
    HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &_id);

public:
    typedef SST::EndPoint<SpaceObjectReference> EndPointType;
    typedef SST::BaseDatagramLayer<SpaceObjectReference> BaseDatagramLayerType;
    typedef BaseDatagramLayerType::Ptr BaseDatagramLayerPtr;
    typedef SST::Connection<SpaceObjectReference> SSTConnection;
    typedef SSTConnection::Ptr SSTConnectionPtr;
    typedef SST::Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;

/// Destructor: will only be called from shared_ptr::~shared_ptr.
    virtual ~HostedObject();

    /** Get a unique identifier for this object on this object host. This is for
     *  internal use only -- it has nothing to do with any space or presences in
     *  the space. It is *not* a public value.
     */
    const UUID& id() const;

    // Sirikata::Service interface
    virtual void start();
    virtual void stop();

    bool stopped() const;
private:
//------- Private member functions:
    // When a connection to a space is setup, initialize it to handle default behaviors
    void initializePerPresenceData(PerPresenceData& psd, ProxyObjectPtr selfproxy);
public:

    /** Get a set of spaces the object is currently connected to.
     *  NOTE: Be very careful with this. It reports everything connected,
     *  including presences where we haven't established the SST stream with the
     *  space, which lots of listeners (including SessionEventListeners) rely
     *  on.
     */
    typedef std::vector<SpaceObjectReference> SpaceObjRefVec;
    void getSpaceObjRefs(SpaceObjRefVec& ss) const;
    void getProxySpaceObjRefs(const SpaceObjectReference& sporef,SpaceObjRefVec& ss) const;


//------- Public member functions:
    ObjectHostContext* context() { return mContext; }
    const ObjectHostContext* context() const { return mContext; }

    virtual Time spaceTime(const SpaceID& space, const Time& t);
    virtual Time currentSpaceTime(const SpaceID& space);
    virtual Time localTime(const SpaceID& space, const Time& t);
    virtual Time currentLocalTime();

    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space
    void initializeScript(const String& script_type, const String& args, const String& script);

    bool addSimListeners(PerPresenceData& pd, const String& oh_sims,    TimeSteppedSimulation*& sim);



    /** Removes this HostedObject from the ObjectHost, and destroys the internal shared pointer
      * Safe to reuse for another connection, as long as you hold a shared_ptr to this object.
      */
    void destroy(bool need_self = true);
    /** Gets the ObjectHost (usually one per host).
        See getProxy(space)->getProxyManger() for the per-space object.
    */
    ObjectHost *getObjectHost()const {return mObjectHost;}

    /// Gets the proxy object representing this HostedObject inside space.
    ///const ProxyObjectPtr &getProxy(const SpaceID &space) const;
    ProxyObjectPtr getProxy(const SpaceID& space, const ObjectReference& oref);
    bool getProxy(const SpaceObjectReference* sporef, ProxyObjectPtr& p);
    const ProxyObjectPtr &getProxyConst(const SpaceID& space, const ObjectReference& oref) const;

public:

    ObjectReference getObjReference(const SpaceID& space);

    TimeSteppedSimulation* runSimulation(const SpaceObjectReference& sporef, const String& simName);

    bool getProxyObjectFrom(const SpaceObjectReference*   spVisTo, const SpaceObjectReference*   sporef, ProxyObjectPtr& p);

    /** Returns the internal object reference, which can be used for connecting
        to a space, talking to other objects within this object host, and
        persistence messages.
    */



    virtual ProxyManagerPtr getProxyManager(const SpaceID& space,const ObjectReference& oref);
    virtual ProxyManagerPtr getDefaultProxyManager(const SpaceID& space);
    virtual ProxyObjectPtr  getDefaultProxyObject(const SpaceID& space);

    /** Called once per frame, at a certain framerate. */
    void tick();


    typedef int64 PresenceToken;
    const static PresenceToken DEFAULT_PRESENCE_TOKEN = -1;


    /** Initiate connection process to a space, but do not send any messages yet.
        After calling connectToSpace, it is immediately possible to send() a NewObj
        message, however any other message must wait until you receive the RetObj
        for that space.
        @param spaceID  The UUID of the space you connect to.
        @param startingLocation  The initial location of this object. Must be known at connection time?
        @param meshBounds  The size of this mesh. If set incorrectly, mesh will
        be scaled to these bounds.
        @param mesh the URL of the mesh for this object
        @param physics Physical parameters, serialized to a string. The exact
        format depends on the physics implementation the server is using.
        @param evidence  Usually use getUUID(); can be set differently if needed
        for authentication.
        @param token  When connection completes, notifies all session
        listeners.  Provides token to these listeners so they can distinguish
        which presence may have connected, etc.
    */
    bool connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& physics,
        const UUID&object_uuid_evidence,
        PresenceToken token = DEFAULT_PRESENCE_TOKEN);

    bool connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& physics,
        const String& query,
        const UUID&object_uuid_evidence,
        const ObjectReference& orefID,
        PresenceToken token = DEFAULT_PRESENCE_TOKEN);

    /** \deprecated */
    bool connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const String& physics,
        const SolidAngle& queryAngle,
        uint32 queryMaxResults,
        const UUID&object_uuid_evidence,
        const ObjectReference& orefID,
        PresenceToken token = DEFAULT_PRESENCE_TOKEN);


    Location getLocation(const SpaceID& space, const ObjectReference& oref);

  private:

    // Because IOStrand->wrap() can't handle > 5 parameters (because the
    // underlying boost impementation doesnt), we need to handle wrapping
    // connection callbacks manually.


    static void handleConnected(const HostedObjectWPtr &weakSelf, const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info);
    static void handleConnectedIndirect(const HostedObjectWPtr &weakSelf, const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, const BaseDatagramLayerPtr&);

//    static bool handleEntityCreateMessage(const HostedObjectWPtr &weakSelf, const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    static void handleMigrated(const HostedObjectWPtr &weakSelf, const SpaceID& space, const ObjectReference& obj, ServerID server);
    static void handleStreamCreated(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, SessionManager::ConnectionEvent after, PresenceToken token);
    static void handleDisconnected(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, Disconnect::Code cc);

  public:
    /// Disconnects from the given space by terminating the corresponding substream.
    void disconnectFromSpace(const SpaceID &spaceID, const ObjectReference& oref);

    /// Receive an ObjectMessage from the space via the ObjectHost. Translate it
    /// to our runtime ODP structure and deliver it.
    void receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg);


  public:


    std::tr1::shared_ptr<HostedObject> getSharedPtr() {
        return std::tr1::static_pointer_cast<HostedObject>(this->VWObject::getSharedPtr());
    }
    std::tr1::weak_ptr<HostedObject> getWeakPtr() {
        return std::tr1::static_pointer_cast<HostedObject>(this->VWObject::getSharedPtr());
    }

    Network::IOService* getIOService();
    //void registerTimeoutCallback(const Duration& dur, std::tr1::function<void( );


  public:
    // Identification
    // virtual SpaceObjectReference id(const SpaceID& space) const;

    virtual ProxyManagerPtr presence(const SpaceObjectReference& sor);
    virtual ProxyObjectPtr self(const SpaceObjectReference& sor);

    // ODP::Service Interface
    virtual ODP::Port* bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port);
    virtual ODP::Port* bindODPPort(const SpaceObjectReference& sor, ODP::PortID port);
    virtual ODP::Port* bindODPPort(const SpaceID& space, const ObjectReference& objref);
    virtual ODP::Port* bindODPPort(const SpaceObjectReference& sor);
    virtual ODP::PortID unusedODPPort(const SpaceID& space, const ObjectReference& objref);
    virtual ODP::PortID unusedODPPort(const SpaceObjectReference& sor);
    virtual void registerDefaultODPHandler(const ODP::Service::MessageHandler& cb);


    // Movement Interface
    //note: location update services both position and velocity

    virtual void requestLocationUpdate(const SpaceID& space, const ObjectReference& oref,const TimedMotionVector3f& loc);


    virtual void requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos);
    virtual void requestVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& vel);

    virtual Vector3d requestCurrentPosition (const SpaceID& space,const ObjectReference& oref);
    virtual Vector3d requestCurrentPosition(ProxyObjectPtr proxy_obj);
    virtual Vector3f requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref);
    virtual Vector3f requestCurrentVelocity(ProxyObjectPtr proxy_obj);

    virtual void requestOrientationUpdate(const SpaceID& space, const ObjectReference& oref, const TimedMotionQuaternion& orient);

    virtual void requestOrientationDirectionUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& orient);
    virtual void requestOrientationVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& quat);

    virtual Quaternion requestCurrentQuatVel(const SpaceID& space, const ObjectReference& oref);
    virtual Quaternion requestCurrentOrientation(const SpaceID& space, const ObjectReference& oref);
    virtual Quaternion requestCurrentOrientationVel(const SpaceID& space, const ObjectReference& oref);


    virtual BoundingSphere3f requestCurrentBounds(const SpaceID& space,const ObjectReference& oref);
    virtual void requestBoundsUpdate(const SpaceID& space, const ObjectReference& oref, const BoundingSphere3f& bounds);
    virtual void requestMeshUpdate(const SpaceID& space, const ObjectReference& oref, const String& mesh);

    virtual bool requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri);

    virtual const String& requestCurrentPhysics(const SpaceID& space,const ObjectReference& oref);
    virtual void requestPhysicsUpdate(const SpaceID& space, const ObjectReference& oref, const String& phy);

    virtual void requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, const String& new_query);
    /** \deprecated */
    virtual void requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, const SolidAngle& sa, uint32 max_count);
    // Shortcut for requestQueryUpdate("")
    virtual void requestQueryRemoval(const SpaceID& space, const ObjectReference& oref);
    virtual const String& requestQuery(const SpaceID& space, const ObjectReference& oref);


    // ObjectQuerier Interface
    void handleProximityMessage(const SpaceObjectReference& spaceobj, const Sirikata::Protocol::Prox::ProximityResults& results);


  private:
    /** \deprecated
     *  Helper for encoding default, solid angle queries. Used to enable old,
     *  deprecated API for setting queries.
     */
    String encodeDefaultQuery(const SolidAngle& qangle, const uint32 max_count);

    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);


    // Handlers for substreams for space-managed updates
    static void handleLocationSubstream(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    // Handlers for substream read events for space-managed updates
    static void handleLocationSubstreamRead(const HostedObjectWPtr &weakSelf, const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);

    // Handlers for core space-managed updates
    void processLocationUpdate(const SpaceObjectReference& sporef, ProxyObjectPtr proxy_obj, const Sirikata::Protocol::Loc::LocationUpdate& update);
    void processLocationUpdate(const SpaceID& space, ProxyObjectPtr proxy_obj, uint64 seqno, bool predictive, TimedMotionVector3f* loc, TimedMotionQuaternion* orient, BoundingSphere3f* bounds, String* mesh, String* phy);
    bool handleLocationMessage(const SpaceObjectReference& spaceobj, const std::string& paylod);

    /**
       Any time that we get a prox removal call, we first save the existing
       proxy object's data to the orphan manager.  If we get a prox addition
       call on the same object before the proxy object's data is removed,
       the orphan manager will return these data to us to process.  (by process,
       I mean update the pos, bounds, orient, etc. for the proxy object).
     */
    void processOrphanedProxyData(const SpaceObjectReference& sporef, ProxyObjectPtr proxy_obj,OrphanLocUpdateManager::OrphanedProxData* opd);

    // Helper for creating the correct type of proxy

    ProxyObjectPtr createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmvq, const BoundingSphere3f& bounds, const String& physics, const String& query, uint64 seqNo);
    ProxyObjectPtr createDummyProxy();

    // Helper for constructing and sending location update
    void updateLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh, const String* const phy);
    void sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref);

};


/// shared_ptr, keeps a reference to the HostedObject. Do not store one of these.
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
/// weak_ptr, to be used to hold onto a HostedObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;



}

#endif
