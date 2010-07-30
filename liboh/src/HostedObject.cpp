/*  Sirikata liboh -- Object Host
 *  HostedObject.cpp
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

#include <sirikata/oh/Platform.hpp>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyLightObject.hpp>
#include <sirikata/proxyobject/ProxyWebViewObject.hpp>
#include <sirikata/proxyobject/ProxyCameraObject.hpp>
#include <sirikata/proxyobject/LightInfo.hpp>
#include <Protocol_Sirikata.pbj.hpp>
#include <Protocol_Subscription.pbj.hpp>
#include <Protocol_Persistence.pbj.hpp>
#include <sirikata/core/task/WorkQueue.hpp>
#include <sirikata/core/util/RoutableMessage.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/persistence/PersistenceSentMessage.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/util/SpaceObjectReference.hpp>
#include <sirikata/oh/HostedObject.hpp>
#include <sirikata/oh/ObjectHostContext.hpp>
#include <sirikata/core/util/SentMessage.hpp>
#include <sirikata/oh/ObjectHost.hpp>

#include <sirikata/oh/ObjectScriptManager.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/oh/ObjectScript.hpp>
#include <sirikata/oh/ObjectScriptManagerFactory.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

#include "Protocol_Loc.pbj.hpp"
#include "Protocol_Prox.pbj.hpp"

#define HO_LOG(lvl,msg) SILOG(ho,lvl,"[HO] " << msg);

namespace Sirikata {

typedef SentMessageBody<RoutableMessageBody> RPCMessage;

OptionValue *defaultTTL;

InitializeGlobalOptions hostedobject_props("",
    defaultTTL=new OptionValue("defaultTTL",".1",OptionValueType<Duration>(),"Default TTL for HostedObject properties"),
    NULL
);

class HostedObject::PerSpaceData {
public:

    HostedObject* parent;
    SpaceID space;
    ObjectReference object;
    ProxyObjectPtr mProxyObject;
    ProxyObject::Extrapolator mUpdatedLocation;

    ODP::Port* rpcPort;
    ODP::Port* persistencePort;

    QueryTracker* tracker;
    ObjectHostProxyManagerPtr proxyManager;

    SpaceObjectReference id() const { return SpaceObjectReference(space, object); }

    void locationWasReset(Time timestamp, Location loc) {
        loc.setVelocity(Vector3f::nil());
        loc.setAngularSpeed(0);
        mUpdatedLocation.resetValue(timestamp, loc);
    }
    void locationWasSet(const Protocol::ObjLoc &msg) {
        Time timestamp = msg.timestamp();
        Location loc = mUpdatedLocation.extrapolate(timestamp);
        //ProxyObject::updateLocationWithObjLoc(loc, msg);
        loc.setVelocity(Vector3f::nil());
        loc.setAngularSpeed(0);
        mUpdatedLocation.updateValue(timestamp, loc);
    }

    void updateLocation(HostedObject *ho) {
        if (!mProxyObject) {
            return;
        }
        SpaceID space = mProxyObject->getObjectReference().space();
        Time now = Time::now(ho->getSpaceTimeOffset(space));
        Location realLocation = mProxyObject->globalLocation(now);
        if (mUpdatedLocation.needsUpdate(now, realLocation)) {
            Protocol::ObjLoc toSet;
            toSet.set_position(realLocation.getPosition());
            toSet.set_velocity(realLocation.getVelocity());
            RoutableMessageBody body;
            toSet.SerializeToString(body.add_message("ObjLoc"));
            RoutableMessageHeader header;
            header.set_destination_port(Services::LOC);
            header.set_destination_object(ObjectReference::spaceServiceID());
            header.set_destination_space(space);
            std::string bodyStr;
            body.SerializeToString(&bodyStr);
            // Avoids waiting a loop.
            ho->sendViaSpace(header, MemoryReference(bodyStr));

            locationWasSet(toSet);
        }
    }

    typedef std::map<uint32, std::set<ObjectReference> > ProxQueryMap;
    ProxQueryMap mProxQueryMap; ///< indexed by ProxCall::query_id()

    PerSpaceData(HostedObject* _parent, const SpaceID& _space)
     : parent(_parent),
       space(_space),
       object(ObjectReference::null()),
       mUpdatedLocation(
            Duration::seconds(.1),
            TemporalValue<Location>::Time::null(),
            Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                     Vector3f(0,0,0),Vector3f(0,1,0),0),
            ProxyObject::UpdateNeeded()),
       rpcPort(NULL),
       persistencePort(NULL),
       tracker(NULL),
       proxyManager(new ObjectHostProxyManager(_space))
    {
    }

    void initializeAs(ProxyObjectPtr proxyobj) {
        object = proxyobj->getObjectReference().object();

        mProxyObject = proxyobj;
        rpcPort = parent->bindODPPort(space, ODP::PortID((uint32)Services::RPC));
        persistencePort = parent->bindODPPort(space, ODP::PortID((uint32)Services::PERSISTENCE));

        // Use any port for tracker
        tracker = new QueryTracker(parent->bindODPPort(space), parent->mContext->ioService);
        tracker->forwardMessagesTo(&parent->mSendService);
    }

    void destroy(QueryTracker *tracker) const {
        delete rpcPort;
        delete persistencePort;

        if (tracker) {
            tracker->endForwardingMessagesTo(&parent->mSendService);
            delete tracker;
        }
    }
};



HostedObject::HostedObject(ObjectHostContext* ctx, ObjectHost*parent, const UUID &objectName, bool is_camera)
 : mContext(ctx),
   mInternalObjectReference(objectName),
   mIsCamera(is_camera)
{
    mSpaceData = new SpaceDataMap;
    mNextSubscriptionID = 0;
    mObjectHost=parent;
    mObjectScript=NULL;
    mSendService.ho = this;

    mDelegateODPService = new ODP::DelegateService(
        std::tr1::bind(
            &HostedObject::createDelegateODPPort, this,
            _1, _2, _3
        )
    );

    mSSTDatagramLayer = BaseDatagramLayer<UUID>::createDatagramLayer(getUUID(), this, this);
}

HostedObject::~HostedObject() {
    destroy();
    delete mSpaceData;
}

void HostedObject::init() {
    mObjectHost->registerHostedObject(getSharedPtr());
}

void HostedObject::destroy() {
    if (mObjectScript) {
        delete mObjectScript;
        mObjectScript=NULL;
    }
    for (SpaceDataMap::const_iterator iter = mSpaceData->begin();
         iter != mSpaceData->end();
         ++iter) {
        iter->second.destroy(getTracker(iter->first));
    }
    mSpaceData->clear();
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
    mProperties.clear();
}

struct HostedObject::PrivateCallbacks {

    static bool needsSubscription(const PropertyCacheValue &pcv) {
        return pcv.mTTL != Duration::zero();
    }

    static void initializeDatabaseCallback(
        const HostedObjectWPtr &weakThis,
        const SpaceID &spaceID,
        Persistence::SentReadWriteSet *msg,
        const RoutableMessageHeader &lastHeader,
        Persistence::Protocol::Response::ReturnStatus errorCode)
    {
        HostedObjectPtr realThis=weakThis.lock();
        if (!realThis) {
            return;//deleted before database read
        }
        if (lastHeader.has_return_status() || errorCode) {
            SILOG(cppoh,error,"Database error recieving Loc and scripting info: "<<(int)lastHeader.return_status()<<": "<<(int)errorCode);
            delete msg;
            return; // unable to get starting position.
        }
        String scriptName;
        std::map<String,String> scriptParams;
        Location location(Vector3d::nil(),Quaternion::identity(),Vector3f::nil(),Vector3f(1,0,0),0);
        for (int i = 0; i < msg->body().reads_size(); i++) {
            String name = msg->body().reads(i).field_name();
            if (msg->body().reads(i).has_return_status() || !msg->body().reads(i).has_data()) {
                continue;
            }
            Duration ttl = msg->body().reads(i).has_ttl() ? msg->body().reads(i).ttl() : defaultTTL->as<Duration>();
            if (!name.empty() && name[0] != '_') {
                realThis->setProperty(name, ttl, msg->body().reads(i).data());
            }
            if (name == "Loc") {
                Protocol::ObjLoc loc;
                loc.ParseFromString(msg->body().reads(i).data());
                SILOG(cppoh,debug,"Creating object "<<ObjectReference(realThis->getUUID())
                      <<" at position "<<loc.position());
                if (loc.has_position()) {
                    location.setPosition(loc.position());
                }
                if (loc.has_orientation()) {
                    location.setOrientation(loc.orientation());
                }
                if (loc.has_velocity()) {
                    location.setVelocity(loc.velocity());
                }
                if (loc.has_rotational_axis()) {
                    location.setAxisOfRotation(loc.rotational_axis());
                }
                if (loc.has_angular_speed()) {
                    location.setAngularSpeed(loc.angular_speed());
                }
            }
            if (name == "_Script") {
                Protocol::StringProperty scrProp;
                scrProp.ParseFromString(msg->body().reads(i).data());
                scriptName = scrProp.value();
            }
            if (name == "_ScriptParams") {
                Protocol::StringMapProperty scrProp;
                scrProp.ParseFromString(msg->body().reads(i).data());
                int numkeys = scrProp.keys_size();
                {
                    int numvalues = scrProp.values_size();
                    if (numvalues < numkeys) {
                        numkeys = numvalues;
                    }
                }
                for (int i = 0; i < numkeys; i++) {
                    scriptParams[scrProp.keys(i)] = scrProp.values(i);
                }
            }
        }
        // Temporary Hack because we do not have access to the CDN here.
        BoundingSphere3f sphere(Vector3f::nil(),realThis->hasProperty("IsCamera")?1.0:1.0);
        realThis->connect(spaceID, location, sphere, "", realThis->getUUID());
        delete msg;
        if (!scriptName.empty()) {
            realThis->initializeScript(scriptName, scriptParams);
        }
    }

    static void handlePersistenceResponse(
        HostedObject *realThis,
        const RoutableMessageHeader &origHeader,
        SentMessage *sent,
        const RoutableMessageHeader &header,
        MemoryReference bodyData)
    {
        std::auto_ptr<SentMessageBody<Persistence::Protocol::ReadWriteSet> > sentDestruct(static_cast<SentMessageBody<Persistence::Protocol::ReadWriteSet> *>(sent));
        SILOG(cppoh,debug,"Got some persistence back: stat = "<<(int)header.return_status());
        if (header.has_return_status()) {
            Persistence::Protocol::Response resp;
            for (int i = 0, respIndex=0; i < sentDestruct->body().reads_size(); i++, respIndex++) {
                Persistence::Protocol::IStorageElement field = resp.add_reads();
                if (sentDestruct->body().reads(i).has_index()) {
                    field.set_index(sentDestruct->body().reads(i).index());
                }
                if (sentDestruct->body().options() & Persistence::Protocol::ReadWriteSet::RETURN_READ_NAMES) {
                    field.set_field_name(sentDestruct->body().reads(i).field_name());
                }
                field.set_return_status(Persistence::Protocol::StorageElement::KEY_MISSING);
            }
            std::string errorData;
            resp.SerializeToString(&errorData);
            RoutableMessageHeader replyHeader = origHeader.createReply();
            realThis->sendViaSpace(replyHeader, MemoryReference(errorData));
        } else {
            Persistence::Protocol::Response resp;
            resp.ParseFromArray(bodyData.data(), bodyData.length());
            for (int i = 0, respIndex=0; i < resp.reads_size(); i++, respIndex++) {
                if (resp.reads(i).has_index()) {
                    respIndex=resp.reads(i).index();
                }
                const Persistence::Protocol::StorageElement &field = resp.reads(i);
                if (respIndex >= 0 && respIndex < sentDestruct->body().reads_size()) {
                    const Persistence::Protocol::StorageElement &sentField = sentDestruct->body().reads(i);
                    const std::string &fieldName = sentField.field_name();
                    Duration ttl = field.has_ttl() ? field.ttl() : defaultTTL->as<Duration>();
                    if (field.has_data()) {
                        realThis->setProperty(fieldName, ttl, field.data());
                        PropertyCacheValue &cachedProp = realThis->mProperties[fieldName];
                        if (!cachedProp.hasSubscriptionID() && needsSubscription(cachedProp)) {
                            cachedProp.setSubscriptionID(realThis->mNextSubscriptionID++);
                        }
                        if (cachedProp.hasSubscriptionID()) {
                            resp.reads(i).set_subscription_id(cachedProp.getSubscriptionID());
                        }
                    }
                }
                ++respIndex;
            }
            std::string newBodyData;
            resp.SerializeToString(&newBodyData);
            RoutableMessageHeader replyHeader = origHeader.createReply();
            realThis->sendViaSpace(replyHeader, MemoryReference(newBodyData));
        }
    }

    static void disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
        std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
        if (thus) {
            SpaceDataMap::iterator where=thus->mSpaceData->find(sid);
            if (where!=thus->mSpaceData->end()) {
                where->second.destroy(thus->getTracker(sid));
                thus->mSpaceData->erase(where);//FIXME do we want to back this up to the database first?
            }
        }
    }

    static void connectionEvent(const HostedObjectWPtr&thus,
                                const SpaceID&sid,
                                Network::Stream::ConnectionStatus ce,
                                const String&reason) {
        if (ce!=Network::Stream::Connected) {
            disconnectionEvent(thus,sid,reason);
        }
    }
};


QueryTracker* HostedObject::getTracker(const SpaceID& space) {
    SpaceDataMap::iterator it = mSpaceData->find(space);
    if (it == mSpaceData->end()) return NULL;
    return it->second.tracker;
}

const QueryTracker* HostedObject::getTracker(const SpaceID& space) const {
    SpaceDataMap::const_iterator it = mSpaceData->find(space);
    if (it == mSpaceData->end()) return NULL;
    return it->second.tracker;
}

ProxyManagerPtr HostedObject::getProxyManager(const SpaceID& space) {
    SpaceDataMap::const_iterator it = mSpaceData->find(space);
    if (it == mSpaceData->end()) {
        it = mSpaceData->insert(
            SpaceDataMap::value_type( space, PerSpaceData(this, space) )
        ).first;
    }
    return it->second.proxyManager;
}



void HostedObject::handleRPCMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
    HostedObject *realThis=this;
    /// Parse message_names and message_arguments.

    RoutableMessageBody msg;
    msg.ParseFromArray(bodyData.data(), bodyData.length());
    int numNames = msg.message_size();
    if (numNames <= 0) {
        // Invalid message!
        RoutableMessageHeader replyHeader = header.createReply();
        replyHeader.set_return_status(RoutableMessageHeader::PROTOCOL_ERROR);
        sendViaSpace(replyHeader, MemoryReference::null());
        return;
    }

    RoutableMessageBody responseMessage;
    for (int i = 0; i < numNames; ++i) {
        std::string name = msg.message_names(i);
        MemoryReference body(msg.message_arguments(i));

        if (header.has_id()) {
            std::string response;
            /// Pass response parameter if we expect a response.
            realThis->processRPC(header, name, body, &response);
            responseMessage.add_message_reply(response);
        } else {
            /// Return value not needed.
            realThis->processRPC(header, name, body, NULL);
        }
    }

    if (header.has_id()) {
        std::string serializedResponse;
        responseMessage.SerializeToString(&serializedResponse);
        RoutableMessageHeader replyHeader = header.createReply();
        realThis->sendViaSpace(replyHeader, MemoryReference(serializedResponse));
    }
}

/* NOTE: Broken because mDefaultTracker no longer exists.
void HostedObject::handlePersistenceMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
        using namespace Persistence::Protocol;
        HostedObject *realThis=this;
        ReadWriteSet rws;
        rws.ParseFromArray(bodyData.data(), bodyData.length());

        Response immedResponse;
        int immedIndex = 0;

        SpaceID space = header.destination_space();
        // FIXME this should use: getTracker(space); but that makes things break
        // because the space identifiers are getting removed when sending to
        // Persistence, causing the replies to just get lost.
        QueryTracker* space_query_tracker = mDefaultTracker;

        SentMessageBody<ReadWriteSet> *persistenceMsg = new SentMessageBody<ReadWriteSet>(space_query_tracker,std::tr1::bind(&PrivateCallbacks::handlePersistenceResponse, realThis, header, _1, _2, _3));
        int outIndex = 0;
        ReadWriteSet &outMessage = persistenceMsg->body();
        if (rws.has_options()) {
            outMessage.set_options(rws.options());
        }
        SILOG(cppoh,debug,"Got a Persistence message: reads size = "<<rws.reads_size()<<
              " writes size = "<<rws.writes_size());

        for (int i = 0, rwsIndex=0 ; i < rws.reads_size(); i++, rwsIndex++) {
            if (rws.reads(i).has_index()) {
                rwsIndex = rws.reads(i).index();
            }
            std::string name;
            if (rws.reads(i).has_field_name()) {
                name = rws.reads(i).field_name();
            }
            bool fail = false;
            if (name.empty() || name[0] == '_') {
                SILOG(cppoh,debug,"Invalid GetProp: "<<name);
                fail = true;
            } else {
                if (realThis->hasProperty(name)) {
                    PropertyCacheValue &cachedProp = realThis->mProperties[name];
                    // Cached property--respond immediately.
                    SILOG(cppoh,debug,"Cached GetProp: "<<name<<" = "<<realThis->getProperty(name));
                    IStorageElement el = immedResponse.add_reads();
                    if (immedIndex != rwsIndex) {
                        el.set_index(rwsIndex);
                    }
                    immedIndex = rwsIndex+1;
                    if (rws.options() & ReadWriteSet::RETURN_READ_NAMES) {
                        el.set_field_name(rws.reads(i).field_name());
                    }
                    el.set_ttl(cachedProp.mTTL);
                    el.set_data(cachedProp.mData);
                    if (!cachedProp.hasSubscriptionID() && PrivateCallbacks::needsSubscription(cachedProp)) {
                        cachedProp.setSubscriptionID(mNextSubscriptionID++);
                    }
                    if (cachedProp.hasSubscriptionID()) {
                        el.set_subscription_id(cachedProp.getSubscriptionID());
                    }
                } else {
                    SILOG(cppoh,debug,"Forward GetProp: "<<name<<" to Persistence");
                    IStorageElement el = outMessage.add_reads();
                    if (outIndex != rwsIndex) {
                        el.set_index(rwsIndex);
                    }
                    outIndex = rwsIndex+1;
                    el.set_field_name(rws.reads(i).field_name());
                    el.set_object_uuid(realThis->getUUID());
                }
            }
            if (fail) {
                IStorageElement el = immedResponse.add_reads();
                if (immedIndex != rwsIndex) {
                    el.set_index(rwsIndex);
                }
                immedIndex = rwsIndex+1;
                if (rws.options() & ReadWriteSet::RETURN_READ_NAMES) {
                    el.set_field_name(rws.reads(i).field_name());
                }
                el.set_return_status(StorageElement::KEY_MISSING);
            }
        }
        outIndex = 0;
        for (int i = 0, rwsIndex=0 ; i < rws.writes_size(); i++, rwsIndex++) {
            if (rws.writes(i).has_index()) {
                rwsIndex = rws.writes(i).index();
            }
            std::string name;
            if (rws.writes(i).has_field_name()) {
                name = rws.writes(i).field_name();
            }
            bool fail = false;
            if (name.empty() || name[0] == '_') {
                SILOG(cppoh,debug,"Invalid SetProp: "<<name);
                fail = true;
            } else {
                if (rws.writes(i).has_data()) {
                    Duration ttl = rws.writes(i).has_ttl() ? rws.writes(i).ttl() : defaultTTL->as<Duration>();
                    realThis->setProperty(name, ttl, rws.writes(i).data());
                    PropertyCacheValue &cachedProp = mProperties[name];
                    if (cachedProp.hasSubscriptionID()) {
                        SpaceDataMap::const_iterator spaceiter = mSpaceData->begin();
                        for (;spaceiter != mSpaceData->end(); ++spaceiter) {
                            int subID = cachedProp.getSubscriptionID();
                            ::Sirikata::Protocol::Broadcast subMsg;
                            std::string subStr;
                            subMsg.set_broadcast_name(subID);
                            subMsg.set_data(cachedProp.mData);
                            subMsg.SerializeToString(&subStr);
                            RoutableMessageHeader header;
                            header.set_destination_port(Services::BROADCAST);
                            header.set_destination_space(spaceiter->first);
                            header.set_destination_object(ObjectReference::spaceServiceID());
                            sendViaSpace(header, MemoryReference(subStr.data(), subStr.length()));
                        }
                    }
                    SpaceDataMap::iterator iter;
                    for (iter = realThis->mSpaceData->begin();
                         iter != realThis->mSpaceData->end();
                         ++iter) {
                        realThis->receivedPropertyUpdate(iter->second.mProxyObject, name, rws.writes(i).data());
                    }
                } else {
                    if (name != "LightInfo" && name != "MeshURI" && name != "IsCamera" && name != "WebViewURL") {
                        // changing the type of this object has to wait until we reload from database.
                        realThis->unsetCachedPropertyAndSubscription(name);
                    }
                }
                SILOG(cppoh,debug,"Forward SetProp: "<<name<<" to Persistence");
                IStorageElement el = outMessage.add_writes();
                if (outIndex != rwsIndex) {
                    el.set_index(rwsIndex);
                }
                outIndex = rwsIndex+1;
                el.set_field_name(rws.writes(i).field_name());
                if (rws.writes(i).has_data()) {
                    el.set_data(rws.writes(i).data());
                }
                el.set_object_uuid(realThis->getUUID());
            }
            // what to do if a write fails?
        }

        if (immedResponse.reads_size()) {
            SILOG(cppoh,debug,"ImmedResponse: "<<immedResponse.reads_size());
            std::string respStr;
            immedResponse.SerializeToString(&respStr);
            RoutableMessageHeader replyHeader = header.createReply();
            realThis->sendViaSpace(replyHeader, MemoryReference(respStr));
        }
        if (outMessage.reads_size() || outMessage.writes_size()) {
            SILOG(cppoh,debug,"ForwardToPersistence: "<<outMessage.reads_size()<<
                  " reads and "<<outMessage.writes_size()<<"writes");
            persistenceMsg->header().set_destination_space(SpaceID::null());
            persistenceMsg->header().set_destination_object(ObjectReference::spaceServiceID());
            persistenceMsg->header().set_destination_port(Services::PERSISTENCE);

            persistenceMsg->serializeSend();
        } else {
            delete persistenceMsg;
        }
    }
*/


