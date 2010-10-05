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
#include <sirikata/core/util/KnownMessages.hpp>

#include <sirikata/core/util/ThreadId.hpp>
#include <sirikata/core/util/PluginManager.hpp>

#include <sirikata/core/odp/Exceptions.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>


#include <sirikata/core/network/Frame.hpp>
#include "Protocol_Frame.pbj.hpp"

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

            toSet.set_orientation(realLocation.getOrientation());
            toSet.set_rotational_axis(realLocation.getAxisOfRotation());
            toSet.set_angular_speed(realLocation.getAngularSpeed());

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
        rpcPort = parent->bindODPPort(id(), ODP::PortID((uint32)Services::RPC));
        persistencePort = parent->bindODPPort(id(), ODP::PortID((uint32)Services::PERSISTENCE));

        // Use any port for tracker
        tracker = new QueryTracker(parent->bindODPPort(id()), parent->mContext->ioService);
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
   mIsCamera(is_camera),
   mStartedTime(Time::local())
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


    mHasScript = false;
    mSSTDatagramLayer = BaseDatagramLayer<UUID>::createDatagramLayer(getUUID(), ctx,this, this);


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
        realThis->connect(spaceID, location, sphere, "", SolidAngle::Max,realThis->getUUID());
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
    if (it == mSpaceData->end())
    {
        
        it = mSpaceData->insert(
            SpaceDataMap::value_type( space, PerSpaceData(this, space) )
        ).first;
    }
    return it->second.proxyManager;
}



void HostedObject::getSpaceObjRefs(SpaceObjRefSet& ss) const
{
    if (mSpaceData == NULL)
    {
        std::cout<<"\n\n\nCalling getSpaceObjRefs when not connected to any spaces.  This really shouldn't happen\n\n\n";
        assert(false);
    }

    SpaceDataMap::const_iterator smapIter;
    for (smapIter = mSpaceData->begin(); smapIter != mSpaceData->end(); ++smapIter)
        ss.insert(SpaceObjectReference(smapIter->second.space,smapIter->second.object));
}





void HostedObject::handleRPCMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
    HostedObject *realThis=this;
    /// Parse message_names and message_arguments.

    // FIXME: Transitional. There are two ways this data could be
    // stored. The old way is directly in the bodyData.  The other is
    // in the payload section of bodyData.  Either way, we parse the
    // body, but in the former, we need to parse a *secondary*
    // RoutableMessageBody in the payload field.  Eventually this
    // should just be in the "header", i.e. the entire packet should
    // just be unified.
    RoutableMessageBody msg;
    RoutableMessageBody outer_msg;
    outer_msg.ParseFromArray(bodyData.data(), bodyData.length());
    if (outer_msg.has_payload()) {
        assert( outer_msg.message_size() == 0 );
        msg.ParseFromString(outer_msg.payload());
    }
    else {
        msg = outer_msg;
    }

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

ProxyObjectPtr HostedObject::getProxy(const SpaceID& space, const ObjectReference& oref)
{
    ProxyManagerPtr proxy_manager = getProxyManager(space);
    ProxyObjectPtr  proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));
    return proxy_obj;
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


// attaches a script to the entity. This is like running 
// the script after the entity is initialized
// the entity should have been intialized
void HostedObject::attachScript(const String& script_name)
{
  if(!mObjectScript)
  {
      SILOG(oh,warn,"[OH] Ignored attachScript because script is not initialized for " << getUUID().toString() << "(internal id)");
      return;
  }
  
  mObjectScript->attachScript(script_name);

}

