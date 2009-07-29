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

#include <util/Platform.hpp>
#include <oh/Platform.hpp>
#include <ObjectHost_Sirikata.pbj.hpp>
#include "util/RoutableMessage.hpp"
#include "util/KnownServices.hpp"
#include "network/Stream.hpp"
#include "util/SpaceObjectReference.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "oh/SentMessage.hpp"
#include "oh/ObjectHost.hpp"
#include "oh/ProxyMeshObject.hpp"
#include "oh/ProxyLightObject.hpp"
#include "oh/ProxyCameraObject.hpp"
#include "oh/LightInfo.hpp"

#include <util/KnownServices.hpp>

namespace Sirikata {

typedef SentMessageBody<RoutableMessageBody> RPCMessage;

class HostedObject::PerSpaceData {
public:
    SpaceConnection mSpaceConnection;
    ProxyObjectPtr mProxyObject; /// 
    PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream);
};

HostedObject::PerSpaceData::PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream) :mSpaceConnection(topLevel,stream) {}


HostedObject::HostedObject(ObjectHost*parent)
    : mTracker(parent->getSpaceIO()),
      mInternalObjectReference(UUID::null()) {
    mObjectHost=parent;
    mSendService.ho = this;
    mReceiveService.ho = this;
    mTracker.forwardMessagesTo(&mSendService);
}

HostedObject::~HostedObject() {
    mObjectHost->unregisterHostedObject(mInternalObjectReference);
    mTracker.endForwardingMessagesTo(&mSendService);
}

struct HostedObject::PrivateCallbacks {

