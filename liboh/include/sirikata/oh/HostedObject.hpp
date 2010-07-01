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
#include <sirikata/oh/TopLevelSpaceConnection.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/core/util/QueryTracker.hpp>
#include <sirikata/proxyobject/VWObject.hpp>

#include <sirikata/core/odp/DelegateService.hpp>
#include <sirikata/core/odp/DelegatePort.hpp>
#include <sirikata/oh/ObjectScriptManager.hpp>

#include <map>
#include <utility>

namespace Sirikata {
class ObjectHost;
class ProxyObject;
class ProxyObject;
struct LightInfo;
class PhysicalParameters;
typedef std::tr1::shared_ptr<ProxyObject> ProxyObjectPtr;
class TopLevelSpaceConnection;
// ObjectHost_Sirikata.pbj.hpp

class ObjectScript;
class HostedObject;
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
class SIRIKATA_OH_EXPORT HostedObject : public VWObject {
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
  public:
    typedef std::set<SpaceID> SpaceSet;
  private:
    SpaceSet mSpaces;
    typedef std::map<SpaceID, PerSpaceData> SpaceDataMap;
    SpaceDataMap *mSpaceData;

    // name -> encoded property message
    typedef std::tr1::unordered_map<String, PropertyCacheValue> PropertyMap;
    PropertyMap mProperties;
    int mNextSubscriptionID;
    ObjectScript *mObjectScript;
    ObjectHost *mObjectHost;
    UUID mInternalObjectReference;

    ODP::DelegateService* mDelegateODPService;

    QueryTracker* mDefaultTracker; // FIXME this is necessary because we're
                                   // using messaging outside of spaces in order
                                   // to communicate with the db, which is
                                   // required for initialization....
    //------- Constructors/Destructors
private:
    friend class ::Sirikata::SelfWeakPtr<VWObject>;
/// Private: Use "SelfWeakPtr<HostedObject>::construct(ObjectHost*)"
    HostedObject(ObjectHost*parent, const UUID &uuid);

public:
/// Destructor: will only be called from shared_ptr::~shared_ptr.
    virtual ~HostedObject();


private:
//------- Private member functions:
    PerSpaceData &cloneTopLevelStream(const SpaceID&,const std::tr1::shared_ptr<TopLevelSpaceConnection>&);
    ///When a message is destined for the RPC port of 0, split it into submessages and process those
    void handleRPCMessage(const RoutableMessageHeader &header, MemoryReference bodyData);
    ///When a message is destined for the persistence port, handle each persistence object accordingly
    void handlePersistenceMessage(const RoutableMessageHeader &header, MemoryReference bodyData);
    ///makes a new object with the bare minimum--assumed that a script or persistence fills in the rest.
    void sendNewObj(const Location&startingLocation, const BoundingSphere3f&meshBounds, const SpaceID&, const UUID&evidence);

    // When a connection to a space is setup, initialize it to handle default behaviors
    void initializePerSpaceData(PerSpaceData& psd, ProxyObjectPtr selfproxy);
public:

    /** Get a set of spaces the object is currently connected to. */
    const SpaceSet& spaces() const {
        return mSpaces;
    }

//------- Public member functions:
    ///makes a new object that is not in the persistence database.
    void initializeDefault(
            const String&mesh,
            const LightInfo *lightInfo,
            const String&webViewURL,
            const Vector3f&meshScale,
            const PhysicalParameters&physicalParameters);
    ///makes a new objects with objectName startingLocation mesh and connect to some interesting space [not implemented]
    void initializeScript(const String&script, const std::map<String,String> &args);
    /// Attempt to restore this item from database including script
    void initializeRestoreFromDatabase(const SpaceID&spaceID, const HostedObjectPtr&spaceConnectionHint=HostedObjectPtr());
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

    ObjectHostProxyManager *getProxyManager(const SpaceID &space) const {
        ProxyObjectPtr obj = getProxy(space);
        if (obj) {
            return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
        }
        return 0;
    }

    //bftm
    ObjectHostProxyManager *bftm_getProxyManager(const SpaceID &space) const {
        ProxyObjectPtr obj = getProxy(space);
        if (obj) {
            return static_cast<ObjectHostProxyManager*>(obj->getProxyManager());
        }
        return 0;
    }
    
