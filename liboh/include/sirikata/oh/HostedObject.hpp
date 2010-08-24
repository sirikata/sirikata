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
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/core/util/QueryTracker.hpp>
#include <sirikata/proxyobject/VWObject.hpp>

#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>

#include <map>
#include <utility>

#include <sirikata/core/network/ObjectMessage.hpp>
#include <sirikata/core/network/SSTImpl.hpp>

#include <sirikata/core/transfer/URI.hpp>

#include <sirikata/oh/ObjectHostProxyManager.hpp>

namespace Sirikata {
class ObjectHostContext;
class ObjectHost;
class ProxyObject;
class ProxyObject;
struct LightInfo;
class PhysicalParameters;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
// ObjectHost_Sirikata.pbj.hpp

class ObjectScript;
class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
class SIRIKATA_OH_EXPORT HostedObject : public VWObject, public ObjectMessageRouter, public ObjectMessageDispatcher {
//------- Private inner classes
    class PerSpaceData;
    struct PrivateCallbacks;
protected:

    struct PropertyCacheValue {
    private:
        /// TODO: Make this be a set of spaces with an associated subscription.
        bool mHasSubscription;
        int mSubscriptionID;
    public:
        String mData;
        Duration mTTL;
        PropertyCacheValue() : mHasSubscription(false), mTTL(Duration::zero()) {}
        PropertyCacheValue(String data, Duration TTL)
            : mHasSubscription(false), mData(data), mTTL(TTL) {
        }
        bool hasSubscriptionID() {
            return mHasSubscription;
        }
        int getSubscriptionID() {
            if (!mHasSubscription) {
                SILOG(cppoh,fatal,"getSubscriptionID called with no subscription!");
                return 0;
            }
            return mSubscriptionID;
        }
        void setSubscriptionID(int newID) {
            mHasSubscription=true;
            mSubscriptionID=newID;
        }
        void clearSubscriptionID() {
            mHasSubscription=false;
        }
    };

//------- Members
    ObjectHostContext* mContext;

    // public:
    //   //typedef std::set<SpaceID> SpaceSet;
    // private:
    
  private:
    //SpaceSet mSpaces;

    typedef std::map<SpaceID, PerSpaceData> SpaceDataMap;
    SpaceDataMap *mSpaceData;

    // name -> encoded property message
    typedef std::tr1::unordered_map<String, PropertyCacheValue> PropertyMap;
    PropertyMap mProperties;
    int mNextSubscriptionID;
    ObjectScript *mObjectScript;
    ObjectHost *mObjectHost;
    UUID mInternalObjectReference;
    bool mIsCamera; // FIXME hack so we can get a camera up and running, need
                    // more flexible selection of proxy type

    ODP::DelegateService* mDelegateODPService;
    boost::shared_ptr<BaseDatagramLayer<UUID> >  mSSTDatagramLayer;


//------- Constructors/Destructors

private:
    friend class ::Sirikata::SelfWeakPtr<VWObject>;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &uuid, bool is_camera);

public:
/// Destructor: will only be called from shared_ptr::~shared_ptr.
    virtual ~HostedObject();


private:
//------- Private member functions:
    ///When a message is destined for the RPC port of 0, split it into submessages and process those
    void handleRPCMessage(const RoutableMessageHeader &header, MemoryReference bodyData);
    ///When a message is destined for the persistence port, handle each persistence object accordingly
    //void handlePersistenceMessage(const RoutableMessageHeader &header, MemoryReference bodyData);

    // When a connection to a space is setup, initialize it to handle default behaviors
    void initializePerSpaceData(PerSpaceData& psd, ProxyObjectPtr selfproxy);
public:

    /** Get a set of spaces the object is currently connected to. */
    typedef std::set<SpaceID> SpaceSet;
    void getSpaces(SpaceSet& ss) const;
    //bftm re-write
    // const SpaceSet& spaces() const {
    //     return mSpaces;
    // }

    

//------- Public member functions:
    ObjectHostContext* context() { return mContext; }
    const ObjectHostContext* context() const { return mContext; }

    ///makes a new object that is not in the persistence database.
    void initializeDefault(
            const String&mesh,
            const LightInfo *lightInfo,
            const String&webViewURL,
            const Vector3f&meshScale,
            const PhysicalParameters&physicalParameters);
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space [not implemented]
    //void initializeScript(const String&script, const std::map<String,String> &args);
    void initializeScript(const String& script, const ObjectScriptManager::Arguments &args, const std::string& fileScriptToAttach="");
    
    bool handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    void processInitScriptMessage(MemoryReference& body);
    bool handleScriptMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    
    /// Attempt to restore this item from database including script
    //void initializeRestoreFromDatabase(const SpaceID&spaceID);