    static void receivedRoutableMessage(const HostedObjectWPtr&thus,const SpaceID&sid, const Network::Chunk&msgChunk) {
        HostedObjectPtr realThis (thus.lock());

        RoutableMessageHeader header;
        MemoryReference bodyData = header.ParseFromArray(&(msgChunk[0]),msgChunk.size());
        header.set_source_space(sid);
        {
            ProxyObjectPtr destinationObject = realThis->getProxy(header.source_space());
            if (destinationObject) {
                header.set_destination_object(destinationObject->getObjectReference().object());
            }
            if (!header.has_source_object()) {
                header.set_source_object(ObjectReference::spaceServiceID());
            }
        }

        if (!realThis) {
            SILOG(objecthost,error,"Received message for dead HostedObject. SpaceID = "<<sid<<"; DestObject = "<<header.destination_object());
            return;
        }

        realThis->processRoutableMessage(header, bodyData);
    }
    static void handlePersistenceMessage(HostedObject *realThis, const RoutableMessageHeader &header, MemoryReference bodyData) {
        SILOG(objecthost, info, "Received Persistence query!");
    }
    static void handleRPCMessage(HostedObject *realThis, const RoutableMessageHeader &header, MemoryReference bodyData) {
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

    static void receivedProxObjectProperties(
        const HostedObjectWPtr &weakThis,
        SentMessage* sentMessageBase,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData,
        int32 queryId)
    {
        RoutableMessage responseMessage(hdr, bodyData.data(), bodyData.length());
        std::auto_ptr<RPCMessage> sentMessage(static_cast<RPCMessage*>(sentMessageBase));
        HostedObjectPtr realThis(weakThis.lock());
        if (!realThis) {
            return;
        }

        SpaceObjectReference proximateObjectId(sentMessage->getSpace(), sentMessage->getRecipient());
        if (responseMessage.header().return_status() != RoutableMessageHeader::SUCCESS) {
            SILOG(cppoh,info,"FAILURE receiving prox object properties "<<proximateObjectId.object());
            return;
        }
        SILOG(cppoh,info,"Received prox object properties "<<proximateObjectId.object());
        bool hasMesh=false;
        bool hasLight=false;
        bool isCamera=false;
        ObjLoc objLoc;
        ProxyObjectPtr proxyObj;
        for (int i = 0; i < sentMessage->body().message_size() && i < responseMessage.body().message_size(); ++i) {
            if (sentMessage->body().message_names(i) == "LocRequest") {
                objLoc.ParseFromString(responseMessage.body().message_arguments(i));
            } else if (sentMessage->body().message_names(i) == "GetProp") {
                Protocol::PropertyUpdate propSent, prop;
                prop.ParseFromString(responseMessage.body().message_arguments(i));
                propSent.ParseFromString(sentMessage->body().message_arguments(i));
                prop.MergeFrom(propSent);
                if (prop.has_error()) {
                    continue;
                }
                if (prop.property_name() == "MeshURI") {
                    hasMesh = true;
                }
                if (prop.property_name() == "LightInfo") {
                    hasLight = true;
                }
                if (prop.property_name() == "IsCamera") {
                    isCamera = true;
                }
            }
        }
        ObjectHostProxyManager *proxyMgr = NULL;
        SpaceDataMap::const_iterator iter = realThis->mSpaceData.find(proximateObjectId.space());
        ObjectReference myObjectReference;
        if (iter != realThis->mSpaceData.end()) {
            proxyMgr = iter->second.mSpaceConnection.getTopLevelStream().get();
        }
        if (!proxyMgr) {
            return;
        }
        if (isCamera) {
            SILOG(cppoh,info, "* I found a camera named " << proximateObjectId.object());
            proxyObj = ProxyObjectPtr(new ProxyCameraObject(proxyMgr, proximateObjectId));
        } else if (hasLight && !hasMesh) {
            SILOG(cppoh,info, "* I found a light named " << proximateObjectId.object());
            proxyObj = ProxyObjectPtr(new ProxyLightObject(proxyMgr, proximateObjectId));
        } else {
            SILOG(cppoh,info, "* I found a MESH named " << proximateObjectId.object());
            proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, proximateObjectId));
        }
        realThis->receivedPositionUpdate(proxyObj, objLoc, true);
        proxyMgr->createObjectProximity(proxyObj, realThis->mInternalObjectReference, queryId);
        for (int i = 0; i < sentMessage->body().message_size() && i < responseMessage.body().message_size(); ++i) {
            if (sentMessage->body().message_names(i) == "GetProp") {
                Protocol::PropertyUpdate resp, sent;
                resp.ParseFromString(responseMessage.body().message_arguments(i));
                sent.ParseFromString(sentMessage->body().message_arguments(i));
                resp.MergeFrom(sent);
                if (!resp.has_error()) {
                    realThis->receivedPropertyUpdate(proxyObj, resp.property_name(), responseMessage.body().message_arguments(i));
                }
            }
        }
        {
            RPCMessage *request = new RPCMessage(&realThis->mTracker);
            request->header().set_destination_space(proximateObjectId.space());
            request->header().set_destination_object(proximateObjectId.object());
            Protocol::LocRequest loc;
            loc.SerializeToString(request->body().add_message("LocRequest"));
            request->setCallback(std::tr1::bind(&receivedPositionUpdateResponse, weakThis, _1, _2, _3));
            request->serializeSend();
        }
        return;
    }

    static void receivedPositionUpdateResponse(
        const HostedObjectWPtr &weakThus,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData)
    {
        RoutableMessage responseMessage(hdr, bodyData.data(), bodyData.length());
        HostedObjectPtr thus(weakThus.lock());
        if (!thus) {
            delete static_cast<RPCMessage*>(sentMessage);
        }
        if (responseMessage.header().return_status()) {
            return;
        }
        Protocol::ObjLoc loc;
        loc.ParseFromString(responseMessage.body().message_arguments(0));

        ProxyManager *pm = thus->getObjectHost()->getProxyManager(sentMessage->getSpace());
        if (pm) {
            ProxyObjectPtr obj(sentMessage->getRecipientProxy(pm));
            if (obj) {
                thus->receivedPositionUpdate(obj,loc,false);
            }
        }

        static_cast<RPCMessage*>(sentMessage)->serializeSend(); // Resend position update each time we get one.
    }

    static void disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
        std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
        if (thus) {
            SpaceDataMap::iterator where=thus->mSpaceData.find(sid);
            if (where!=thus->mSpaceData.end()) {
                thus->mSpaceData.erase(where);//FIXME do we want to back this up to the database first?
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

HostedObject::PerSpaceData& HostedObject::cloneTopLevelStream(const SpaceID&sid,const std::tr1::shared_ptr<TopLevelSpaceConnection>&tls) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    SpaceDataMap::iterator iter = mSpaceData.insert(
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
                                            _1))))).first;
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
    SpaceDataMap::const_iterator iter = mSpaceData.find(space);
    if (iter == mSpaceData.end()) {
        return nullPtr;
    }
    return iter->second.mProxyObject;
}


