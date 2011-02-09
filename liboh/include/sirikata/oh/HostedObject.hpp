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

#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>

#include <map>
#include <list>
#include <utility>

#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/core/transfer/URI.hpp>

#include <sirikata/oh/ObjectHost.hpp>
#include <sirikata/oh/ObjectHostProxyManager.hpp>
#include <sirikata/oh/PerPresenceData.hpp>
#include <sirikata/proxyobject/SimulationFactory.hpp>

#include <sirikata/proxyobject/OrphanLocUpdateManager.hpp>

namespace Sirikata {

namespace Protocol {
namespace Loc {
class LocationUpdate;
}
}

class ProxyObject;
class ProxyObject;
struct LightInfo;
class PhysicalParameters;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
// ObjectHost_Sirikata.pbj.hpp

class ObjectScript;
class HostedObject;
class PerPresenceData;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;

class SIRIKATA_OH_EXPORT HostedObject : public VWObject {
//------- Private inner classes

    struct PrivateCallbacks;
protected:

//------- Members
    ObjectHostContext* mContext;

  public:
     struct EntityState
     {
       public:

         String objType;
         String subType;
         String name;
         Vector3d pos;
         Quaternion orient;
         Vector3f vel;
         Vector3f rot;
         float angular_speed;
         String mesh;
         float scale;
         String objectID;
         String script_type;
         String script_opts;
         void persistToFile(std::ofstream&);

     };
     typedef struct EntityState EntityState;

  private:
    //SpaceSet mSpaces;


    ObjectHost *mObjectHost;
    ObjectScript *mObjectScript;
    typedef std::map<SpaceObjectReference, PerPresenceData> PresenceDataMap;
    PresenceDataMap *mPresenceData;


    int mNextSubscriptionID;
    UUID mInternalObjectReference;

    ODP::DelegateService* mDelegateODPService;


    typedef EndPoint<SpaceObjectReference> EndPointType;
    typedef BaseDatagramLayer<SpaceObjectReference> BaseDatagramLayerType;
    typedef BaseDatagramLayerType::Ptr BaseDatagramLayerPtr;
    typedef Stream<SpaceObjectReference> SSTStream;
    typedef SSTStream::Ptr SSTStreamPtr;
    typedef Connection<SpaceObjectReference> SSTConnection;
    typedef SSTConnection::Ptr SSTConnectionPtr;

    // FIXME maintain a proper map here or put in per-presence data
    std::vector<BaseDatagramLayerPtr> mSSTDatagramLayers;

    OrphanLocUpdateManager mOrphanLocUpdates;

//------- Constructors/Destructors

private:
    friend class ::Sirikata::SelfWeakPtr<VWObject>;
    friend class PerPresenceData;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &uuid);

public:
/// Destructor: will only be called from shared_ptr::~shared_ptr.
    virtual ~HostedObject();


private:
//------- Private member functions:
    // When a connection to a space is setup, initialize it to handle default behaviors
    void initializePerPresenceData(PerPresenceData& psd, ProxyObjectPtr selfproxy);
public:

    /** Get a set of spaces the object is currently connected to. */
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
    void initializeScript(const String& script, const String& args);

    bool handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    void processInitScriptMessage(MemoryReference& body);
    bool handleScriptMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);

    /// Attempt to restore this item from database including script
    //void initializeRestoreFromDatabase(const SpaceID&spaceID);

    /** Initializes this HostedObject, particularly to get it set up with the
     *  underlying ObjectHost.
     */
    void init();
    void addSimListeners(PerPresenceData& pd, const String& oh_sims,    TimeSteppedSimulation*& sim);



    /** Removes this HostedObject from the ObjectHost, and destroys the internal shared pointer
      * Safe to reuse for another connection, as long as you hold a shared_ptr to this object.
      */
    void destroy();
    /** Gets the ObjectHost (usually one per host).
        See getProxy(space)->getProxyManger() for the per-space object.
    */
    ObjectHost *getObjectHost()const {return mObjectHost;}

    /// Gets the proxy object representing this HostedObject inside space.
    ///const ProxyObjectPtr &getProxy(const SpaceID &space) const;
    ProxyObjectPtr getProxy(const SpaceID& space, const ObjectReference& oref);
    const ProxyObjectPtr &getProxyConst(const SpaceID& space, const ObjectReference& oref) const;