static String nullProperty;
bool HostedObject::hasProperty(const String &propName) const {
    PropertyMap::const_iterator iter = mProperties.find(propName);
    return (iter != mProperties.end());
}
const String &HostedObject::getProperty(const String &propName) const {
    PropertyMap::const_iterator iter = mProperties.find(propName);
    if (iter != mProperties.end()) {
        return (*iter).second.mData;
    }
    return nullProperty;
}
String *HostedObject::propertyPtr(const String &propName, Duration ttl) {
    PropertyCacheValue &pcv = mProperties[propName];
    pcv.mTTL = ttl;
    return &(pcv.mData);
}
void HostedObject::setProperty(const String &propName, Duration ttl, const String &encodedValue) {
    PropertyMap::iterator iter = mProperties.find(propName);
    if (iter == mProperties.end()) {
        iter = mProperties.insert(PropertyMap::value_type(propName,
            PropertyCacheValue(encodedValue, ttl))).first;
    }
}
void HostedObject::unsetCachedPropertyAndSubscription(const String &propName) {
    PropertyMap::iterator iter = mProperties.find(propName);
    if (iter != mProperties.end()) {
        if (iter->second.hasSubscriptionID()) {
            SpaceDataMap::const_iterator spaceiter = mSpaceData->begin();
            for (;spaceiter != mSpaceData->end(); ++spaceiter) {
                int subID = iter->second.getSubscriptionID();
                Protocol::Broadcast subMsg;
                std::string subStr;
                subMsg.set_broadcast_name(subID);
                subMsg.SerializeToString(&subStr);
                RoutableMessageHeader header;
                header.set_destination_port(Services::BROADCAST);
                header.set_destination_space(spaceiter->first);
                header.set_destination_object(ObjectReference::spaceServiceID());
                sendViaSpace(header, MemoryReference(subStr.data(), subStr.length()));
            }
        }
        mProperties.erase(iter);
    }
}