using Sirikata::Protocol::NewObj;
using Sirikata::Protocol::IObjLoc;

void HostedObject::initializeConnect(
    const UUID &objectName, const Location&startingLocation,
    const String&mesh, const BoundingSphere3f&meshBounds,
    const LightInfo *lightInfo,
    const SpaceID&spaceID, const HostedObjectWPtr&spaceConnectionHint)
{
    mInternalObjectReference=objectName;
    mObjectHost->registerHostedObject(getSharedPtr());

    std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection;

    HostedObjectPtr spaceConnectionHintPtr;
    if (spaceConnectionHintPtr = spaceConnectionHint.lock()) {
        SpaceDataMap::const_iterator iter = spaceConnectionHintPtr->mSpaceData.find(spaceID);
        if (iter != spaceConnectionHintPtr->mSpaceData.end()) {
            topLevelConnection = iter->second.mSpaceConnection.getTopLevelStream();
        }
    }
    if (!topLevelConnection) {
        topLevelConnection = mObjectHost->connectToSpace(spaceID);
    }
    PerSpaceData &psd = cloneTopLevelStream(spaceID,topLevelConnection);
    SpaceConnection &conn = psd.mSpaceConnection;
    //psd.mProxyObject = ProxyObjectPtr(new ProxyMeshObject(topLevelConnection.get(), SpaceObjectRefernce(spaceID, obj)
    RoutableMessageHeader messageHeader;
    messageHeader.set_destination_object(ObjectReference::spaceServiceID());
    messageHeader.set_destination_space(spaceID);
    messageHeader.set_destination_port(Services::REGISTRATION);
    NewObj newObj;
    newObj.set_object_uuid_evidence(objectName);
    newObj.set_bounding_sphere(meshBounds);
    IObjLoc loc = newObj.mutable_requested_object_loc();
    loc.set_timestamp(Time::now());
    loc.set_position(startingLocation.getPosition());
    loc.set_orientation(startingLocation.getOrientation());
    loc.set_velocity(startingLocation.getVelocity());
    loc.set_rotational_axis(startingLocation.getAxisOfRotation());
    loc.set_angular_speed(startingLocation.getAngularSpeed());

    RoutableMessageBody messageBody;
    newObj.SerializeToString(messageBody.add_message("NewObj"));

    std::string serializedBody;
    messageBody.SerializeToString(&serializedBody);

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
    } else if (lightInfo) {
        Protocol::LightInfoProperty lightProp;
        lightInfo->toProtocol(lightProp);
        lightProp.SerializeToString(propertyPtr("LightInfo"));
    } else {
        setProperty("IsCamera");
    }
    send(messageHeader, MemoryReference(serializedBody));
}