    /** Initializes this HostedObject, particularly to get it set up with the
     *  underlying ObjectHost.
     */
    void init();

    /** Removes this HostedObject from the ObjectHost, and destroys the internal shared pointer
      * Safe to reuse for another connection, as long as you hold a shared_ptr to this object.
      */
    void destroy();
    /** Gets the ObjectHost (usually one per host).
        See getProxy(space)->getProxyManger() for the per-space object.
    */
    ObjectHost *getObjectHost()const {return mObjectHost;}
    const Duration&getSpaceTimeOffset(const SpaceID&space);
    /// Gets the proxy object representing this HostedObject inside space.
    const ProxyObjectPtr &getProxy(const SpaceID &space) const;
    ProxyObjectPtr getProxy(const SpaceID& space, const ObjectReference& oref);

    
	/** These are properties related the object script 
	    that would be attached to this hosted object in case
		it is a scripted object
	*/
	
    bool mHasScript;
    String mScriptType;
    ObjectScriptManager::Arguments mScriptArgs;
    String mScriptName;


    bool hasScript()
    {
        return mHasScript;
    }

    void setHasScript(bool t)
    {
        mHasScript = t;
    }
    
    void setScriptType(String s)
    {
        mScriptType = s;
    }

    void setScriptArgs(ObjectScriptManager::Arguments& args)
    {
        mScriptArgs = args;
    }

    String getScriptName()
    {
        return mScriptName;
    }
    void setScriptName(String s)
    {
        mScriptName = s;
    }

    void attachScript(const String& );

    // ObjectMessageRouter Interface
    WARN_UNUSED
    virtual bool route(Sirikata::Protocol::Object::ObjectMessage* msg);

protected:

    /// Checks for a public cached property named propName.
    bool hasProperty(const String &propName) const;
    /// Gets the protobuf encoded value of a public cached property named propName.
    const String &getProperty(const String &propName) const;

    /** Gets a modifiable value in the cache for a (often new) public property.
        Is usually used in conjunction with google::protobuf::message::SerializeToString:
        myMessage.SerializeToString(propertyPtr("SomeProperty"));
        -- Note: this will not update the Subscription --
    */
    String *propertyPtr(const String &propName, Duration ttl);
    /** Sets a cached property with an optional encoded value.
        -- Note: this will not update the Subscription --
     */
    void setProperty(const String &propName, Duration ttl, const String &encodedValue=String());
    /** Deletes a public property from this object cache. It may still remain in the database.
     */
    void unsetCachedPropertyAndSubscription(const String &propName);

    struct SendService: public MessageService {
        HostedObject *ho;
        void processMessage(const RoutableMessageHeader &hdr, MemoryReference body) {
            ho->send(hdr, body);
        }
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
    } mSendService;

public:


    ObjectReference getObjReference(const SpaceID& space);

    
    /** Returns the internal object reference, which can be used for connecting
        to a space, talking to other objects within this object host, and
        persistence messages.
    */
    const UUID &getUUID() const {
        return mInternalObjectReference;
    }
    /// Returns QueryTracker object that tracks of message ids awaiting reply.
    QueryTracker* getTracker(const SpaceID& space);
    /// Returns QueryTracker object that tracks of message ids awaiting reply (const edition).
    const QueryTracker*getTracker(const SpaceID& space) const;

    virtual ProxyManagerPtr getProxyManager(const SpaceID& space);

    /** Called once per frame, at a certain framerate. */
    void tick();