static ProxyObjectPtr nullPtr;
const ProxyObjectPtr &HostedObject::getProxy(const SpaceID &space) const {
    SpaceDataMap::const_iterator iter = mSpaceData->find(space);
    if (iter == mSpaceData->end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}

void HostedObject::initializeDefault(
    const String&mesh,
    const LightInfo *lightInfo,
    const String&webViewURL,
    const Vector3f&meshScale,
    const PhysicalParameters&physicalParameters)
{
    Duration ttl = defaultTTL->as<Duration>();
    if (!mesh.empty()) {
        Protocol::StringProperty meshprop;
        meshprop.set_value(mesh);
        meshprop.SerializeToString(propertyPtr("MeshURI", ttl));
        Protocol::Vector3fProperty scaleprop;
        scaleprop.set_value(Vector3f(1,1,1)); // default value, set it manually if you want different.
        scaleprop.SerializeToString(propertyPtr("MeshScale", ttl));
        Protocol::PhysicalParameters physicalprop;
        physicalprop.set_mode(Protocol::PhysicalParameters::NONPHYSICAL);
        physicalprop.SerializeToString(propertyPtr("PhysicalParameters", ttl));
        if (!webViewURL.empty()) {
            Protocol::StringProperty meshprop;
            meshprop.set_value(webViewURL);
            meshprop.SerializeToString(propertyPtr("WebViewURL", ttl));
        }
    } else if (lightInfo) {
        Protocol::LightInfoProperty lightProp;
        lightInfo->toProtocol(lightProp);
        lightProp.SerializeToString(propertyPtr("LightInfo", ttl));
    } else {
        setProperty("IsCamera", ttl);
    }
    //connect(spaceID, spaceConnectionHint, startingLocation, meshBounds, getUUID());
}

/* NOTE: Removed to avoid mDefaultTracker and mSendService, but may want to be rescued
void HostedObject::initializeRestoreFromDatabase(const SpaceID& spaceID) {
    Persistence::SentReadWriteSet *msg;
    if (mDefaultTracker == NULL) {
        // FIXME this allocation is happening before a real connection to the
        // space.  Currently this ends up just using NULL form
        // default_tracker_port, which obviously won't work out in the long term.
        ODP::Port* default_tracker_port = NULL;
        try {
            default_tracker_port = bindODPPort(spaceID);
        } catch(ODP::PortAllocationException& e) {
        }
        mDefaultTracker = new QueryTracker(default_tracker_port, mObjectHost->getSpaceIO());
        mDefaultTracker->forwardMessagesTo(&mSendService);
    }
    msg = new Persistence::SentReadWriteSet(mDefaultTracker);
    msg->setPersistenceCallback(std::tr1::bind(
                         &PrivateCallbacks::initializeDatabaseCallback,
                         getWeakPtr(), spaceID,
                         _1, _2, _3));
    msg->body().add_reads().set_field_name("WebViewURL");
    msg->body().add_reads().set_field_name("MeshURI");
    msg->body().add_reads().set_field_name("MeshScale");
    msg->body().add_reads().set_field_name("Name");
    msg->body().add_reads().set_field_name("PhysicalParameters");
    msg->body().add_reads().set_field_name("LightInfo");
    msg->body().add_reads().set_field_name("IsCamera");
    msg->body().add_reads().set_field_name("Parent");
    msg->body().add_reads().set_field_name("Loc");
    msg->body().add_reads().set_field_name("_Script");
    msg->body().add_reads().set_field_name("_ScriptParams");
    for (int i = 0; i < msg->body().reads_size(); i++) {
        msg->body().reads(i).set_object_uuid(getUUID()); // database assumes uuid 0 if omitted
    }
    msg->header().set_destination_object(ObjectReference::spaceServiceID());
    msg->header().set_destination_port(Services::PERSISTENCE);
    msg->serializeSend();
}
*/
namespace {
bool myisalphanum(char c) {
    if (c>='a'&&c<='z') return true;
    if (c>='A'&&c<='Z') return true;
    if (c>='0'&&c<='9') return true;
    return false;
}
}
void HostedObject::initializeScript(const String& script, const ObjectScriptManager::Arguments &args) {
    assert(!mObjectScript); // Don't want to kill a live script!
    static ThreadIdCheck scriptId=ThreadId::registerThreadGroup(NULL);
    assertThreadGroup(scriptId);
    if (!ObjectScriptManagerFactory::getSingleton().hasConstructor(script)) {
        bool passed=true;
        for (std::string::const_iterator i=script.begin(),ie=script.end();i!=ie;++i) {
            if (!myisalphanum(*i)) {
                if (*i!='-'&&*i!='_') {
                    passed=false;
                }
            }
        }
        if (passed) {
            mObjectHost->getScriptPluginManager()->load(script);
        }
    }
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getConstructor(script)("");
    if (mgr) {
        mObjectScript = mgr->createObjectScript(this->getSharedPtr(), args);
    }
}
void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const UUID&object_uuid_evidence)
{
    if (spaceID == SpaceID::null())
        return;

    mObjectHost->connect(
        getSharedPtr(), spaceID,
        TimedMotionVector3f(Time::null(), MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
        TimedMotionQuaternion(Time::null(), MotionQuaternion( Quaternion::identity(), Quaternion::identity() )),
        meshBounds,
        mesh,
        SolidAngle(.00001f),
        mContext->mainStrand->wrap( std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3) ),
        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, spaceID)
    );

    if(mSpaceData->find(spaceID) == mSpaceData->end()) {
        mSpaceData->insert(
            SpaceDataMap::value_type( spaceID, PerSpaceData(this, spaceID) )
        );
    }
}