void HostedObject::initializeRestoreFromDatabase(const UUID &objectName) {
    initializeConnect(objectName, Location(), String(), BoundingSphere3f(), NULL, SpaceID::null(), HostedObjectWPtr());
}
void HostedObject::initializeScripted(const UUID&objectName, const String& script, const SpaceID&id,const HostedObjectWPtr&spaceConnectionHint) {
    mInternalObjectReference=objectName;
    mObjectHost->registerHostedObject(getSharedPtr());
    if (id!=SpaceID::null()) {
        //bind script to object...script might be a remote ID, so need to bind download target, etc
        std::tr1::shared_ptr<HostedObject> parentObject=spaceConnectionHint.lock();
        std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection;
        SpaceDataMap::iterator where;
        if (parentObject&&(where=parentObject->mSpaceData.find(id))!=mSpaceData.end()) {
            topLevelConnection=where->second.mSpaceConnection.getTopLevelStream();
        }else {
            topLevelConnection=mObjectHost->connectToSpace(id);
        }
        // sending initial packet is done by the script!
        //conn->send(initializationPacket,Network::ReliableOrdered);
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
        PrivateCallbacks::handleRPCMessage(this, header, bodyData);
    } else if (header.destination_port() == Services::PERSISTENCE) {
        PrivateCallbacks::handlePersistenceMessage(this, header, bodyData);
    } else {
        sendErrorReply(header, RoutableMessageHeader::PORT_FAILURE);
    }
}