    /** Initiate connection process to a space, but do not send any messages yet.
        After calling connectToSpace, it is immediately possible to send() a NewObj
        message, however any other message must wait until you receive the RetObj
        for that space.
        @param spaceID  The UUID of the space you connect to.
        @param startingLocation  The initial location of this object. Must be known at connection time?
        @param meshBounds  The size of this mesh. If set incorrectly, mesh will be scaled to these bounds.
        @param evidence  Usually use getUUID(); can be set differently if needed for authentication.
    */
    void connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f&meshBounds,
        const String& mesh,
        const UUID&evidence,
        const String& scriptFile="",
        const String& scriptType="");
    
    Location getLocation(const SpaceID& space);

  private:
    
    void handleConnected(const SpaceID& space, const ObjectReference& obj, ServerID server,const Location& startingLocation, const String& scriptFile, const String& scriptType );
    void handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server);
    void handleStreamCreated(const SpaceID& space);

  public:
    /// Disconnects from the given space by terminating the corresponding substream.
    void disconnectFromSpace(const SpaceID&id);

    /// Receive an ObjectMessage from the space via the ObjectHost. Translate it
    /// to our runtime ODP structure and deliver it.
    void receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg);

  private:
    

    /** Sends directly via an attached space, without going through the ObjectHost.
        No messages with a null SpaceId (i.e. for a local object or message service)
        may be delivered using this path.
        @see send
    */
    void sendViaSpace(const RoutableMessageHeader &hdr, MemoryReference body);

    /** Sends a message from the space hdr.destination_space() to the object
        hdr.destination_object(). Note that this will properly route locally destined
        messages via a WorkQueue in the ObjectHost.
        @param header  A RoutableMessageHeader: must include destination_space/object.
        @param body  An encoded RoutableMessageBody.
    */
    void send(const RoutableMessageHeader &header, MemoryReference body);

    /** Handles a single RPC out of a received message.
        @param msg  A ReceivedMessage struct with sender, message_name, and
                    arguments.
        @param name the name of the RPC to invoke
        @param args the serialized RPC arguments
        @param returnValue  A serialized message indicating a return value.
               If NULL, no ID was passed, and no response will be sent.
               If non-NULL, a message is expected to be encoded, but the empty
               string is a valid message if a return value does not apply.
        @see ReceivedMessage
    */
    void processRPC(const RoutableMessageHeader &msg, const std::string &name, MemoryReference args, String *returnValue);

    bool handleEntityCreateMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData);
    
  public:
    //BFTM_FIXME: need to actually write this function.
    void updateAddressable();

    std::tr1::shared_ptr<HostedObject> getSharedPtr() {
        return std::tr1::static_pointer_cast<HostedObject>(this->VWObject::getSharedPtr());
    }
    std::tr1::weak_ptr<HostedObject> getWeakPtr() {
        return std::tr1::static_pointer_cast<HostedObject>(this->VWObject::getSharedPtr());
    }


  public:
    // Identification
    virtual SpaceObjectReference id(const SpaceID& space) const;

    // ODP::Service Interface
    virtual ODP::Port* bindODPPort(SpaceID space, ODP::PortID port);
    virtual ODP::Port* bindODPPort(SpaceID space);
    virtual void registerDefaultODPHandler(const ODP::MessageHandler& cb);
    virtual void registerDefaultODPHandler(const ODP::OldMessageHandler& cb);

    // Movement Interface
    //note: location update services both position and velocity

    virtual void requestLocationUpdate(const SpaceID& space, const TimedMotionVector3f& loc);

    virtual void requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos);
    virtual void requestVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& vel);

    virtual Vector3d requestCurrentPosition (const SpaceID& space,const ObjectReference& oref);
    virtual Vector3f requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref);

    //should re-name one or the other
    virtual void requestOrientationUpdate(const SpaceID& space, const TimedMotionQuaternion& orient);
    virtual void requestOrientationDirectionUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& orient);
    virtual Quaternion requestCurrentQuatVel(const SpaceID& space, const ObjectReference& oref);
    virtual Quaternion requestCurrentOrientation(const SpaceID& space, const ObjectReference& oref);
    
    virtual void requestBoundsUpdate(const SpaceID& space, const BoundingSphere3f& bounds);
    virtual void requestMeshUpdate(const SpaceID& space, const String& mesh);
    

    virtual void requestScaleUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& toScaleTo);
    virtual bool requestCurrentScale(const SpaceID& space, const ObjectReference& oref, Vector3f& scaler);

    

    virtual bool requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri);

    
  private:
    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService* parentService, SpaceID space, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);

    // Handlers for core space-managed updates
    void handleLocationMessage(const SpaceID& space, uint8* buffer, int len);
    void handleProximityMessage(const SpaceID& space, uint8* buffer, int len);

    // Helper for creating the correct type of proxy
    //ProxyObjectPtr createProxy(const SpaceObjectReference& objref, const Transfer::URI& meshuri, bool is_camera = false);
    ProxyObjectPtr createProxy(const SpaceObjectReference& objref, const Transfer::URI& meshuri, bool is_camera, const Location& startingLoc);
    ProxyObjectPtr createProxy(const SpaceObjectReference& objref, const Transfer::URI& meshuri, bool is_camera, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmvq);
    ProxyObjectPtr buildProxy(const SpaceObjectReference& objref, const Transfer::URI& meshuri, bool is_camera);
    
    // Helper for constructing and sending location update
    void sendLocUpdateRequest(const SpaceID& space, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh);
};



/// shared_ptr, keeps a reference to the HostedObject. Do not store one of these.
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
/// weak_ptr, to be used to hold onto a HostedObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;



}

#endif