void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ServerID server) {
    if (server == NullServerID) {
        HO_LOG(warning,"Failed to connect object (internal:" << mInternalObjectReference.toString() << ") to space " << space);
        return;
    }

    // Create
    ProxyObjectPtr self_proxy = createProxy(SpaceObjectReference(space, obj), URI(), mIsCamera);

    // Use to initialize PerSpaceData
    SpaceDataMap::iterator psd_it = mSpaceData->find(space);
    PerSpaceData& psd = psd_it->second;
    initializePerSpaceData(psd, self_proxy);

    // Special case for camera
    ProxyCameraObjectPtr cam = std::tr1::dynamic_pointer_cast<ProxyCameraObject, ProxyObject>(self_proxy);
    if (cam)
        cam->attach(String(), 0, 0);
}

void HostedObject::handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server) {
    NOT_IMPLEMENTED(ho);
}

void HostedObject::handleStreamCreated(const SpaceID& space) {
    boost::shared_ptr<Stream<UUID> > sstStream = mObjectHost->getSpaceStream(space, getUUID());

    if (sstStream != boost::shared_ptr<Stream<UUID> >() ) {
        boost::shared_ptr<Connection<UUID> > sstConnection = sstStream->connection().lock();
        assert(sstConnection);

        sstConnection->registerReadDatagramCallback(OBJECT_PORT_LOCATION,
            std::tr1::bind(&HostedObject::handleLocationMessage, this, space, _1, _2)
        );
        sstConnection->registerReadDatagramCallback(OBJECT_PORT_PROXIMITY,
            std::tr1::bind(&HostedObject::handleProximityMessage, this, space, _1, _2)
        );

    }
}