public:

    ObjectReference getObjReference(const SpaceID& space);

    TimeSteppedSimulation* runSimulation(const SpaceObjectReference& sporef, const String& simName);

    bool getProxyObjectFrom(const SpaceObjectReference*   spVisTo, const SpaceObjectReference*   sporef, ProxyObjectPtr& p);

    /** Returns the internal object reference, which can be used for connecting
        to a space, talking to other objects within this object host, and
        persistence messages.
    */
    
    // const UUID &getUUID() const {
    //     return mInternalObjectReference;
    // }


    virtual ProxyManagerPtr getProxyManager(const SpaceID& space,const ObjectReference& oref);
    virtual ProxyManagerPtr getDefaultProxyManager(const SpaceID& space);
    virtual ProxyObjectPtr  getDefaultProxyObject(const SpaceID& space);

    /** Called once per frame, at a certain framerate. */
    void tick();


    const static int DEFAULT_PRESENCE_TOKEN = -1;
    /** Initiate connection process to a space, but do not send any messages yet.
        After calling connectToSpace, it is immediately possible to send() a NewObj
        message, however any other message must wait until you receive the RetObj
        for that space.
        @param spaceID  The UUID of the space you connect to.
        @param startingLocation  The initial location of this object. Must be known at connection time?
        @param meshBounds  The size of this mesh. If set incorrectly, mesh will be scaled to these bounds.
        @param evidence  Usually use getUUID(); can be set differently if needed
        for authentication.
        @param token  When connection completes, notifies all session
        listeners.  Provides token to these listeners so they can distinguish
        which presence may have connected, etc.
    */
    void connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const UUID&object_uuid_evidence,
        PerPresenceData* ppd,
        int token = DEFAULT_PRESENCE_TOKEN);

    void connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const SolidAngle& queryAngle,
        const UUID&object_uuid_evidence,
        PerPresenceData* ppd,
        int token = DEFAULT_PRESENCE_TOKEN);


    Location getLocation(const SpaceID& space, const ObjectReference& oref);

  private:

    // Because IOStrand->wrap() can't handle > 5 parameters (because the
    // underlying boost impementation doesnt), we need to handle wrapping
    // connection callbacks manually.


    void handleConnected(const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, PerPresenceData* ppd, int token);
    void handleConnectedIndirect(const SpaceID& space, const ObjectReference& obj, ObjectHost::ConnectionInfo info, PerPresenceData* ppd, int token);

    bool handleEntityCreateMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);

    void handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server);
    void handleStreamCreated(const SpaceObjectReference& spaceobj);
    void handleDisconnected(const SpaceObjectReference& spaceobj, Disconnect::Code cc);

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
    virtual void registerDefaultODPHandler(const ODP::MessageHandler& cb);


    // Movement Interface
    //note: location update services both position and velocity

    virtual void requestLocationUpdate(const SpaceID& space, const ObjectReference& oref,const TimedMotionVector3f& loc);


    virtual void requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos);
    virtual void requestVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& vel);

    virtual Vector3d requestCurrentPosition (const SpaceID& space,const ObjectReference& oref);
    virtual Vector3d requestCurrentPosition(ProxyObjectPtr proxy_obj);
    virtual Vector3f requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref);


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

    virtual void requestQueryUpdate(const SpaceID& space, const ObjectReference& oref, SolidAngle new_angle);
    virtual void requestQueryRemoval(const SpaceID& space, const ObjectReference& oref);

  private:
    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);

    // Handlers for substreams for space-managed updates
    void handleLocationSubstream(const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    void handleProximitySubstream(const SpaceObjectReference& spaceobj, int err, SSTStreamPtr s);
    // Handlers for substream read events for space-managed updates
    void handleLocationSubstreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream* prevdata, uint8* buffer, int length);
    void handleProximitySubstreamRead(const SpaceObjectReference& spaceobj, SSTStreamPtr s, std::stringstream** prevdata, uint8* buffer, int length);

    // Handlers for core space-managed updates
    void processLocationUpdate(const SpaceID& space, ProxyObjectPtr proxy_obj, const Sirikata::Protocol::Loc::LocationUpdate& update);
    void processLocationUpdate(const SpaceID& space, ProxyObjectPtr proxy_obj, uint64 seqno, bool predictive, TimedMotionVector3f* loc, TimedMotionQuaternion* orient, BoundingSphere3f* bounds, String* mesh);
    bool handleLocationMessage(const SpaceObjectReference& spaceobj, const std::string& paylod);
    bool handleProximityMessage(const SpaceObjectReference& spaceobj, const std::string& payload);

    // Helper for creating the correct type of proxy

    ProxyObjectPtr createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmvq, const BoundingSphere3f& bounds);
    ProxyObjectPtr buildProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const Transfer::URI& meshuri);
    ProxyObjectPtr createDummyProxy();

    // Helper for constructing and sending location update
    void sendLocUpdateRequest(const SpaceID& space, const ObjectReference& oref, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh);

    public:
    HostedObject::EntityState* getEntityState(const SpaceID& space, const ObjectReference& oref);
    void persistToFile(std::ofstream& file);
};


/// shared_ptr, keeps a reference to the HostedObject. Do not store one of these.
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
/// weak_ptr, to be used to hold onto a HostedObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;



}

#endif
