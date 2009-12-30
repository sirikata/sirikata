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

#include <oh/Platform.hpp>
#include "proxyobject/ProxyMeshObject.hpp"
#include "proxyobject/ProxyLightObject.hpp"
#include "proxyobject/ProxyWebViewObject.hpp"
#include "proxyobject/ProxyCameraObject.hpp"
#include "proxyobject/LightInfo.hpp"
#include <ObjectHost_Sirikata.pbj.hpp>
#include <ObjectHost_Persistence.pbj.hpp>
#include <task/WorkQueue.hpp>
#include "util/RoutableMessage.hpp"
#include "util/KnownServices.hpp"
#include "persistence/PersistenceSentMessage.hpp"
#include "network/Stream.hpp"
#include "util/SpaceObjectReference.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "util/SentMessage.hpp"
#include "oh/ObjectHost.hpp"

#include "oh/ObjectScriptManager.hpp"
#include "oh/ObjectScript.hpp"
#include "oh/ObjectScriptManagerFactory.hpp"
#include <util/KnownServices.hpp>
#include "util/ThreadId.hpp"
#include "util/PluginManager.hpp"
namespace Sirikata {

typedef SentMessageBody<RoutableMessageBody> RPCMessage;


class HostedObject::PerSpaceData {
public:
    SpaceConnection mSpaceConnection;
    ProxyObjectPtr mProxyObject;
    ProxyObject::Extrapolator mUpdatedLocation;

    void locationWasReset(Time timestamp, Location loc) {
        loc.setVelocity(Vector3f::nil());
        loc.setAngularSpeed(0);
        mUpdatedLocation.resetValue(timestamp, loc);
    }
    void locationWasSet(const Protocol::ObjLoc &msg) {
        Time timestamp = msg.timestamp();
        Location loc = mUpdatedLocation.extrapolate(timestamp);
        ProxyObject::updateLocationWithObjLoc(loc, msg);
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

    PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream)
        :mSpaceConnection(topLevel,stream),
        mUpdatedLocation(
            Duration::seconds(.1),
            TemporalValue<Location>::Time::null(),
            Location(Vector3d(0,0,0),Quaternion(Quaternion::identity()),
                     Vector3f(0,0,0),Vector3f(0,1,0),0),
            ProxyObject::UpdateNeeded()) {
    }
};



HostedObject::HostedObject(ObjectHost*parent, const UUID &objectName)
    : VWObject(parent->getSpaceIO()),
      mInternalObjectReference(objectName) {
    mSpaceData = new SpaceDataMap;
    mObjectHost=parent;
    mObjectScript=NULL;
    mSendService.ho = this;
    mReceiveService.ho = this;
    mTracker.forwardMessagesTo(&mSendService);
}

HostedObject::~HostedObject() {
    if (mObjectScript) {
        delete mObjectScript;
    }
    for (SpaceDataMap::const_iterator iter = mSpaceData->begin();
         iter != mSpaceData->end();
         ++iter) {
        for (PerSpaceData::ProxQueryMap::const_iterator qiter = iter->second.mProxQueryMap.begin();
             qiter != iter->second.mProxQueryMap.end();
             ++qiter) {
            for (std::set<ObjectReference>::const_iterator oriter = qiter->second.begin();
                 oriter != qiter->second.end();
                 ++oriter)
            {
                iter->second.mSpaceConnection.getTopLevelStream()->
                    destroyObject(iter->second.mSpaceConnection.getTopLevelStream()->getProxyObject(SpaceObjectReference(iter->first, *oriter)), getTracker());
            }
        }
    }
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
    mTracker.endForwardingMessagesTo(&mSendService);
    delete mSpaceData;
}

struct HostedObject::PrivateCallbacks {