void HostedObject::initializePerSpaceData(PerSpaceData& psd, ProxyObjectPtr selfproxy) {
    psd.initializeAs(selfproxy);
    psd.rpcPort->receive( std::tr1::bind(&HostedObject::handleRPCMessage, this, _1, _2), ODP::Port::OLD_HANDLER);
    //psd.persistencePort->receive( std::tr1::bind(&HostedObject::handlePersistenceMessage, this, _1, _2), ODP::Port::OLD_HANDLER );
}

void HostedObject::disconnectFromSpace(const SpaceID &spaceID) {
    SpaceDataMap::iterator where;
    where=mSpaceData->find(spaceID);
    if (where!=mSpaceData->end()) {
        where->second.destroy(getTracker(spaceID));
        mSpaceData->erase(where);
    } else {
        SILOG(cppoh,error,"Attempting to disconnect from space "<<spaceID<<" when not connected to it...");
    }
}

bool HostedObject::route(Sirikata::Protocol::Object::ObjectMessage* msg) {
    DEPRECATED(); // We need a SpaceID in here
    assert( mSpaceData->size() == 1);
    SpaceID space = mSpaceData->begin()->first;
    return mObjectHost->send(getSharedPtr(), space, msg->source_port(), msg->dest_object(), msg->dest_port(), msg->payload());
}