void HostedObject::sendViaSpace(const RoutableMessageHeader &hdrOrig, MemoryReference body) {
    ///// MessageService::processMessage
    assert(hdrOrig.has_destination_object());
    assert(hdrOrig.has_destination_space());
    SpaceDataMap::iterator where=mSpaceData.find(hdrOrig.destination_space());
    if (where!=mSpaceData.end()) {
        RoutableMessageHeader hdr (hdrOrig);
        hdr.clear_destination_space();
        hdr.clear_source_space();
        hdr.clear_source_object();
        String serialized_header;
        hdr.SerializeToString(&serialized_header);
        where->second.mSpaceConnection.getStream()->send(MemoryReference(serialized_header),body, Network::ReliableOrdered);
    }
    assert(where!=mSpaceData.end());
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
    SpaceDataMap::iterator where=mSpaceData.find(hdrOrig.destination_space());
    if (where!=mSpaceData.end()) {
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

void HostedObject::receivedPositionUpdate(
    const ProxyObjectPtr &proxy,
    const ObjLoc &objLoc,
    bool force_reset)
{
    if (!proxy) {
        return;
    }
    force_reset = force_reset || (objLoc.update_flags() & ObjLoc::FORCE);
    if (!objLoc.has_timestamp()) {
        objLoc.set_timestamp(Task::AbsTime::now());
    }
    Location currentLoc = proxy->globalLocation(objLoc.timestamp());
    if (force_reset || objLoc.has_position()) {
        currentLoc.setPosition(objLoc.position());
    }
    if (force_reset || objLoc.has_orientation()) {
        currentLoc.setOrientation(objLoc.orientation());
    }
    if (force_reset || objLoc.has_velocity()) {
        currentLoc.setVelocity(objLoc.velocity());
    }
    if (force_reset || objLoc.has_rotational_axis()) {
        currentLoc.setAxisOfRotation(objLoc.rotational_axis());
    }
    if (force_reset || objLoc.has_angular_speed()) {
        currentLoc.setAngularSpeed(objLoc.angular_speed());
    }
    if (force_reset) {
        proxy->resetLocation(objLoc.timestamp(), currentLoc);
    } else {
        std::ostringstream os;
        os << "Received position update to "<<currentLoc;
        SILOG(cppoh,debug,os.str());
        proxy->setLocation(objLoc.timestamp(), currentLoc);
    }
}


static int32 query_id = 0;
using Protocol::LocRequest;
void HostedObject::processRPC(const RoutableMessageHeader &msg, const std::string &name, MemoryReference args, String *response) {
    std::ostringstream printstr;
    printstr<<"\t";
    ProxyObjectPtr thisObj = getProxy(msg.source_space());

    if (name == "GetProp") {
        String propertyName;
        Protocol::PropertyUpdate getProp;
        getProp.ParseFromArray(args.data(), args.length());
        if (getProp.has_property_name()) {
            propertyName = getProp.property_name();
        }
        if (hasProperty(propertyName)) {
            if (getProp.omit_value()) {
                Protocol::PropertyUpdate responseMsg;
                responseMsg.set_error(Protocol::PropertyUpdate::EXISTS);
                responseMsg.SerializeToString(response);
            } else {
                *response = getProperty(propertyName);
            }
            printstr<<"GetProp: "<<propertyName<<" = "<<getProperty(propertyName);
        } else {
            printstr<<"GetProp: "<<propertyName<<" (404'ed)";
            Protocol::PropertyUpdate responseMsg;
            responseMsg.set_error(Protocol::PropertyUpdate::NOT_FOUND);
            responseMsg.SerializeToString(response);
        }
    }
    else if (name == "LocRequest") {
        LocRequest query;
        printstr<<"LocRequest: ";
        query.ParseFromArray(args.data(), args.length());
        ObjLoc loc;
        Task::AbsTime now = Task::AbsTime::now();
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
            loc.SerializeToString(response);
        } else {
            SILOG(objecthost, error, "LocRequest message not for any known object.");
            return;
        }
    }
    else if (name == "DelObj") {
        SpaceDataMap::iterator perSpaceIter = mSpaceData.find(msg.source_space());
        if (perSpaceIter == mSpaceData.end()) {
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
        SpaceDataMap::iterator perSpaceIter = mSpaceData.find(msg.source_space());
        if (msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "RetObj message not coming from space: "<<msg.source_object());
            return;
        }
        if (perSpaceIter == mSpaceData.end()) {
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
            } else {
                printstr<<"RetObj: I am now a Mesh known as "<<objectId.object();
                proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, objectId));
            }
            perSpaceIter->second.mProxyObject = proxyObj;
            proxyMgr->registerHostedObject(objectId.object(), getSharedPtr());
            receivedPositionUpdate(proxyObj, retObj.location(), true);
            if (proxyMgr) {
                proxyMgr->createObject(proxyObj);
                ProxyCameraObject* cam = dynamic_cast<ProxyCameraObject*>(proxyObj.get());
                if (cam) {
                    /* HACK: Because we have no method of scripting yet, we force
                       any local camera we create to attach for convenience. */
                    cam->attach(String(), 0, 0);
                    uint32 my_query_id = query_id;
                    query_id++;
                    Protocol::NewProxQuery proxQuery;
                    proxQuery.set_query_id(my_query_id);
                    proxQuery.set_max_radius(100);
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
    }
    else if (name == "ProxCall") {
        ObjectHostProxyManager *proxyMgr;
        if (false && msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "ProxCall message not coming from space: "<<msg.source_object());
            return;
        }
        if (!thisObj) {
            SILOG(objecthost, error, "ProxCall message with null ProxyManager.");
            return;
        }

        proxyMgr = getProxyManager(msg.source_space());

        Protocol::ProxCall proxCall;
        proxCall.ParseFromArray(args.data(), args.length());
        SpaceObjectReference proximateObjectId (msg.source_space(), ObjectReference(proxCall.proximate_object()));
        ProxyObjectPtr proxyObj (proxyMgr->getProxyObject(proximateObjectId));
        switch (proxCall.proximity_event()) {
          case Protocol::ProxCall::EXITED_PROXIMITY:
            printstr<<"ProxCall EXITED "<<proximateObjectId.object();
            if (proxyObj) {
                proxyMgr->destroyObjectProximity(proxyObj, mInternalObjectReference, proxCall.query_id());
            } else {
                printstr<<" (unknown obj)";
            }
            break;
          case Protocol::ProxCall::ENTERED_PROXIMITY:
            printstr<<"ProxCall ENTERED "<<proximateObjectId.object();
            if (!proxyObj) { // FIXME: We may get one of these for each prox query. Keep track of in-progress queries in ProxyManager.
                printstr<<" (Requesting information...)";

                {
                    RPCMessage *request = new RPCMessage(&mTracker);
                    std::vector<std::string> propertyRequests;

                    request->header().set_destination_space(proximateObjectId.space());
                    request->header().set_destination_object(proximateObjectId.object());

                    LocRequest loc;
                    loc.SerializeToString(request->body().add_message("LocRequest"));
                    Protocol::PropertyUpdate req;
                    propertyRequests.push_back("MeshURI");
                    propertyRequests.push_back("MeshScale");
                    propertyRequests.push_back("PhysicalParameters");
                    propertyRequests.push_back("LightInfo");
                    propertyRequests.push_back("Parent");
                    propertyRequests.push_back("IsCamera");
                    propertyRequests.push_back("GroupName");
                    for (size_t i = 0; i < propertyRequests.size(); ++i) {
                        req.set_property_name(propertyRequests[i]);
                        req.SerializeToString(request->body().add_message("GetProp"));
                    }
                    request->setCallback(std::tr1::bind(&PrivateCallbacks::receivedProxObjectProperties,
                                                        getWeakPtr(), _1, _2, _3,
                                                        proxCall.query_id()));
                    request->setTimeout(5.0);
                    request->serializeSend();
                }
            } else {
                printstr<<" (Already known)";
                proxyMgr->createObjectProximity(proxyObj, mInternalObjectReference, proxCall.query_id());
            }
            break;
          case Protocol::ProxCall::STATELESS_PROXIMITY:
            printstr<<"ProxCall Stateless'ed "<<proximateObjectId.object();
            // Do not create a proxy object in this case: This message is for one-time queries
            break;
        }
    } else {
        printstr<<"Unknown message name: "<<name;
    }
    SILOG(cppoh,debug,printstr.str());
}