    static void initializeDatabaseCallback(
        HostedObject *realThis,
        const SpaceID &spaceID,
        Persistence::SentReadWriteSet *msg,
        const RoutableMessageHeader &lastHeader,
        Persistence::Protocol::Response::ReturnStatus errorCode)
    {
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
            if (!name.empty() && name[0] != '_') {
                realThis->setProperty(name, msg->body().reads(i).data());
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
        BoundingSphere3f sphere(Vector3f::nil(),1);
        realThis->sendNewObj(location, sphere, spaceID, realThis->getUUID());
        delete msg;
        if (!scriptName.empty()) {
            realThis->initializeScript(scriptName, scriptParams);
        }
        realThis->mObjectHost->dequeueAll();
    }

    static Network::Stream::ReceivedResponse receivedRoutableMessage(const HostedObjectWPtr&thus,const SpaceID&sid, const Network::Chunk&msgChunk) {
        HostedObjectPtr realThis (thus.lock());

        RoutableMessageHeader header;
        MemoryReference bodyData = header.ParseFromArray(&(msgChunk[0]),msgChunk.size());
        header.set_source_space(sid);
        header.set_destination_space(sid);

        if (!realThis) {
            SILOG(objecthost,error,"Received message for dead HostedObject. SpaceID = "<<sid<<"; DestObject = <deleted>");
            return Network::Stream::AcceptedData;
        }

        {
            ProxyObjectPtr destinationObject = realThis->getProxy(header.source_space());
            if (destinationObject) {
                header.set_destination_object(destinationObject->getObjectReference().object());
            }
            if (!header.has_source_object()) {
                header.set_source_object(ObjectReference::spaceServiceID());
            }
        }

        realThis->processRoutableMessage(header, bodyData);
        return Network::Stream::AcceptedData;
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
            realThis->sendReply(origHeader, MemoryReference(errorData));
        } else {
            realThis->sendReply(origHeader, bodyData);
        }
        realThis->mObjectHost->dequeueAll();
    }