void HostedObject::receiveMessage(const SpaceID& space, const Protocol::Object::ObjectMessage* msg) {
    // Convert to ODP runtime format
    ODP::Endpoint src_ep(space, ObjectReference(msg->source_object()), msg->source_port());
    ODP::Endpoint dst_ep(space, ObjectReference(msg->dest_object()), msg->dest_port());

    // FIXME to transition to real ODP instead of ObjectMessageRouter +
    // ObjectMessageDispatcher, we need to allow the old route as well.  First
    // we check if we can use ObjectMessageDispatcher::dispatchMessage, and if
    // not, we allow it through to the long term solution, ODP::Service
    if (ObjectMessageDispatcher::dispatchMessage(*msg)) {
        // Successfully delivered using old method
        delete msg;
        return;
    }
    if (mDelegateODPService->deliver(src_ep, dst_ep, MemoryReference(msg->payload()))) {
        // if this was true, it got delivered
        delete msg;
    }
    else {
        SILOG(cppoh,debug,"[HO] Undelivered message from " << src_ep << " to " << dst_ep);
        delete msg;
    }
//    } else if (header.destination_port() == 0) {
//        DEPRECATED(HostedObject);
//        handleRPCMessage(header, bodyData);
//    } else {
//        if (mObjectScript)
//            mObjectScript->processMessage(header, bodyData);
}

void HostedObject::sendViaSpace(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    //DEPRECATED(HostedObject);
    ///// MessageService::processMessage
    assert(hdrOrig.has_destination_object());
    assert(hdrOrig.has_destination_space());
    SpaceDataMap::iterator where=mSpaceData->find(hdrOrig.destination_space());
    if (where!=mSpaceData->end()) {
        mObjectHost->send(getSharedPtr(), hdrOrig.destination_space(), hdrOrig.source_port(), hdrOrig.destination_object().getAsUUID(), hdrOrig.destination_port(), body);
    }
    assert(where!=mSpaceData->end());
}

void HostedObject::send(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    //DEPRECATED(HostedObject);
    assert(hdrOrig.has_destination_object());
    if (!hdrOrig.has_destination_space() || hdrOrig.destination_space() == SpaceID::null()) {
        DEPRECATED(HostedObject); // QueryTracker still causes this case
        RoutableMessageHeader hdr (hdrOrig);
        hdr.set_destination_space(SpaceID::null());
        hdr.set_source_object(ObjectReference(mInternalObjectReference));
        mObjectHost->processMessage(hdr, body);
        return;
    }
    sendViaSpace(hdrOrig, body);
}

void HostedObject::tick() {
    for (SpaceDataMap::iterator iter = mSpaceData->begin(); iter != mSpaceData->end(); ++iter) {
        // send update to LOC (2) service in the space, if necessary
        iter->second.updateLocation(this);
    }
}

void HostedObject::handleLocationMessage(const SpaceID& space, uint8* buffer, int len) {
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    bool parse_success = contents.ParseFromArray(buffer, len);
    assert(parse_success);

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        Sirikata::Protocol::TimedMotionVector update_loc = update.location();
        TimedMotionVector3f loc(update_loc.t(), MotionVector3f(update_loc.position(), update_loc.velocity()));

        CONTEXT_OHTRACE(objectLoc,
            getUUID(),
            update.object(),
            loc
        );

        ProxyManagerPtr proxy_manager = getProxyManager(space);
        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space, ObjectReference(update.object())));
        proxy_obj->setLocation(loc);
    }
}

void HostedObject::handleProximityMessage(const SpaceID& space, uint8* buffer, int len) {
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromArray(buffer, len);
    assert(parse_success);

    for(int32 idx = 0; idx < contents.addition_size(); idx++) {
        Sirikata::Protocol::Prox::ObjectAddition addition = contents.addition(idx);

        TimedMotionVector3f loc(addition.location().t(), MotionVector3f(addition.location().position(), addition.location().velocity()));

        SpaceObjectReference proximateID(space, ObjectReference(addition.object()));
        // FIXME use weak_ptr instead of raw
        URI meshuri;
        if (addition.has_mesh()) meshuri = URI(addition.mesh());
        ProxyObjectPtr proxy_obj = createProxy(proximateID, meshuri, false);

        HO_LOG(debug,"Proximity addition: " << addition.object().toString() << " - mesh: " << (addition.has_mesh() ? addition.mesh() : "")); // Remove when properly handled

        CONTEXT_OHTRACE(prox,
            getUUID(),
            addition.object(),
            true,
            loc
        );
    }

    for(int32 idx = 0; idx < contents.removal_size(); idx++) {
        Sirikata::Protocol::Prox::ObjectRemoval removal = contents.removal(idx);

        HO_LOG(debug,"Proximity removal."); // Remove when properly handled
        CONTEXT_OHTRACE(prox,
            getUUID(),
            removal.object(),
            false,
            TimedMotionVector3f()
        );
    }
}


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const URI& meshuri, bool is_camera) {
    ProxyManagerPtr proxy_manager = getProxyManager(objref.space());
    ProxyObjectPtr proxy_obj;
    if (is_camera) {
        proxy_obj = ProxyObjectPtr(new ProxyCameraObject(proxy_manager.get(), objref, getSharedPtr()));
        proxy_manager->createObject(proxy_obj, getTracker(objref.space()));
    }
    else {
        ProxyMeshObjectPtr mesh_proxy_obj(new ProxyMeshObject(proxy_manager.get(), objref, getSharedPtr()));
        proxy_obj = ProxyObjectPtr(mesh_proxy_obj);
        // The call to createObject must occur before trying to do any other
        // operations so that any listeners will be set up.
        proxy_manager->createObject(proxy_obj, getTracker(objref.space()));
        if (meshuri)
            mesh_proxy_obj->setMesh(meshuri);
    }
    return proxy_obj;
}