void HostedObject::initializeScript(const String& script, const ObjectScriptManager::Arguments &args, const std::string& fileScriptToAttach) {
    if (mObjectScript) {
        SILOG(oh,warn,"[OH] Ignored initializeScript because script already exists for " << getUUID().toString() << "(internal id)");
        return;
    }

    SILOG(oh,debug,"[OH] Creating a script object for " << getUUID().toString() << "(internal id)");
    
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
        if (fileScriptToAttach != "")
        {
            std::cout<<"\n\nAttaching script: "<<fileScriptToAttach<<"\n\n";
            mObjectScript->attachScript(fileScriptToAttach);
        }
    }
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const UUID&object_uuid_evidence,
        const String& scriptFile,
        const String& scriptType)
{
    connect(spaceID, startingLocation, meshBounds, mesh, SolidAngle::Max, object_uuid_evidence);
}

void HostedObject::connect(
        const SpaceID&spaceID,
        const Location&startingLocation,
        const BoundingSphere3f &meshBounds,
        const String& mesh,
        const SolidAngle& queryAngle,
        const UUID&object_uuid_evidence,
        const String& scriptFile,
        const String& scriptType)
{
    if (spaceID == SpaceID::null())
    {
        return;
    }

    Time approx_server_time = getApproxServerTime();
    mObjectHost->connect(
        getSharedPtr(), spaceID,
        TimedMotionVector3f(approx_server_time, MotionVector3f( Vector3f(startingLocation.getPosition()), startingLocation.getVelocity()) ),
        TimedMotionQuaternion(approx_server_time,MotionQuaternion(startingLocation.getOrientation(),Quaternion(startingLocation.getAxisOfRotation(),startingLocation.getAngularSpeed()))),
        meshBounds,
        mesh,
        queryAngle,
        mContext->mainStrand->wrap( std::tr1::bind(&HostedObject::handleConnected, this, _1, _2, _3, approx_server_time, startingLocation,scriptFile,scriptType, meshBounds) ),

        std::tr1::bind(&HostedObject::handleMigrated, this, _1, _2, _3),
        std::tr1::bind(&HostedObject::handleStreamCreated, this, _1)
    );


    if(mSpaceData->find(spaceID) == mSpaceData->end()) {
        mSpaceData->insert(
            SpaceDataMap::value_type( spaceID, PerSpaceData(this, spaceID))
        );
    }
}



void HostedObject::handleConnected(const SpaceID& space, const ObjectReference& obj, ServerID server, const Time& start_time, const Location& startingLocation, const String& scriptFile, const String& scriptType, const BoundingSphere3f& bnds )
{
    if (server == NullServerID) {
        HO_LOG(warning,"Failed to connect object (internal:" << mInternalObjectReference.toString() << ") to space " << space);
        return;
    }

    // Create

    SpaceObjectReference self_objref(space, obj);
    // Convert back to local time
    Time local_start_time = convertToApproxLocalTime(start_time);
    ProxyObjectPtr self_proxy = createProxy(self_objref, self_objref, URI(), mIsCamera, local_start_time, startingLocation, bnds);


    // Use to initialize PerSpaceData
    SpaceDataMap::iterator psd_it = mSpaceData->find(space);
    PerSpaceData& psd = psd_it->second;
    initializePerSpaceData(psd, self_proxy);

    // Special case for camera
    ProxyCameraObjectPtr cam = std::tr1::dynamic_pointer_cast<ProxyCameraObject, ProxyObject>(self_proxy);
    if (cam)
        cam->attach(String(), 0, 0);

    //bind an odp port to listen for the begin scripting signal.  if have
    //receive the scripting signal for the first time, that means that we create
    //a JSObjectScript for this hostedobject
    bindODPPort(space,obj,Services::LISTEN_FOR_SCRIPT_BEGIN);

    //attach script callback;
    if (scriptFile != "")
    {
        ObjectScriptManager::Arguments script_args;  //these can just be empty;
        this->initializeScript(scriptType,script_args,scriptFile);
    }
    
}

void HostedObject::handleMigrated(const SpaceID& space, const ObjectReference& obj, ServerID server) {
    NOT_IMPLEMENTED(ho);
}