	/** These are properties related the object script 
	    that would be attached to this hosted object in case
		it is a scripted object
	*/
	
	bool mHasScript;
	String mScriptType;
	ObjectScriptManager::Arguments mScriptArgs;


	bool hasScript()
	{
	  return mHasScript;
	}

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

    //FIXME implement SpaceConnection& connect(const SpaceID&space);
    //FIXME implement SpaceConnection& connect(const SpaceID&space, const SpaceConnection&example);

    struct SendService: public MessageService {
        HostedObject *ho;
        void processMessage(const RoutableMessageHeader &hdr, MemoryReference body) {
            ho->send(hdr, body);
        }
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
    } mSendService;

    struct ReceiveService: public MessageService {
        HostedObject *ho;
        void processMessage(const RoutableMessageHeader &hdr, MemoryReference body) {
            assert(hdr.has_source_space());
            ho->processRoutableMessage(hdr, body);
        }
        bool forwardMessagesTo(MessageService*) { return false; }
        bool endForwardingMessagesTo(MessageService*) { return false; }
    } mReceiveService;

public:
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

    /** Called once per frame, at a certain framerate. */
    void tick();

    /** Initiate connection process to a space, but do not send any messages yet.
        After calling connectToSpace, it is immediately possible to send() a NewObj
        message, however any other message must wait until you receive the RetObj
        for that space.
        @param spaceID  The UUID of the space you connect to.
        @param spaceConnectionHint  Another nearby object; may be set to null HostedObjectPtr().
        @param startingLocation  The initial location of this object. Must be known at connection time?
        @param meshBounds  The size of this mesh. If set incorrectly, mesh will be scaled to these bounds.
        @param evidence  Usually use getUUID(); can be set differently if needed for authentication.
    */
    void connectToSpace(
        const SpaceID&spaceID,
        const HostedObjectPtr&spaceConnectionHint,
        const Location&startingLocation,
        const BoundingSphere3f&meshBounds,
        const UUID&evidence);

    /// Disconnects from the given space by terminating the corresponding substream.
    void disconnectFromSpace(const SpaceID&id);

    /** Handles an incoming message, then passes the message to the scripting language. */
    void processRoutableMessage(const RoutableMessageHeader &hdr, MemoryReference body);

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

  public:

    ProxyManager* getProxyManager(const SpaceID&space);
    bool isLocal(const SpaceObjectReference&space)const;

    // Location
    virtual Location getLocation(const SpaceID& space);
    virtual void setLocation(const SpaceID& space, const Location& loc);

    //bftm Object Reference
    ObjectReference getObjReference(const SpaceID& space);


    // Visual (mesh)
    virtual Transfer::URI getVisual(const SpaceID& space);
    virtual void setVisual(const SpaceID& space, const Transfer::URI& vis);
    virtual Vector3f getVisualScale(const SpaceID& space);
    virtual void setVisualScale(const SpaceID& space, const Vector3f& scale);

    void removeQueryInterest(uint32 query_id, const ProxyObjectPtr&proxyObj, const SpaceObjectReference&proximateObjectId);
    void addQueryInterest(uint32 query_id, const SpaceObjectReference&proximateObjectId);
    std::tr1::shared_ptr<HostedObject> getSharedPtr() {
        return std::tr1::static_pointer_cast<HostedObject>(this->VWObject::getSharedPtr());
    }
    std::tr1::weak_ptr<HostedObject> getWeakPtr() {
        return std::tr1::static_pointer_cast<HostedObject>(this->VWObject::getSharedPtr());
    }


  public:
    // ODP::Service Interface
    virtual ODP::Port* bindODPPort(SpaceID space, ODP::PortID port);
    virtual ODP::Port* bindODPPort(SpaceID space);
    virtual void registerDefaultODPHandler(const ODP::MessageHandler& cb);
  private:
    ODP::DelegatePort* createDelegateODPPort(ODP::DelegateService* parentService, SpaceID space, ODP::PortID port);
    bool delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload);
};



/// shared_ptr, keeps a reference to the HostedObject. Do not store one of these.
typedef std::tr1::shared_ptr<HostedObject> HostedObjectPtr;
/// weak_ptr, to be used to hold onto a HostedObject reference. @see SelfWeakPtr.
typedef std::tr1::weak_ptr<HostedObject> HostedObjectWPtr;



}

#endif