void HostedObject::receivedPropertyUpdate(
        const ProxyObjectPtr &proxy,
        const std::string &propertyName,
        const std::string &arguments)
{
    if (propertyName == "MeshURI") {
        Protocol::StringProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
            if (proxymesh) {
                SILOG(cppoh,info, "* Received MESH property update for " << proxy->getObjectReference().object() <<" has mesh URI "<<URI(parsedProperty.value()));
                proxymesh->setMesh(URI(parsedProperty.value()));
            }
        }
    }
    if (propertyName == "MeshScale") {
        Protocol::Vector3fProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
            if (proxymesh) {
                proxymesh->setScale(parsedProperty.value());
            }
        }
    }
    if (propertyName == "LightInfo") {
        Protocol::LightInfoProperty parsedLight;
        parsedLight.ParseFromString(arguments);
        ProxyLightObject *proxylight = dynamic_cast<ProxyLightObject*>(proxy.get());
        if (proxylight) {
            proxylight->update(LightInfo(parsedLight));
        }
    }
    if (propertyName == "PhysicalParameters") {
        Protocol::PhysicalParameters parsedProperty;
        parsedProperty.ParseFromString(arguments);
        ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
        if (proxymesh) {
            // FIXME: allow missing fields, and do not hardcode enum values.
            physicalParameters params;
            switch (parsedProperty.mode()) {
              case Protocol::PhysicalParameters::NONPHYSICAL:
                params.mode = 0;
                break;
              case Protocol::PhysicalParameters::STATIC:
                params.mode = 1;
                break;
              case Protocol::PhysicalParameters::DYNAMIC:
                params.mode = 2;
                break;
              default:
                params.mode = 0;
            }
            params.density = parsedProperty.density();
            params.friction = parsedProperty.friction();
            params.bounce = parsedProperty.bounce();
            proxymesh->setPhysical(params);
        }
    }
// Parent not supported yet -- may be merged into ObjLoc.
/*
  if (propertyName == "Parent") {
  Protocol::ParentProperty parsedProperty;
  parsedProperty.ParseFromString(arguments);
  if (parsedProperty.has_value()) {
  proxy->setParent(ObjectReference(parsedProperty.value()));
  }
  }
*/
// Generic properties not supported yet.
/*
  if (propertyName == "GroupName") {
  Protocol::StringProperty parsedProperty;
  parsedProperty.ParseFromString(arguments);
  if (parsedProperty.has_value()) {
  groupName = parsedProperty.value();
  }
  }
*/
}


void HostedObject::setScale(Vector3f vec) {
    Protocol::Vector3fProperty prop;
    prop.set_value(vec);
    prop.SerializeToString(propertyPtr("MeshScale"));
}

}