    static void disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
        std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
        if (thus) {
            SpaceDataMap::iterator where=thus->mSpaceData->find(sid);
            if (where!=thus->mSpaceData->end()) {
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



void HostedObject::handleRPCMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
    HostedObject *realThis=this;
    /// Parse message_names and message_arguments.
    
    RoutableMessageBody msg;
    msg.ParseFromArray(bodyData.data(), bodyData.length());
    int numNames = msg.message_size();
    if (numNames <= 0) {
        // Invalid message!
        realThis->sendErrorReply(header, RoutableMessageHeader::PROTOCOL_ERROR);
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
        realThis->sendReply(header, MemoryReference(serializedResponse));
    }
}


void HostedObject::handlePersistenceMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
        using namespace Persistence::Protocol;
        HostedObject *realThis=this;
        ReadWriteSet rws;
        rws.ParseFromArray(bodyData.data(), bodyData.length());

        Response immedResponse;
        int immedIndex = 0;

        SentMessageBody<ReadWriteSet> *persistenceMsg = new SentMessageBody<ReadWriteSet>(&realThis->mTracker,std::tr1::bind(&PrivateCallbacks::handlePersistenceResponse, realThis, header, _1, _2, _3));
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
                    el.set_data(realThis->getProperty(name));
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
                    realThis->setProperty(name, rws.writes(i).data());
                    SpaceDataMap::iterator iter;
                    for (iter = realThis->mSpaceData->begin();
                         iter != realThis->mSpaceData->end();
                         ++iter) {
                        realThis->receivedPropertyUpdate(iter->second.mProxyObject, name, rws.writes(i).data());
                    }
                } else {
                    if (name != "LightInfo" && name != "MeshURI" && name != "IsCamera" && name != "WebViewURL") {
                        // changing the type of this object has to wait until we reload from database.
                        realThis->unsetProperty(name);
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
            RoutableMessageHeader respHeader (header);
            realThis->sendReply(respHeader, MemoryReference(respStr));
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
        realThis->mObjectHost->dequeueAll();
    }



HostedObject::PerSpaceData& HostedObject::cloneTopLevelStream(const SpaceID&sid,const std::tr1::shared_ptr<TopLevelSpaceConnection>&tls) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    SpaceDataMap::iterator iter = mSpaceData->insert(
        SpaceDataMap::value_type(
            sid,
            PerSpaceData(tls,
                         tls->topLevelStream()->clone(
                             std::tr1::bind(&PrivateCallbacks::connectionEvent,
                                            getWeakPtr(),
                                            sid,
                                            _1,
                                            _2),
                             std::tr1::bind(&PrivateCallbacks::receivedRoutableMessage,
                                            getWeakPtr(),
                                            sid,
                                            _1),
                             &Network::Stream::ignoreReadySendCallback)
            ))).first;
    return iter->second;
}

static String nullProperty;
bool HostedObject::hasProperty(const String &propName) const {
    PropertyMap::const_iterator iter = mProperties.find(propName);
    return (iter != mProperties.end());
}
const String &HostedObject::getProperty(const String &propName) const {
    PropertyMap::const_iterator iter = mProperties.find(propName);
    if (iter != mProperties.end()) {
        return (*iter).second;
    }
    return nullProperty;
}
String *HostedObject::propertyPtr(const String &propName) {
    return &(mProperties[propName]);
}
void HostedObject::setProperty(const String &propName, const String &encodedValue) {
    mProperties.insert(PropertyMap::value_type(propName, encodedValue));
}
void HostedObject::unsetProperty(const String &propName) {
    PropertyMap::iterator iter = mProperties.find(propName);
    if (iter != mProperties.end()) {
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


using Sirikata::Protocol::NewObj;
using Sirikata::Protocol::IObjLoc;

void HostedObject::sendNewObj(
    const Location&startingLocation,
    const BoundingSphere3f &meshBounds,
    const SpaceID&spaceID,
    const UUID&object_uuid_evidence)
{

    RoutableMessageHeader messageHeader;
    messageHeader.set_destination_object(ObjectReference::spaceServiceID());
    messageHeader.set_destination_space(spaceID);
    messageHeader.set_destination_port(Services::REGISTRATION);
    NewObj newObj;
    newObj.set_object_uuid_evidence(object_uuid_evidence);
    newObj.set_bounding_sphere(meshBounds);
    IObjLoc loc = newObj.mutable_requested_object_loc();
    loc.set_timestamp(Time::now(getSpaceTimeOffset(spaceID)));
    loc.set_position(startingLocation.getPosition());
    loc.set_orientation(startingLocation.getOrientation());
    loc.set_velocity(startingLocation.getVelocity());
    loc.set_rotational_axis(startingLocation.getAxisOfRotation());
    loc.set_angular_speed(startingLocation.getAngularSpeed());

    RoutableMessageBody messageBody;
    newObj.SerializeToString(messageBody.add_message("NewObj"));

    std::string serializedBody;
    messageBody.SerializeToString(&serializedBody);
    sendViaSpace(messageHeader, MemoryReference(serializedBody));
}

void HostedObject::initializeConnect(
    const String&mesh,
    const LightInfo *lightInfo,
    const String&webViewURL,
    const Vector3f&meshScale,
    const PhysicalParameters&physicalParameters)
{
    mObjectHost->registerHostedObject(getSharedPtr());
    if (!mesh.empty()) {
        Protocol::StringProperty meshprop;
        meshprop.set_value(mesh);
        meshprop.SerializeToString(propertyPtr("MeshURI"));
        Protocol::Vector3fProperty scaleprop;
        scaleprop.set_value(Vector3f(1,1,1)); // default value, set it manually if you want different.
        scaleprop.SerializeToString(propertyPtr("MeshScale"));
        Protocol::PhysicalParameters physicalprop;
        physicalprop.set_mode(Protocol::PhysicalParameters::NONPHYSICAL);
        physicalprop.SerializeToString(propertyPtr("PhysicalParameters"));
        if (!webViewURL.empty()) {
            Protocol::StringProperty meshprop;
            meshprop.set_value(webViewURL);
            meshprop.SerializeToString(propertyPtr("WebViewURL"));
        }
    } else if (lightInfo) {
        Protocol::LightInfoProperty lightProp;
        lightInfo->toProtocol(lightProp);
        lightProp.SerializeToString(propertyPtr("LightInfo"));
    } else {
        setProperty("IsCamera");
    }
    //connectToSpace(spaceID, spaceConnectionHint);
    //sendNewObj(startingLocation, meshBounds, spaceID);
    //mObjectHost->dequeueAll(); // don't need to wait until next frame.
}
void HostedObject::initializePythonScript() {
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getDefaultConstructor()("");
    if (mgr) {
        ObjectScriptManager::Arguments args;
        args["Assembly"]="Sirikata.Runtime";
        args["Class"]="PythonObject";
        args["Namespace"]="Sirikata.Runtime";
        args["PythonModule"]="test";
        args["PythonClass"]="exampleclass";

        mObjectScript=mgr->createObjectScript(this,args);
    }
}

void HostedObject::initializeRestoreFromDatabase(const SpaceID&spaceID, const HostedObjectPtr&spaceConnectionHint) {
    mObjectHost->registerHostedObject(getSharedPtr());
    connectToSpace(spaceID, spaceConnectionHint);

    Persistence::SentReadWriteSet *msg;
    msg = new Persistence::SentReadWriteSet(&mTracker);
    msg->setPersistenceCallback(std::tr1::bind(
                         &PrivateCallbacks::initializeDatabaseCallback,
                         this, spaceID,
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
    mObjectHost->dequeueAll(); // don't need to wait until next frame.
}
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
    mObjectHost->registerHostedObject(getSharedPtr());
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
            mObjectHost->getScriptPluginManager()->load(DynamicLibrary::filename(script));
        }
    }
    ObjectScriptManager *mgr = ObjectScriptManagerFactory::getSingleton().getConstructor(script)("");
    if (mgr) {
        mObjectScript = mgr->createObjectScript(this, args);
    }
}
void HostedObject::connectToSpace(const SpaceID&id,const HostedObjectPtr&spaceConnectionHint) {
    if (id!=SpaceID::null()) {
        //bind script to object...script might be a remote ID, so need to bind download target, etc
        std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection;
        SpaceDataMap::iterator where;
        if (spaceConnectionHint&&(where=spaceConnectionHint->mSpaceData->find(id))!=mSpaceData->end()) {
            topLevelConnection=where->second.mSpaceConnection.getTopLevelStream();
        }else {
            topLevelConnection=mObjectHost->connectToSpace(id);
        }

        // sending initial packet is done by the script!
        //conn->send(initializationPacket,Network::ReliableOrdered);
        PerSpaceData &psd = cloneTopLevelStream(id,topLevelConnection);
        // return &(psd.mSpaceConnection);
    }
}

void HostedObject::processRoutableMessage(const RoutableMessageHeader &header, MemoryReference bodyData) {
    std::string myself_name;
    {
        std::ostringstream os;
        if (header.has_destination_object()) {
            os << header.destination_object();
        } else {
            os << "[Temporary UUID " << mInternalObjectReference.toString() << "]";
        }
        myself_name = os.str();
    }
    {
        std::ostringstream os;
        os << "** Message from: " << header.source_object() << " port " << header.source_port() << " to "<<myself_name<<" port " << header.destination_port();
        SILOG(cppoh,debug,os.str());
    }
    /// Handle Return values to queries we sent to someone:
    if (header.has_reply_id()) {
        mTracker.processMessage(header, bodyData);
        return; // Not a message for us to process.
    }

    if (header.destination_port() == 0) {
        handleRPCMessage(header, bodyData);
    } else if (header.destination_port() == Services::PERSISTENCE) {
        handlePersistenceMessage(header, bodyData);
    } else {
        if (mObjectScript) {
            mObjectScript->processMessage(header, bodyData);
        } else {
            sendErrorReply(header, RoutableMessageHeader::PORT_FAILURE);
        }
    }
}

void HostedObject::sendViaSpace(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    ///// MessageService::processMessage
    assert(hdrOrig.has_destination_object());
    assert(hdrOrig.has_destination_space());
    SpaceDataMap::iterator where=mSpaceData->find(hdrOrig.destination_space());
    if (where!=mSpaceData->end()) {
        RoutableMessageHeader hdr (hdrOrig);
        hdr.clear_destination_space();
        hdr.clear_source_space();
        hdr.clear_source_object();
        String serialized_header;
        hdr.SerializeToString(&serialized_header);
        where->second.mSpaceConnection.getStream()->send(MemoryReference(serialized_header),body, Network::ReliableOrdered);
    }
    assert(where!=mSpaceData->end());
}

void HostedObject::send(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    assert(hdrOrig.has_destination_object());
    if (!hdrOrig.has_destination_space() || hdrOrig.destination_space() == SpaceID::null()) {
        RoutableMessageHeader hdr (hdrOrig);
        hdr.set_destination_space(SpaceID::null());
        hdr.set_source_object(ObjectReference(mInternalObjectReference));
        mObjectHost->processMessage(hdr, body);
        return;
    }
    SpaceDataMap::iterator where=mSpaceData->find(hdrOrig.destination_space());
    if (where!=mSpaceData->end()) {
        const ProxyObjectPtr &obj = where->second.mProxyObject;
        if (obj) {
            RoutableMessageHeader hdr (hdrOrig);
            hdr.set_source_object(obj->getObjectReference().object());
            mObjectHost->processMessage(hdr, body);
        } else {
            sendViaSpace(hdrOrig, body);
        }
    }
}

void HostedObject::tick() {
    for (SpaceDataMap::iterator iter = mSpaceData->begin(); iter != mSpaceData->end(); ++iter) {
        // send update to LOC (2) service in the space, if necessary
        iter->second.updateLocation(this);
    }
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
                    parsePhysicalParameters(phys, co.physical());
                }
                if (co.has_light_info()) {
                    pLight=&light;
                    light = LightInfo(co.light_info());
                }
                if (co.has_camera() && co.camera()) {
                    camera=true;
                }
                SILOG(cppoh,info,"Creating new object "<<ObjectReference(uuid));
                VWObjectPtr vwobj = HostedObject::construct<HostedObject>(mObjectHost, uuid);
                std::tr1::shared_ptr<HostedObject>obj=std::tr1::static_pointer_cast<HostedObject>(vwobj);
                if (camera) {
                    obj->initializeConnect("",NULL,"",co.scale(),phys);
                } else {
                    obj->initializeConnect(mesh,pLight,weburl,co.scale(),phys);
                }
                for (int i = 0; i < co.space_properties_size(); ++i) {
                    //RoutableMessageHeader connMessage
                    //obj->processRoutableMessage(connMessageHeader, connMessageData);
                    Protocol::ConnectToSpace space = co.space_properties(i);
                    UUID evidence = space.has_object_uuid_evidence()
                         ? space.object_uuid_evidence()
                         : uuid;
                    SpaceID spaceid (space.has_space_id()?space.space_id():msg.destination_space().getObjectUUID());
                    obj->connectToSpace(spaceid, getSharedPtr());
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
                        obj->sendNewObj(location, bs, spaceid, evidence);
                    }
                }
                mObjectHost->dequeueAll(); // don't need to wait until next frame.
                return;
    }
    if (name == "ConnectToSpace") {
        // Fixme: move connection logic here so it's possible to reply later on.
        return;
    }