void HostedObject::handleStreamCreated(const SpaceObjectReference& spaceobj) {
    boost::shared_ptr<Stream<UUID> > sstStream = mObjectHost->getSpaceStream(spaceobj.space(), getUUID());

    if (sstStream != boost::shared_ptr<Stream<UUID> >() ) {
        sstStream->listenSubstream(OBJECT_PORT_LOCATION,
            std::tr1::bind(&HostedObject::handleLocationSubstream, this, spaceobj, _1, _2)
        );
        sstStream->listenSubstream(OBJECT_PORT_PROXIMITY,
            std::tr1::bind(&HostedObject::handleProximitySubstream, this, spaceobj, _1, _2)
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



//returns true if this is a script init message.  returns false otherwise
bool HostedObject::handleScriptInitMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    if (dst.port() != Services::LISTEN_FOR_SCRIPT_BEGIN)
        return false;

    //I don't really know what this segment of code does.  I copied it from processRPC
    RoutableMessageBody msg;
    RoutableMessageBody outer_msg;
    outer_msg.ParseFromArray(bodyData.data(), bodyData.length());
    if (outer_msg.has_payload())
    {
        assert( outer_msg.message_size() == 0 );
        msg.ParseFromString(outer_msg.payload());
    }
    else
        msg = outer_msg;


    int numNames = msg.message_size();
    if (numNames <= 0)
    {
        // Invalid message!
        //was a poorly formatted message to the listen_for_script_begin port.
        //send back a protocol error.
        RoutableMessageHeader replyHeader;
        replyHeader.set_return_status(RoutableMessageHeader::PROTOCOL_ERROR);
        sendViaSpace(replyHeader, MemoryReference::null());
        return true;
    }


    //if any of the names match, then we're going to go ahead an create a script
    //for it.
    for (int i = 0; i < numNames; ++i)
    {
        std::string name = msg.message_names(i);
        MemoryReference body(msg.message_arguments(i));
        
        //means that we are supposed to create a new scripted object
        if (name == KnownMessages::INIT_SCRIPT)
            processInitScriptMessage(body);
    }

    //it was on the script init port, so it was a scripting init message
    return true;
}


//The processInitScriptSetup takes in a message body that we know should be an
//init script message (from checks in handleScriptInitMessage).  
//Does some additional checking on the message body, and then sets a few global
//variables and calls the object's initializeScript function
void HostedObject::processInitScriptMessage(MemoryReference& body)
{
    Protocol::ScriptingInit si;
    si.ParseFromArray(body.data(),body.size());
    
    if (si.has_script())
    {
        String script_type = si.script();
        ObjectScriptManager::Arguments script_args;
        if (si.has_script_args())
        {
            
            Protocol::StringMapProperty args_map = si.script_args();
            assert(args_map.keys_size() == args_map.values_size());
            for (int i = 0; i < args_map.keys_size(); ++i)
                script_args[ args_map.keys(i) ] = args_map.values(i);
        }
        mHasScript = true;
        mScriptType = script_type;
        mScriptArgs = script_args;
        initializeScript(script_type, script_args);
    }
}


bool HostedObject::handleEntityCreateMessage(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference bodyData)
{
    //if the message isn't on the create_entity port, then it's good.
    //Otherwise, it's bad.
    if (dst.port() != Services::CREATE_ENTITY)
        return false;

    std::cout<<"\n\n\nI got a create entity message!!!\n\n";
    std::cout<<"\n\nIt's unsupported for now\n\n";

    return true;
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
    else if (handleScriptInitMessage(src_ep,dst_ep,MemoryReference(msg->payload())))
    {
        //if this was true, then that means that it was an init script command,
        //and we dealt with it.
        delete msg;
    }
    else if (handleEntityCreateMessage(src_ep,dst_ep,MemoryReference(msg->payload())))
    {
        //if this was true, then that means that we got a
        //handleEntityCreateMessage, and tried to create a new entity
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

void HostedObject::handleLocationSubstream(const SpaceObjectReference& spaceobj, int err, boost::shared_ptr< Stream<UUID> > s) {
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleLocationSubstreamRead, this, spaceobj, s, new std::stringstream(), _1, _2) );
}

void HostedObject::handleProximitySubstream(const SpaceObjectReference& spaceobj, int err, boost::shared_ptr< Stream<UUID> > s) {
    std::stringstream** prevdataptr = new std::stringstream*;
    *prevdataptr = new std::stringstream();
    s->registerReadCallback( std::tr1::bind(&HostedObject::handleProximitySubstreamRead, this, spaceobj, s, prevdataptr, _1, _2) );
}

void HostedObject::handleLocationSubstreamRead(const SpaceObjectReference& spaceobj, boost::shared_ptr< Stream<UUID> > s, std::stringstream* prevdata, uint8* buffer, int length) {
    prevdata->write((const char*)buffer, length);
    if (handleLocationMessage(spaceobj, prevdata->str())) {
        // FIXME we should be getting a callback on stream close instead of
        // relying on this parsing as an indicator
        delete prevdata;
        // Clear out callback so we aren't responsible for any remaining
        // references to s
        s->registerReadCallback(0);
    }
}

void HostedObject::handleProximitySubstreamRead(const SpaceObjectReference& spaceobj, boost::shared_ptr< Stream<UUID> > s, std::stringstream** prevdataptr, uint8* buffer, int length) {
    std::stringstream* prevdata = *prevdataptr;
    prevdata->write((const char*)buffer, length);

    while(true) {
        std::string msg = Network::Frame::parse(*prevdata);
        if (prevdata->eof()) {
            delete prevdata;
            prevdata = new std::stringstream();
            *prevdataptr = prevdata;
        }

        // If we don't have a full message, just wait for more
        if (msg.empty()) return;

        // Otherwise, try to handle it
        handleProximityMessage(spaceobj, msg);
    }

    // FIXME we should be getting a callback on stream close so we can clean up!
    //s->registerReadCallback(0);
}

bool HostedObject::handleLocationMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Frame frame;
    bool parse_success = frame.ParseFromString(payload);
    if (!parse_success) return false;
    Sirikata::Protocol::Loc::BulkLocationUpdate contents;
    contents.ParseFromString(frame.payload());

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Loc::LocationUpdate update = contents.update(idx);

        ProxyManagerPtr proxy_manager = getProxyManager(spaceobj.space());
        ProxyObjectPtr proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(spaceobj.space(), ObjectReference(update.object())));
        if (!proxy_obj) continue;

        if (update.has_location())
        {
            Sirikata::Protocol::TimedMotionVector update_loc = update.location();
            TimedMotionVector3f loc(convertToApproxLocalTime(update_loc.t()), MotionVector3f(update_loc.position(), update_loc.velocity()));
            proxy_obj->setLocation(loc);

            CONTEXT_OHTRACE(objectLoc,
                getUUID(),
                update.object(),
                loc
            );
        }

        if (update.has_orientation()) {
            Sirikata::Protocol::TimedMotionQuaternion update_orient = update.orientation();
            TimedMotionQuaternion orient(convertToApproxLocalTime(update_orient.t()), MotionQuaternion(update_orient.position(), update_orient.velocity()));
            proxy_obj->setOrientation(orient);
        }
    }

    return true;
}