static int32 query_id = 0;
using Protocol::LocRequest;
void HostedObject::processRPC(const RoutableMessageHeader &msg, const std::string &name, MemoryReference args, String *response) {
    if (name == "CreateObject") {
                Protocol::CreateObject co;
                co.ParseFromArray(args.data(),args.size());
                PhysicalParameters phys;
                LightInfo light;
                LightInfo *pLight=NULL;
                UUID uuid;
                std::string weburl;
                std::string mesh;
                bool camera=false;
                if (!co.has_object_uuid()) {
                    uuid = UUID::random();
                } else {
                    uuid = co.object_uuid();
                }
                if (!co.has_scale()) {
                    co.set_scale(Vector3f(1,1,1));
                }
                if (co.has_weburl()) {
                    weburl = co.weburl();
                }
                if (co.has_mesh()) {
                    mesh = co.mesh();
                }
                if (co.has_physical()) {
                    //parsePhysicalParameters(phys, co.physical());
                }
                if (co.has_light_info()) {
                    pLight=&light;
                    light = LightInfo(co.light_info());
                }
                if (co.has_camera() && co.camera()) {
                    camera=true;
                }
                SILOG(cppoh,info,"Creating new object "<<ObjectReference(uuid));
                VWObjectPtr vwobj = HostedObject::construct<HostedObject>(mContext, mObjectHost, uuid, camera);
                std::tr1::shared_ptr<HostedObject>obj=std::tr1::static_pointer_cast<HostedObject>(vwobj);
                if (camera) {
                    obj->initializeDefault("",NULL,"",co.scale(),phys);
                } else {
                    obj->initializeDefault(mesh,pLight,weburl,co.scale(),phys);
                }
                for (int i = 0; i < co.space_properties_size(); ++i) {
                    //RoutableMessageHeader connMessage
                    //obj->processRoutableMessage(connMessageHeader, connMessageData);
                    Protocol::ConnectToSpace space = co.space_properties(i);
                    UUID evidence = space.has_object_uuid_evidence()
                         ? space.object_uuid_evidence()
                         : uuid;
                    SpaceID spaceid (space.has_space_id()?space.space_id():msg.destination_space().getObjectUUID());
                    if (!space.has_bounding_sphere()) {
                        space.set_bounding_sphere(PBJ::BoundingSphere3f(Vector3f(0,0,0),1));
                    }
                    if (space.has_requested_object_loc() && space.has_bounding_sphere()) {
                        BoundingSphere3f bs (space.bounding_sphere());
                        Location location(Vector3d::nil(),Quaternion::identity(),Vector3f::nil(),Vector3f(1,0,0),0);
                        const Protocol::ObjLoc &loc = space.requested_object_loc();
                        SILOG(cppoh,debug,"Creating object "<<ObjectReference(getUUID())
                                <<" at position "<<loc.position());
                        if (loc.has_position()) {
                            location.setPosition(loc.position());
                        }
                        if (loc.has_orientation()) {
                            location.setOrientation(loc.orientation());
                        }
                        if (loc.has_velocity()) {
                            location.setVelocity(loc.velocity());
                        }
                        if (loc.has_rotational_axis()) {
                            location.setAxisOfRotation(loc.rotational_axis());
                        }
                        if (loc.has_angular_speed()) {
                            location.setAngularSpeed(loc.angular_speed());
                        }
                        obj->connect(spaceid, location, bs, "", evidence);
                    }
                }
                if (co.has_script()) {
                    String script_type = co.script();
                    ObjectScriptManager::Arguments script_args;
                    if (co.has_script_args()) {
                        Protocol::StringMapProperty args_map = co.script_args();
                        assert(args_map.keys_size() == args_map.values_size());
                        for (int i = 0; i < args_map.keys_size(); ++i)
                            script_args[ args_map.keys(i) ] = args_map.values(i);
                    }
                    obj->initializeScript(script_type, script_args);
                }
                return;
    }
    if (name == "ConnectToSpace") {
        // Fixme: move connection logic here so it's possible to reply later on.
        return;
    }
    if (name == "DisconnectFromSpace") {
        Protocol::DisconnectFromSpace disCon;
        disCon.ParseFromArray(args.data(),args.size());
        this->disconnectFromSpace(SpaceID(disCon.space_id()));
        return;
    }
    if (name=="DestroyObject") {
        this->destroy();
        return;
    }
    std::ostringstream printstr;
    printstr<<"\t";
    ProxyObjectPtr thisObj = getProxy(msg.source_space());
    if (name == "LocRequest") {
        LocRequest query;
        printstr<<"LocRequest: ";
        query.ParseFromArray(args.data(), args.length());
        Protocol::ObjLoc loc;
        Time now = Time::now(getSpaceTimeOffset(msg.source_space()));
        if (thisObj) {
            Location globalLoc = thisObj->globalLocation(now);
            loc.set_timestamp(now);
            uint32 fields = 0;
            bool all_fields = true;
            if (query.has_requested_fields()) {
                fields = query.requested_fields();
                all_fields = false;
            }
            if (all_fields || (fields & LocRequest::POSITION))
                loc.set_position(globalLoc.getPosition());
            if (all_fields || (fields & LocRequest::ORIENTATION))
                loc.set_orientation(globalLoc.getOrientation());
            if (all_fields || (fields & LocRequest::VELOCITY))
                loc.set_velocity(globalLoc.getVelocity());
            if (all_fields || (fields & LocRequest::ROTATIONAL_AXIS))
                loc.set_rotational_axis(globalLoc.getAxisOfRotation());
            if (all_fields || (fields & LocRequest::ANGULAR_SPEED))
                loc.set_angular_speed(globalLoc.getAngularSpeed());
            if (response)
                loc.SerializeToString(response);
        } else {
            SILOG(objecthost, error, "LocRequest message not for any known object.");
        }
        return;             /// comment out if we want scripts to see these requests
    }
    else if (name == "SetLoc") {
        Protocol::ObjLoc setloc;
        printstr<<"Someone wants to set my position: ";
        setloc.ParseFromArray(args.data(), args.length());
        if (thisObj) {
            printstr<<setloc.position();
            //applyPositionUpdate(thisObj, setloc, false);
        }
    }
    else if (name == "AddObject") {
        Protocol::ObjLoc setloc;
        printstr<<"Someone wants to set my position: ";
        setloc.ParseFromArray(args.data(), args.length());
        if (thisObj) {
            printstr<<setloc.position();
            //applyPositionUpdate(thisObj, setloc, false);
        }
    }
    else if (name == "DelObj") {
        SpaceDataMap::iterator perSpaceIter = mSpaceData->find(msg.source_space());
        if (perSpaceIter == mSpaceData->end()) {
            SILOG(objecthost, error, "DelObj message not for any known space.");
            return;
        }
        // FIXME
        /*
        TopLevelSpaceConnection *proxyMgr =
            perSpaceIter->second.mSpaceConnection.getTopLevelStream().get();
        if (thisObj && proxyMgr) {
            proxyMgr->unregisterHostedObject(thisObj->getObjectReference().object());
        }
        */
    }
    else if (name == "RetObj") {
        SpaceID space = msg.source_space();
        SpaceDataMap::iterator perSpaceIter = mSpaceData->find(space);
        if (msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "RetObj message not coming from space: "<<msg.source_object());
            return;
        }
        if (perSpaceIter == mSpaceData->end()) {
            SILOG(objecthost, error, "RetObj message not for any known space.");
            return;
        }
        // getProxyManager() does not work because we have not yet created our
        // ProxyObject.
        /** FIXME
        TopLevelSpaceConnection *proxyMgr =
            perSpaceIter->second.mSpaceConnection.getTopLevelStream().get();

        Protocol::RetObj retObj;
        retObj.ParseFromArray(args.data(), args.length());
        if (retObj.has_object_reference() && retObj.has_location()) {
            SpaceObjectReference objectId(msg.source_space(), ObjectReference(retObj.object_reference()));
            perSpaceIter->second.setObject(objectId.object());
            ProxyObjectPtr proxyObj;
            if (hasProperty("IsCamera")) {
                printstr<<"RetObj: I am now a Camera known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyCameraObject(proxyMgr, objectId, this));
            } else if (hasProperty("LightInfo") && !hasProperty("MeshURI")) {
                printstr<<"RetObj. I am now a Light known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyLightObject(proxyMgr, objectId, this));
            } else if (hasProperty("MeshURI") && hasProperty("WebViewURL")){
                printstr<<"RetObj: I am now a WebView known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyWebViewObject(proxyMgr, objectId, this));
            } else {
                printstr<<"RetObj: I am now a Mesh known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, objectId, this));
            }
            proxyObj->setLocal(true);
            initializePerSpaceData(perSpaceIter->second, proxyObj);
            proxyMgr->registerHostedObject(objectId.object(), getSharedPtr());
            applyPositionUpdate(proxyObj, retObj.location(), true);
            perSpaceIter->second.locationWasReset(retObj.location().timestamp(), proxyObj->getLastLocation());
            if (proxyMgr) {
                proxyMgr->createObject(proxyObj, getTracker(space));
                ProxyCameraObject* cam = dynamic_cast<ProxyCameraObject*>(proxyObj.get());
                if (cam) {
                    //HACK: Because we have no method of scripting yet, we force
                    //   any local camera we create to attach for convenience.
                    cam->attach(String(), 0, 0);
                    uint32 my_query_id = query_id;
                    query_id++;
                    Protocol::NewProxQuery proxQuery;
                    proxQuery.set_query_id(my_query_id);
                    proxQuery.set_max_radius(1.0e+30f);
                    String proxQueryStr;
                    proxQuery.SerializeToString(&proxQueryStr);
                    RoutableMessageBody body;
                    body.add_message("NewProxQuery", proxQueryStr);
                    String bodyStr;
                    body.SerializeToString(&bodyStr);
                    RoutableMessageHeader proxHeader;
                    proxHeader.set_destination_port(Services::GEOM);
                    proxHeader.set_destination_object(ObjectReference::spaceServiceID());
                    proxHeader.set_destination_space(objectId.space());
                    send(proxHeader, MemoryReference(bodyStr));
                }
                for (PropertyMap::const_iterator iter = mProperties.begin();
                        iter != mProperties.end();
                        ++iter) {
                    receivedPropertyUpdate(proxyObj, iter->first, iter->second.mData);
                }
            }
        }
        */
    } else {
        printstr<<"Message to be handled in script: "<<name;
    }
    SILOG(cppoh,debug,printstr.str());
    if (mObjectScript) {
        MemoryBuffer returnCopy;
        mObjectScript->processRPC(msg, name, args, returnCopy);
        if (response) {
            response->reserve(returnCopy.size());
            std::copy(returnCopy.begin(), returnCopy.end(),
                      std::insert_iterator<std::string>(*response, response->begin()));
        }
    }
}
const Duration&HostedObject::getSpaceTimeOffset(const SpaceID&space) {
    static Duration nil(Duration::seconds(0));
    SpaceDataMap::iterator where=mSpaceData->find(space);
    if (where!=mSpaceData->end()) {
        SILOG(ho,warn,"Hit missing time sync in HostedObject.");
        //return where->second.mSpaceConnection.getTopLevelStream()->getServerTimeOffset();
    }
    return nil;
}