    std::ostringstream printstr;
    printstr<<"\t";
    ProxyObjectPtr thisObj = getProxy(msg.source_space());
    VWObject::processRPC(msg,name,args,response);
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
            applyPositionUpdate(thisObj, setloc, false);
        }
    }
    else if (name == "AddObject") {
        Protocol::ObjLoc setloc;
        printstr<<"Someone wants to set my position: ";
        setloc.ParseFromArray(args.data(), args.length());
        if (thisObj) {
            printstr<<setloc.position();
            applyPositionUpdate(thisObj, setloc, false);
        }
    }
    else if (name == "DelObj") {
        SpaceDataMap::iterator perSpaceIter = mSpaceData->find(msg.source_space());
        if (perSpaceIter == mSpaceData->end()) {
            SILOG(objecthost, error, "DelObj message not for any known space.");
            return;
        }
        TopLevelSpaceConnection *proxyMgr =
            perSpaceIter->second.mSpaceConnection.getTopLevelStream().get();
        if (thisObj && proxyMgr) {
            proxyMgr->unregisterHostedObject(thisObj->getObjectReference().object());
        }
    }
    else if (name == "RetObj") {
        SpaceDataMap::iterator perSpaceIter = mSpaceData->find(msg.source_space());
        if (msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "RetObj message not coming from space: "<<msg.source_object());
            return;
        }
        if (perSpaceIter == mSpaceData->end()) {
            SILOG(objecthost, error, "RetObj message not for any known space.");
            return;
        }
        // getProxyManager() does not work because we have not yet created our ProxyObject.
        TopLevelSpaceConnection *proxyMgr =
            perSpaceIter->second.mSpaceConnection.getTopLevelStream().get();

        Protocol::RetObj retObj;
        retObj.ParseFromArray(args.data(), args.length());
        if (retObj.has_object_reference() && retObj.has_location()) {
            SpaceObjectReference objectId(msg.source_space(), ObjectReference(retObj.object_reference()));
            ProxyObjectPtr proxyObj;
            if (hasProperty("IsCamera")) {
                printstr<<"RetObj: I am now a Camera known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyCameraObject(proxyMgr, objectId));
            } else if (hasProperty("LightInfo") && !hasProperty("MeshURI")) {
                printstr<<"RetObj. I am now a Light known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyLightObject(proxyMgr, objectId));
            } else if (hasProperty("MeshURI") && hasProperty("WebViewURL")){
                printstr<<"RetObj: I am now a WebView known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyWebViewObject(proxyMgr, objectId));
            } else {
                printstr<<"RetObj: I am now a Mesh known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, objectId));
            }
            proxyObj->setLocal(true);
            perSpaceIter->second.mProxyObject = proxyObj;
            proxyMgr->registerHostedObject(objectId.object(), getSharedPtr());
            applyPositionUpdate(proxyObj, retObj.location(), true);
            perSpaceIter->second.locationWasReset(retObj.location().timestamp(), proxyObj->getLastLocation());
            if (proxyMgr) {
                proxyMgr->createObject(proxyObj, getTracker());
                ProxyCameraObject* cam = dynamic_cast<ProxyCameraObject*>(proxyObj.get());
                if (cam) {
                    /* HACK: Because we have no method of scripting yet, we force
                       any local camera we create to attach for convenience. */
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
                    receivedPropertyUpdate(proxyObj, iter->first, iter->second);
                }
            }
        }
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
    if (where!=mSpaceData->end())
        return where->second.mSpaceConnection.getTopLevelStream()->getServerTimeOffset();
    return nil;
}