bool HostedObject::handleProximityMessage(const SpaceObjectReference& spaceobj, const std::string& payload) {
    Sirikata::Protocol::Prox::ProximityResults contents;
    bool parse_success = contents.ParseFromString(payload);
    if (!parse_success) return false;

    for(int32 idx = 0; idx < contents.update_size(); idx++) {
        Sirikata::Protocol::Prox::ProximityUpdate update = contents.update(idx);

        for(int32 aidx = 0; aidx < update.addition_size(); aidx++) {
            Sirikata::Protocol::Prox::ObjectAddition addition = update.addition(aidx);
            SpaceObjectReference proximateID(spaceobj.space(), ObjectReference(addition.object()));
            TimedMotionVector3f loc(convertToApproxLocalTime(addition.location().t()), MotionVector3f(addition.location().position(), addition.location().velocity()));

            CONTEXT_OHTRACE(prox,
                getUUID(),
                addition.object(),
                true,
                loc
            );

            if (!getProxyManager(proximateID.space())->getProxyObject(proximateID)) {
                TimedMotionQuaternion orient(convertToApproxLocalTime(addition.orientation().t()), MotionQuaternion(addition.orientation().position(), addition.orientation().velocity()));

                URI meshuri;
                if (addition.has_mesh()) meshuri = URI(addition.mesh());

                // FIXME use weak_ptr instead of raw
                BoundingSphere3f bnds = addition.bounds();
                ProxyObjectPtr proxy_obj = createProxy(proximateID, spaceobj, meshuri, false, loc, orient, bnds);
            }
        }

        for(int32 ridx = 0; ridx < update.removal_size(); ridx++) {
            Sirikata::Protocol::Prox::ObjectRemoval removal = update.removal(ridx);

            HO_LOG(debug,"Proximity removal."); // Remove when properly handled
            CONTEXT_OHTRACE(prox,
                getUUID(),
                removal.object(),
                false,
                TimedMotionVector3f()
            );
        }
    }
    return true;
}


ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera, TimedMotionVector3f& tmv, TimedMotionQuaternion& tmq, const BoundingSphere3f& bs)
{
    ProxyObjectPtr returner = buildProxy(objref,owner_objref,meshuri,is_camera);
    returner->setLocation(tmv);
    returner->setOrientation(tmq);
    returner->setBounds(bs);

    if (!is_camera && meshuri) {
        ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(returner.get());
        if (mesh) mesh->setMesh(meshuri);
    }

    return returner;
}