// Identification
SpaceObjectReference HostedObject::id(const SpaceID& space) const {
    SpaceDataMap::const_iterator it = mSpaceData->find(space);
    if (it == mSpaceData->end()) return SpaceObjectReference::null();
    return it->second.id();
}

// ODP::Service Interface
ODP::Port* HostedObject::bindODPPort(SpaceID space, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(space, port);
}

ODP::Port* HostedObject::bindODPPort(SpaceID space) {
    return mDelegateODPService->bindODPPort(space);
}

void HostedObject::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

void HostedObject::registerDefaultODPHandler(const ODP::OldMessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODP::DelegatePort* HostedObject::createDelegateODPPort(ODP::DelegateService* parentService, SpaceID space, ODP::PortID port) {
    assert(space != SpaceID::any());
    assert(space != SpaceID::null());

    SpaceDataMap::const_iterator space_data_it = mSpaceData->find(space);
    if (space_data_it == mSpaceData->end())
        throw ODP::PortAllocationException("HostedObject::createDelegateODPPort can't allocate port because the HostedObject is not connected to the specified space.");

    ObjectReference objid = space_data_it->second.object;
    ODP::Endpoint port_ep(space, objid, port);
    return new ODP::DelegatePort(
        mDelegateODPService,
        port_ep,
        std::tr1::bind(
            &HostedObject::delegateODPPortSend, this,
            port_ep, _1, _2
        )
    );
}

bool HostedObject::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload) {
    assert(source_ep.space() == dest_ep.space());
    return mObjectHost->send(getSharedPtr(), source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(), dest_ep.port(), payload);
}

// Movement Interface

void HostedObject::requestLocationUpdate(const SpaceID& space, const TimedMotionVector3f& loc) {
    sendLocUpdateRequest(space, &loc, NULL, NULL, NULL);
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const TimedMotionQuaternion& orient) {
    sendLocUpdateRequest(space, NULL, &orient, NULL, NULL);
}

void HostedObject::requestBoundsUpdate(const SpaceID& space, const BoundingSphere3f& bounds) {
    sendLocUpdateRequest(space, NULL, NULL, &bounds, NULL);
}

void HostedObject::requestMeshUpdate(const SpaceID& space, const String& mesh) {
    sendLocUpdateRequest(space, NULL, NULL, NULL, &mesh);
}

void HostedObject::sendLocUpdateRequest(const SpaceID& space, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh) {
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    if (loc != NULL) {
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t(loc->updateTime());
        requested_loc.set_position(loc->position());
        requested_loc.set_velocity(loc->velocity());
    }
    if (orient != NULL) {
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t(orient->updateTime());
        requested_orient.set_position(orient->position());
        requested_orient.set_velocity(orient->velocity());
    }
    if (bounds != NULL)
        loc_request.set_bounds(*bounds);
    if (mesh != NULL)
        loc_request.set_mesh(*mesh);

    std::string payload = serializePBJMessage(container);

    boost::shared_ptr<Stream<UUID> > spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != boost::shared_ptr<Stream<UUID> >()) {
        boost::shared_ptr<Connection<UUID> > conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
    }
}

}