ProxyManager* HostedObject::getProxyManager(const SpaceID&space) {
    SpaceDataMap::iterator iter = mSpaceData->find(space);
    if (iter == mSpaceData->end()) {
        return NULL;
    }
    return iter->second.mSpaceConnection.getTopLevelStream().get();
}
bool HostedObject::isLocal(const SpaceObjectReference&objref) const{
    SpaceDataMap::const_iterator iter = mSpaceData->find(objref.space());
    HostedObjectPtr destHostedObj;
    if (iter != mSpaceData->end()) {
        destHostedObj = iter->second.mSpaceConnection.getTopLevelStream()->getHostedObject(objref.object());
    }
    if (destHostedObj) {
        // This object is local to our object host--no need to query for its position.
        return true;
    }
    return false;
}


void HostedObject::removeQueryInterest(uint32 query_id, const ProxyObjectPtr&proxyObj, const SpaceObjectReference&proximateObjectId) {
    SpaceDataMap::iterator where = mSpaceData->find(proximateObjectId.space());
    if (where !=mSpaceData->end()) {
        ProxyManager* proxyMgr=where->second.mSpaceConnection.getTopLevelStream().get();
        PerSpaceData::ProxQueryMap::iterator iter = where->second.mProxQueryMap.find(query_id);
        if (iter != where->second.mProxQueryMap.end()) {
            std::set<ObjectReference>::iterator proxyiter = iter->second.find(proximateObjectId.object());
            assert (proxyiter != iter->second.end());
            if (proxyiter != iter->second.end()) {
                iter->second.erase(proxyiter);
                //FIXME slow: iterate through all outstanding queries for this object to see if others still refer to this
                bool otherCopies=false;
                for (PerSpaceData::ProxQueryMap::const_iterator i= where->second.mProxQueryMap.begin(),
                         ie=where->second.mProxQueryMap.end();
                     i!=ie;
                     ++i) {
                    if (i->second.find(proximateObjectId.object())!=i->second.end()) {
                        otherCopies=true;
                        break;
                    }
                }
                if (!otherCopies) {
                    proxyMgr->destroyObject(proxyObj, this->getTracker());
                }
            }
        }
    }
}

void HostedObject::addQueryInterest(uint32 query_id, const SpaceObjectReference&proximateObjectId) {
    SpaceDataMap::iterator where = mSpaceData->find(proximateObjectId.space());
    if (where !=mSpaceData->end()) {
        where->second.mProxQueryMap[query_id].insert(proximateObjectId.object());
    }
}



}