ProxyObjectPtr HostedObject::createProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera, const Time& start_time, const Location& startingLoc, const BoundingSphere3f& bnds)
{
    ProxyObjectPtr returner = buildProxy(objref,owner_objref,meshuri,is_camera);

//takes care of starting location/veloc
    Vector3f startingLocPosVec (startingLoc.getPosition());
    Vector3f startingLocVelVec (startingLoc.getVelocity());
    // FIXME #116, #117 Time::local should have a real value from the server
    TimedMotionVector3f tmv(start_time, MotionVector3f(startingLocPosVec,startingLocVelVec));
    returner->setLocation(tmv);

//takes care of starting quaternion/rotation veloc
    Quaternion quaternionVeloc(startingLoc.getAxisOfRotation(), startingLoc.getAngularSpeed());
    MotionQuaternion initQuatVec (startingLoc.getOrientation(),quaternionVeloc);
    // FIXME #116, #117 Time::local should have a real value from the server
    TimedMotionQuaternion tmq (start_time,initQuatVec);

    if (!is_camera) returner->setOrientation(tmq);

    returner->setBounds(bnds);

    if (!is_camera && meshuri) {
        ProxyMeshObject *mesh = dynamic_cast<ProxyMeshObject*>(returner.get());
        if (mesh) mesh->setMesh(meshuri);
    }

    return returner;
}

