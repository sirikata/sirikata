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
#include "network/Stream.hpp"
#include "util/SpaceObjectReference.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "oh/ObjectHost.hpp"
#include "oh/ProxyMeshObject.hpp"
#include "oh/ProxyLightObject.hpp"
#include "oh/ProxyCameraObject.hpp"
#include "oh/LightInfo.hpp"

#include <util/KnownServices.hpp>

namespace Sirikata {

HostedObject::PerSpaceData::PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream) :mSpaceConnection(topLevel,stream) {}

HostedObject::HostedObject(ObjectHost*parent):mInternalObjectReference(UUID::null()) {
    mObjectHost=parent;
}
namespace {
    static void connectionEvent(const HostedObjectWPtr&thus,
                                const SpaceID&sid,
                                Network::Stream::ConnectionStatus ce,
                                const String&reason) {
        if (ce!=Network::Stream::Connected) {
            HostedObject::disconnectionEvent(thus,sid,reason);
        }
    }
}
HostedObject::PerSpaceData& HostedObject::cloneTopLevelStream(const SpaceID&sid,const std::tr1::shared_ptr<TopLevelSpaceConnection>&tls) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    SpaceDataMap::iterator iter = mSpaceData.insert(
        SpaceDataMap::value_type(
            sid,
            PerSpaceData(tls,
                         tls->topLevelStream()->clone(
                             std::tr1::bind(&connectionEvent,
                                            getWeakPtr(),
                                            sid,
                                            _1,
                                            _2),
                             std::tr1::bind(&HostedObject::receivedRoutableMessage,
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



using Sirikata::Protocol::NewObj;
using Sirikata::Protocol::IObjLoc;

void HostedObject::initializeConnect(
    const UUID &objectName, const Location&startingLocation,
    const String&mesh, const BoundingSphere3f&meshBounds,
    const LightInfo *lightInfo,
    const SpaceID&spaceID, const HostedObjectWPtr&spaceConnectionHint)
{
    mInternalObjectReference=objectName;

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
    loc.set_position(startingLocation.getPosition());
    loc.set_orientation(startingLocation.getOrientation());
    loc.set_velocity(startingLocation.getVelocity());
    loc.set_rotational_axis(startingLocation.getAxisOfRotation());
    loc.set_angular_speed(startingLocation.getAngularSpeed());
    std::string serializedNewObj;
    newObj.SerializeToString(&serializedNewObj);

    std::string serializedBody;
    RoutableMessageBody messageBody;
    messageBody.add_message_names("NewObj");
    messageBody.add_message_arguments(serializedNewObj);
    messageBody.SerializeToString(&serializedBody);

    if (!mesh.empty()) {
        Protocol::StringProperty meshprop;
        meshprop.set_value(mesh);
        meshprop.SerializeToString(propertyPtr("MeshURI"));
        Protocol::Vector3fProperty scaleprop;
        scaleprop.set_value(Vector3f(1,1,1)); // default value, set it manually if you want different.
        meshprop.SerializeToString(propertyPtr("MeshScale"));
        Protocol::PhysicalParameters physicalprop;
        physicalprop.set_mode(Protocol::PhysicalParameters::NONPHYSICAL);
        meshprop.SerializeToString(propertyPtr("PhysicalParameters"));
    } else if (lightInfo) {
        Protocol::LightInfoProperty lightProp;
        lightInfo->toProtocol(lightProp);
        lightProp.SerializeToString(propertyPtr("LightInfo"));
    } else {
        setProperty("IsCamera");
    }
    bool success = send(messageHeader, MemoryReference(serializedBody));
    assert(success); //conn should be connected
}

void HostedObject::initializeRestoreFromDatabase(const UUID &objectName) {
    initializeConnect(objectName, Location(), String(), BoundingSphere3f(), NULL, SpaceID::null(), HostedObjectWPtr());
}
void HostedObject::initializeScripted(const UUID&objectName, const String& script, const SpaceID&id,const HostedObjectWPtr&spaceConnectionHint) {
    mInternalObjectReference=objectName;
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
bool HostedObject::send(const RoutableMessageHeader &hdrOrig, const MemoryReference&body) {
    assert(hdrOrig.has_destination_object());
    assert(hdrOrig.has_destination_space());
    SpaceDataMap::iterator where=mSpaceData.find(hdrOrig.destination_space());
    if (where!=mSpaceData.end()) {
        RoutableMessageHeader hdr (hdrOrig);
        hdr.clear_destination_space();
        String serialized_header;
        hdr.SerializeToString(&serialized_header);
        where->second.mSpaceConnection.getStream()->send(MemoryReference(serialized_header),body, Network::ReliableOrdered);
        return true;
    }
    return false;
}

void HostedObject::receivedPositionUpdate(
    const ProxyPositionObjectPtr &proxy,
    const ObjLoc &objLoc,
    bool force_reset)
{
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
        proxy->resetPositionVelocity(objLoc.timestamp(), currentLoc);
    } else {
        proxy->setPositionVelocity(objLoc.timestamp(), currentLoc);
    }
}

void HostedObject::receivedPropertyUpdate(
    const ProxyPositionObjectPtr &proxy,
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

bool HostedObject::receivedProxObjectProperties(
    const SpaceObjectReference &proximateObjectId,
    int32 queryId,
    const std::vector<std::string> &propertyRequests,
    const ReadOnlyMessage &responseMessage)
{
    if (responseMessage.return_status() != ReadOnlyMessage::SUCCESS) {
        SILOG(cppoh,info,"FAILURE receiving prox object properties "<<proximateObjectId.object());
        return false;
    }
    SILOG(cppoh,info,"Received prox object properties "<<proximateObjectId.object());
    bool hasMesh=false;
    bool hasLight=false;
    bool isCamera=false;
    ObjLoc objLoc;
    ProxyPositionObjectPtr proxyObj;
    for (size_t i = 0; i < propertyRequests.size() && (int)i < responseMessage.message_arguments_size(); ++i) {
        Protocol::PropertyResponse resp;
        if (propertyRequests[i] == "LocRequest") {
            objLoc.ParseFromString(responseMessage.message_arguments(i));
            continue;
        }
        resp.ParseFromString(responseMessage.message_arguments(i));
        if (resp.has_error()) {
            continue;
        }
        if (propertyRequests[i] == "MeshURI") {
            hasMesh = true;
        }
        if (propertyRequests[i] == "LightInfo") {
            hasLight = true;
        }
        if (propertyRequests[i] == "IsCamera") {
            isCamera = true;
        }
    }
    ObjectHostProxyManager *proxyMgr = NULL;
    SpaceDataMap::const_iterator iter = mSpaceData.find(proximateObjectId.space());
    ObjectReference myObjectReference;
    if (iter != mSpaceData.end()) {
        proxyMgr = iter->second.mSpaceConnection.getTopLevelStream().get();
    }
    if (!proxyMgr) {
        return false;
    }
    if (isCamera) {
        SILOG(cppoh,info, "* I found a camera named " << proximateObjectId.object());
        proxyObj = ProxyPositionObjectPtr(new ProxyCameraObject(proxyMgr, proximateObjectId));
    } else if (hasLight && !hasMesh) {
        SILOG(cppoh,info, "* I found a light named " << proximateObjectId.object());
        proxyObj = ProxyPositionObjectPtr(new ProxyLightObject(proxyMgr, proximateObjectId));
    } else {
        SILOG(cppoh,info, "* I found a MESH named " << proximateObjectId.object());
        proxyObj = ProxyPositionObjectPtr(new ProxyMeshObject(proxyMgr, proximateObjectId));
    }
    receivedPositionUpdate(proxyObj, objLoc, true);
    proxyMgr->createObjectProximity(proxyObj, mInternalObjectReference, queryId);
    for (size_t i = 0; i < propertyRequests.size() && (int)i < responseMessage.message_arguments_size(); ++i) {
        if (propertyRequests[i] == "LocRequest") {
            // does not have error field.
            continue;
        }
        const std::string &args = responseMessage.message_arguments(i);
        Protocol::PropertyResponse resp;
        resp.ParseFromString(args);
        if (!resp.has_error()) {
            receivedPropertyUpdate(proxyObj, propertyRequests[i], args);
        }
    }
    return false;
}

static int32 query_id = 0;
using Protocol::LocRequest;
void HostedObject::processMessage(const ReceivedMessage &msg, std::string *response) {
    ObjectHostProxyManager *proxyMgr = NULL;
    ProxyPositionObjectPtr thisObj;
    std::ostringstream printstr;
    printstr<<"\t";
    if (msg.perSpaceData) {
        proxyMgr = msg.perSpaceData->mSpaceConnection.getTopLevelStream().get();
        thisObj = msg.perSpaceData->mProxyObject;
    }

    if (msg.name == "GetProp") {
        String propertyName;
        Protocol::GetProp getProp;
        getProp.ParseFromArray(msg.body.data(), msg.body.length());
        if (getProp.has_property_name()) {
            propertyName = getProp.property_name();
        }
        if (hasProperty(propertyName)) {
            if (getProp.check_exists()) {
                Protocol::PropertyResponse responseMsg;
                responseMsg.set_error(Protocol::PropertyResponse::EXISTS);
                responseMsg.SerializeToString(response);
            } else {
                *response = getProperty(propertyName);
            }
            printstr<<"GetProp: "<<propertyName<<" = "<<getProperty(propertyName);
        } else {
            printstr<<"GetProp: "<<propertyName<<" (404'ed)";
            Protocol::PropertyResponse responseMsg;
            responseMsg.set_error(Protocol::PropertyResponse::NOT_FOUND);
            responseMsg.SerializeToString(response);
        }
    }
    else if (msg.name == "LocRequest") {
        LocRequest query;
        printstr<<"LocRequest: Proxy exists = "<<(thisObj?true:false);
        query.ParseFromArray(msg.body.data(), msg.body.length());
        ObjLoc loc;
        Task::AbsTime now = Task::AbsTime::now();
        if (thisObj) {
            Location globalLoc = thisObj->globalLocation(now);
            loc.set_timestamp(now);
            uint32 fields = 0xffffffff;
            if (query.has_requested_fields()) {
                fields = query.requested_fields();
            }
            if (fields & LocRequest::POSITION)
                loc.set_position(globalLoc.getPosition());
            if (fields & LocRequest::ORIENTATION)
                loc.set_orientation(globalLoc.getOrientation());
            if (fields & LocRequest::VELOCITY)
                loc.set_velocity(globalLoc.getVelocity());
            if (fields & LocRequest::ROTATIONAL_AXIS)
                loc.set_rotational_axis(globalLoc.getAxisOfRotation());
            if (fields & LocRequest::ANGULAR_SPEED)
                loc.set_angular_speed(globalLoc.getAngularSpeed());
            loc.SerializeToString(response);
        } else {
            SILOG(objecthost, error, "LocRequest message not for any known object.");
            return;
        }
    }
    else if (msg.name == "RetObj") {
        if (!msg.perSpaceData) {
            SILOG(objecthost, error, "RetObj message not for any known space.");
            return;
        }
        Protocol::RetObj retObj;
        retObj.ParseFromArray(msg.body.data(), msg.body.length());
        if (retObj.has_object_reference() && retObj.has_location()) {
            SpaceObjectReference objectId(msg.sourceObject.space(), ObjectReference(retObj.object_reference()));
            ProxyPositionObjectPtr proxyObj;
            if (hasProperty("IsCamera")) {
                printstr<<"RetObj: I am now a Camera known as "<<objectId.object();
                proxyObj = ProxyPositionObjectPtr(new ProxyCameraObject(proxyMgr, objectId));
            } else if (hasProperty("LightInfo") && !hasProperty("MeshURI")) {
                printstr<<"RetObj. I am now a Light known as "<<objectId.object();
                proxyObj = ProxyPositionObjectPtr(new ProxyLightObject(proxyMgr, objectId));
            } else {
                printstr<<"RetObj: I am now a Mesh known as "<<objectId.object();
                proxyObj = ProxyPositionObjectPtr(new ProxyMeshObject(proxyMgr, objectId));
            }
            msg.perSpaceData->mProxyObject = proxyObj;
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
                    body.add_message_names("NewProxQuery");
                    body.add_message_arguments(proxQueryStr);
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
    else if (msg.name == "ProxCall") {
        if (!msg.perSpaceData) {
            SILOG(objecthost, error, "ProxCall message not for any known space.");
            return;
        }
        if (!proxyMgr) {
            SILOG(objecthost, error, "ProxCall message with null ProxyManager.");
            return;
        }
        Protocol::ProxCall proxCall;
        proxCall.ParseFromArray(msg.body.data(), msg.body.length());
        SpaceObjectReference proximateObjectId (msg.sourceObject.space(), ObjectReference(proxCall.proximate_object()));
        ProxyPositionObjectPtr proxyObj = std::tr1::static_pointer_cast<ProxyPositionObject>(
            proxyMgr->getProxyObject(proximateObjectId));
        switch (proxCall.proximity_event()) {
          case Protocol::ProxCall::EXITED_PROXIMITY:
            printstr<<"ProxCall EXITED "<<proximateObjectId.object();
            proxyMgr->destroyObjectProximity(proxyObj, mInternalObjectReference, proxCall.query_id());
            break;
          case Protocol::ProxCall::ENTERED_PROXIMITY:
            printstr<<"ProxCall ENTERED "<<proximateObjectId.object();
            if (!proxyObj) { // FIXME: We may get one of these for each prox query. Keep track of in-progress queries in ProxyManager.
                printstr<<" (Not yet known)";
                RoutableMessageBody body;
                std::string serializedGetProp;
                std::vector<std::string> propertyRequests;

                propertyRequests.push_back("LocRequest");
                {
                    LocRequest loc;
                    loc.SerializeToString(&serializedGetProp);
                    body.add_message_names("LocRequest");
                    body.add_message_arguments(serializedGetProp);
                }

                body.add_message_names("GetProp");
                Protocol::GetProp req;
                propertyRequests.push_back("MeshURI");
                propertyRequests.push_back("MeshScale");
                propertyRequests.push_back("PhysicalParameters");
                propertyRequests.push_back("LightInfo");
                propertyRequests.push_back("Parent");
                propertyRequests.push_back("IsCamera");
                propertyRequests.push_back("GroupName");
                for (size_t i = 1; i < propertyRequests.size(); ++i) {
                    req.set_property_name(propertyRequests[i]);
                    req.SerializeToString(&serializedGetProp);
                    body.add_message_arguments(serializedGetProp);
                }

                uint32 my_query_id = query_id;
                ++query_id;
                body.set_id(my_query_id);
                std::string serializedBody;
                body.SerializeToString(&serializedBody);
                RoutableMessageHeader header;
                header.set_destination_space(proximateObjectId.space());
                header.set_destination_object(ObjectReference(proxCall.proximate_object()));
                mQueries.insert(RunningQueryMap::value_type(
                                    my_query_id,
                                    std::tr1::bind(
                    &HostedObject::receivedProxObjectProperties,
                        this,
                        proximateObjectId,
                        proxCall.query_id(),
                        propertyRequests,
                        _1)));
                send(header, MemoryReference(serializedBody));
                // add timer to (&HostedObject::timedOut, weak_ptr(this), my_query_id)

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
        printstr<<"Unknown message name: "<<msg.name;
    }
    SILOG(cppoh,debug,printstr.str());
}

void HostedObject::receivedRoutableMessage(const HostedObjectWPtr&thus,const SpaceID&sid, const Network::Chunk&msgChunk) {
    HostedObjectPtr realThis (thus.lock());
    ReadOnlyMessage message;
    message.ParseFromArray(&msgChunk[0],msgChunk.size());
    if (!realThis) {
        SILOG(objecthost,error,"Received message for dead HostedObject. SpaceID = "<<sid<<"; DestObject = "<<ObjectReference(message.destination_object()));
        return;
    }

    std::string myself_name;
    ReceivedMessage msg (sid, ObjectReference(message.source_object()), MessagePort(message.source_port()), MessagePort(message.destination_port()));
    {
        std::ostringstream os;
        SpaceDataMap::iterator iter = realThis->mSpaceData.find(sid);
        if (iter != realThis->mSpaceData.end()) {
            msg.perSpaceData = &(iter->second);
        }
        if (msg.perSpaceData && msg.perSpaceData->mProxyObject) {
            os << msg.perSpaceData->mProxyObject->getObjectReference().object();
        } else {
            os << "[Temporary UUID " << realThis->mInternalObjectReference.toString() << "]";
        }
        myself_name = os.str();
    }
    {
        std::ostringstream os;
        os << "** Message from: " << msg.sourceObject.object() << " port " << msg.sourcePort << " to "<<myself_name<<" port " << msg.destinationPort;
        SILOG(cppoh,debug,os.str());
    }
    /// Handle Return values to queries we sent to someone:
    if (message.has_return_status()) {
        if (!message.has_id()) {
            return; // invalid!
        }
        // This is a response message;
        int64 id = message.id();
        RunningQueryMap::iterator iter = realThis->mQueries.find(id);
        if (iter != realThis->mQueries.end()) {
            if (!(iter->second)(message)) {
                // The map may have changed... so look it up again to be sure.
                iter = realThis->mQueries.find(id);
                if (iter != realThis->mQueries.end()) {
                    realThis->mQueries.erase(iter);
                }
            }
        }
    }

    /// Parse message_names and message_arguments.
    int numNames = message.message_names_size();
    int numBodies = message.message_arguments_size();
    RoutableMessageBody responseMessage;
    if (numNames <= 0) {
        // Invalid message!
        return;
    }
    if (numBodies < numNames) {
        numNames = numBodies; // ignore invalid names.
    }
    for (int i = 0; i < numBodies; ++i) {
        if (i < numNames) {
            msg.name = message.message_names(i);
        }
        msg.body = MemoryReference(message.message_arguments(i));

        if (message.has_id()) {
            std::string response;
            /// Pass response parameter if we expect a response.
            realThis->processMessage(msg, &response);
            responseMessage.add_message_arguments(response);
        } else {
            /// Return value not needed.
            realThis->processMessage(msg, NULL);
        }
    }

    if (message.has_id()) {
        /// Send return value--
        // All message_arguments should be filled for now, even if they are all empty.
        // Doing otherwise needs special cases...
        // FIXME: Create a wrapper for RoutableMessageBody so we can handle special cases.

        responseMessage.set_id(message.id());
        responseMessage.set_return_status(RoutableMessageBody::SUCCESS);
        RoutableMessageHeader responseHeader;
        if (message.has_source_object()) {
            responseHeader.set_destination_object(ObjectReference(message.source_object()));
        }
        if (message.has_source_port()) {
            responseHeader.set_destination_port(message.source_port());
        }
        responseHeader.set_destination_space(sid);

        std::string messageBodyStr;
        responseMessage.SerializeToString(&messageBodyStr);
        realThis->send(responseHeader, MemoryReference(messageBodyStr));
    }
}
void HostedObject::disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
    std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
    if (thus) {
        SpaceDataMap::iterator where=thus->mSpaceData.find(sid);
        if (where!=thus->mSpaceData.end()) {
            thus->mSpaceData.erase(where);//FIXME do we want to back this up to the database first?
        }
    }
}


void HostedObject::setScale(Vector3f vec) {
    Protocol::Vector3fProperty prop;
    prop.set_value(vec);
    prop.SerializeToString(propertyPtr("MeshScale"));
}

}