//should only be called from within createProxy functions.  Otherwise, will not
//initilize position and quaternion correctly
ProxyObjectPtr HostedObject::buildProxy(const SpaceObjectReference& objref, const SpaceObjectReference& owner_objref, const URI& meshuri, bool is_camera)
{
    ProxyManagerPtr proxy_manager = getProxyManager(objref.space());
    ProxyObjectPtr proxy_obj;

    if (is_camera) proxy_obj = ProxyObject::construct<ProxyCameraObject>
                       (proxy_manager.get(), objref, getSharedPtr(), owner_objref);
    else proxy_obj = ProxyObject::construct<ProxyMeshObject>(proxy_manager.get(), objref, getSharedPtr(), owner_objref);

// The call to createObject must occur before trying to do any other
// operations so that any listeners will be set up.
    proxy_manager->createObject(proxy_obj, getTracker(objref.space()));
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
                
			// We check if the new entity is scripted. Also
			//note down the details of the script env given 
			//while creating this entity
			//These details are retrieved when the RetObj 
			//confirmation is sent by the space and we
			//initialize the script for the entity

                if (co.has_script()) {

                    obj->setHasScript(true); 
                    String script_type = co.script();

                    obj->setScriptType(script_type);

                    ObjectScriptManager::Arguments script_args;
                    if (co.has_script_args()) {
                        Protocol::StringMapProperty args_map = co.script_args();
                        assert(args_map.keys_size() == args_map.values_size());
                        for (int i = 0; i < args_map.keys_size(); ++i)
                            script_args[ args_map.keys(i) ] = args_map.values(i);
                    
					}

					obj->setScriptArgs(script_args);

                    if(co.has_script_name())
					{
					    obj->setScriptName(co.script_name());
					}

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
                                return;
    }
    if (name == "InitScript") {
        Protocol::ScriptingInit si;
        si.ParseFromArray(args.data(),args.size());

        if (si.has_script()) {
            String script_type = si.script();
            ObjectScriptManager::Arguments script_args;
            if (si.has_script_args()) {
                Protocol::StringMapProperty args_map = si.script_args();
                assert(args_map.keys_size() == args_map.values_size());
                for (int i = 0; i < args_map.keys_size(); ++i)
                    script_args[ args_map.keys(i) ] = args_map.values(i);
            }
			mHasScript = true;
			mScriptType = script_type;
			mScriptArgs = script_args;
            initializeScript(script_type, script_args);
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


            if(mHasScript)
			{
			  // do something to initialize the object script
			  initializeScript(mScriptType, mScriptArgs);
			  if(!mScriptName.empty())
			  {
			    attachScript(mScriptName);
			  }
		    } 	  
			//update all the entities on this object host
            mObjectHost->updateAddressable();  

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
        //bftm FIXME: uncomment the next line (but not the line below it).

        //SILOG(ho,warn,"Hit missing time sync in HostedObject.");
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
ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(space, objref, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor, ODP::PortID port) {
    return mDelegateODPService->bindODPPort(sor, port);
}

ODP::Port* HostedObject::bindODPPort(const SpaceID& space, const ObjectReference& objref) {
    return mDelegateODPService->bindODPPort(space, objref);
}

ODP::Port* HostedObject::bindODPPort(const SpaceObjectReference& sor) {
    return mDelegateODPService->bindODPPort(sor);
}

void HostedObject::registerDefaultODPHandler(const ODP::MessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

void HostedObject::registerDefaultODPHandler(const ODP::OldMessageHandler& cb) {
    mDelegateODPService->registerDefaultODPHandler(cb);
}

ODP::DelegatePort* HostedObject::createDelegateODPPort(ODP::DelegateService* parentService, const SpaceObjectReference& spaceobj, ODP::PortID port) {
    assert(spaceobj.space() != SpaceID::any());
    assert(spaceobj.space() != SpaceID::null());

    // FIXME We used to check if we had a presence in the space, but are now
    // allocating based on the full SpaceObjectReference (presence ID).  We
    // should verify we have this presence.
    //SpaceDataMap::const_iterator space_data_it = mSpaceData->find(space);
    //if (space_data_it == mSpaceData->end())
    //    throw ODP::PortAllocationException("HostedObject::createDelegateODPPort can't allocate port because the HostedObject is not connected to the specified space.");

    ODP::Endpoint port_ep(spaceobj, port);
    return new ODP::DelegatePort(
        mDelegateODPService,
        port_ep,
        std::tr1::bind(
            &HostedObject::delegateODPPortSend, this,
            port_ep, _1, _2
        )
    );
}

bool HostedObject::delegateODPPortSend(const ODP::Endpoint& source_ep, const ODP::Endpoint& dest_ep, MemoryReference payload)
{
    assert(source_ep.space() == dest_ep.space());
    return mObjectHost->send(getSharedPtr(), source_ep.space(), source_ep.port(), dest_ep.object().getAsUUID(), dest_ep.port(), payload);
}



void HostedObject::requestLocationUpdate(const SpaceID& space, const TimedMotionVector3f& loc)
{
    sendLocUpdateRequest(space, &loc, NULL, NULL, NULL);
}

//only update the position of the object, leave the velocity and orientation unaffected
void HostedObject::requestPositionUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& pos)
{
    Vector3f curVel = requestCurrentVelocity(space,oref);
    TimedMotionVector3f tmv (getApproxServerTime(),MotionVector3f(pos,curVel));
//FIXME: re-write the requestLocationUpdate function so that takes in object
//reference as well
    requestLocationUpdate(space,tmv);
}

//only update the velocity of the object, leave the position and the orientation
//unaffected
void HostedObject::requestVelocityUpdate(const SpaceID& space,  const ObjectReference& oref, const Vector3f& vel)
{
    Vector3f curPos = Vector3f(requestCurrentPosition(space,oref));
    TimedMotionVector3f tmv (getApproxServerTime(),MotionVector3f(curPos,vel));

    //FIXME: re-write the requestLocationUpdate function so that takes in object
    //reference as well

    requestLocationUpdate(space,tmv);
}

//send a request to update the orientation of this object
void HostedObject::requestOrientationDirectionUpdate(const SpaceID& space, const ObjectReference& oref,const Quaternion& quat)
{
    Quaternion curQuatVel = requestCurrentQuatVel(space,oref);
    TimedMotionQuaternion tmq (Time::local(),MotionQuaternion(quat,curQuatVel));
    requestOrientationUpdate(space, tmq);
}


Quaternion HostedObject::requestCurrentQuatVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getOrientationSpeed();
}


Quaternion HostedObject::requestCurrentOrientation(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    Location curLoc = proxy_obj->extrapolateLocation(Time::local());
    return curLoc.getOrientation();
    // return proxy_obj->getOrientation();
    // lkjs;
}

Quaternion HostedObject::requestCurrentOrientationVel(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return proxy_obj->getOrientationSpeed();
}

void HostedObject::requestOrientationVelocityUpdate(const SpaceID& space, const ObjectReference& oref, const Quaternion& quat)
{
    Quaternion curOrientQuat = requestCurrentOrientation(space,oref);
    TimedMotionQuaternion tmq (Time::local(),MotionQuaternion(curOrientQuat,quat));
    requestOrientationUpdate(space, tmq);
}



//goes into proxymanager and gets out the current location of the presence
//associated with
Vector3d HostedObject::requestCurrentPosition (const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj  = getProxy(space,oref);

    //BFTM_FIXME: need to decide whether want the extrapolated position or last
    //known position.  (Right now, we're going with last known position.)


    Location curLoc = proxy_obj->extrapolateLocation(Time::local());
    Vector3d currentPosition = curLoc.getPosition();
    return currentPosition;
}


bool HostedObject::requestMeshUri(const SpaceID& space, const ObjectReference& oref, Transfer::URI& tUri)
{
    
    ProxyManagerPtr proxy_manager = getProxyManager(space);
    ProxyObjectPtr  proxy_obj = proxy_manager->getProxyObject(SpaceObjectReference(space,oref));


    //this cast does not work.
    ProxyMeshObjectPtr proxy_mesh_obj = std::tr1::dynamic_pointer_cast<ProxyMeshObject,ProxyObject> (proxy_obj);

    
    if (proxy_mesh_obj )
        return false;

    tUri =  proxy_mesh_obj->getMesh();
    return true;
}



//FIXME: may need to do some checking to ensure that actually still connected to
//this space.
ObjectReference HostedObject::getObjReference(const SpaceID& space)
{
    SpaceDataMap::const_iterator space_data_it = mSpaceData->find(space);

    if (space_data_it == mSpaceData->end())
    {
        std::cout<<"\n\n";
        std::cout<<"Not connected to this space";
        std::cout<<"\n\n";
        std::cout.flush();
        assert(false);
    }
   
    return space_data_it->second.object;
}


Vector3f HostedObject::requestCurrentVelocity(const SpaceID& space, const ObjectReference& oref)
{
    ProxyObjectPtr proxy_obj = getProxy(space,oref);
    return (Vector3f)proxy_obj->getVelocity();
}

void HostedObject::requestOrientationUpdate(const SpaceID& space, const TimedMotionQuaternion& orient) {
    sendLocUpdateRequest(space, NULL, &orient, NULL, NULL);
}

void HostedObject::requestBoundsUpdate(const SpaceID& space, const BoundingSphere3f& bounds) {
    sendLocUpdateRequest(space, NULL, NULL, &bounds, NULL);
}

void HostedObject::requestScaleUpdate(const SpaceID& space, const ObjectReference& oref, const Vector3f& toScaleTo)
{
    std::cout<<"\n\nThe requestScaleUpdate function does not work\n\n";
    assert(false);
}

bool HostedObject::requestCurrentScale(const SpaceID& space, const ObjectReference& oref, Vector3f& scaler)
{
    std::cout<<"\n\nThe requestCurrentScale function does not work\n\n";
    assert(false);
    return false;
}


void HostedObject::requestMeshUpdate(const SpaceID& space, const String& mesh)
{
    sendLocUpdateRequest(space, NULL, NULL, NULL, &mesh);
}

void HostedObject::sendLocUpdateRequest(const SpaceID& space, const TimedMotionVector3f* const loc, const TimedMotionQuaternion* const orient, const BoundingSphere3f* const bounds, const String* const mesh) {
    // Generate and send an update to Loc
    Protocol::Loc::Container container;
    Protocol::Loc::ILocationUpdateRequest loc_request = container.mutable_update_request();
    if (loc != NULL) {
        Protocol::ITimedMotionVector requested_loc = loc_request.mutable_location();
        requested_loc.set_t( convertToApproxServerTime(loc->updateTime()) );
        requested_loc.set_position(loc->position());
        requested_loc.set_velocity(loc->velocity());
    }
    if (orient != NULL) {
        Protocol::ITimedMotionQuaternion requested_orient = loc_request.mutable_orientation();
        requested_orient.set_t( convertToApproxServerTime(orient->updateTime()) );
        requested_orient.set_position(orient->position());
        requested_orient.set_velocity(orient->velocity());
    }
    if (bounds != NULL)
        loc_request.set_bounds(*bounds);
    if (mesh != NULL)
    {
        loc_request.set_mesh(*mesh);
    }

    std::string payload = serializePBJMessage(container);

    boost::shared_ptr<Stream<UUID> > spaceStream = mObjectHost->getSpaceStream(space, getUUID());
    if (spaceStream != boost::shared_ptr<Stream<UUID> >()) {
        boost::shared_ptr<Connection<UUID> > conn = spaceStream->connection().lock();
        assert(conn);

        conn->datagram( (void*)payload.data(), payload.size(), OBJECT_PORT_LOCATION,
            OBJECT_PORT_LOCATION, NULL);
    }
}


Location HostedObject::getLocation(const SpaceID& space)
{
    ProxyObjectPtr proxy = getProxy(space);
    assert(proxy);
    Time tnow = proxy->getProxyManager()->getTimeOffsetManager()->now(*proxy);
    Location currentLoc = proxy->globalLocation(tnow);
    return currentLoc;
}

//BFTM_FIXME: need to actually write this function (called by ObjectHost's updateAddressable).
void HostedObject::updateAddressable()
{
    std::cout<<"\n\n\n";
    std::cout<<"BFTM: need to actually write this function";
    std::cout<<"\n\n\n";
    assert(false);
}


}
