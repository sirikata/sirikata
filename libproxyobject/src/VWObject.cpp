/*  Sirikata
 *  VWObject.cpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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

#include <sirikata/proxyobject/Platform.hpp>
#include "Protocol_Sirikata.pbj.hpp"
#include "Protocol_Persistence.pbj.hpp"
#include <sirikata/core/util/QueryTracker.hpp>
#include <sirikata/core/util/RoutableMessage.hpp>
#include <sirikata/core/util/SentMessage.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/persistence/PersistenceSentMessage.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include <sirikata/proxyobject/TimeOffsetManager.hpp>
#include <sirikata/proxyobject/ProxyLightObject.hpp>
#include <sirikata/proxyobject/ProxyCameraObject.hpp>
#include <sirikata/proxyobject/ProxyWebViewObject.hpp>
#include <sirikata/proxyobject/ProxyMeshObject.hpp>
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/VWObject.hpp>

namespace Sirikata {
using Protocol::ObjLoc;


typedef SentMessageBody<RoutableMessageBody> RPCMessage;

void VWObject::receivedProxObjectLocation(
    const VWObjectWPtr &weakThis,
    SentMessage* sentMessage,
    const RoutableMessageHeader &hdr,
    MemoryReference bodyData,
    int32 queryId) {

    SpaceID space = hdr.destination_space();

        std::auto_ptr<RPCMessage> destructor(static_cast<RPCMessage*>(sentMessage));
        VWObjectPtr realThis(weakThis.lock());
        if (!realThis) {
            return;
        }

        RoutableMessage responseMessage(hdr, bodyData.data(), bodyData.length());
        if (responseMessage.header().return_status() != RoutableMessageHeader::SUCCESS) {
            return;
        }
        ObjLoc objLoc;
        objLoc.ParseFromString(responseMessage.body().message_arguments(0));

        Persistence::SentReadWriteSet *request = new Persistence::SentReadWriteSet(realThis->getTracker(space));

        request->header().set_destination_space(sentMessage->getSpace());
        request->header().set_destination_object(sentMessage->getRecipient());
        request->header().set_destination_port(Services::PERSISTENCE);

        request->body().add_reads().set_field_name("MeshURI");
        request->body().add_reads().set_field_name("WebViewURL");
        request->body().add_reads().set_field_name("MeshScale");
        request->body().add_reads().set_field_name("Name");
        request->body().add_reads().set_field_name("PhysicalParameters");
        request->body().add_reads().set_field_name("LightInfo");
        request->body().add_reads().set_field_name("_Passwd");
        request->body().add_reads().set_field_name("IsCamera");
        request->body().add_reads().set_field_name("Parent");
        request->setPersistenceCallback(std::tr1::bind(&VWObject::receivedProxObjectProperties,
                                                       weakThis, _1, _2, _3,
                                                       queryId, objLoc));
        request->setTimeout(Duration::seconds(5.0));
        request->serializeSend();
}

VWObject::VWObject() {

}
VWObject::~VWObject(){}
void VWObject::receivedProxObjectProperties(
        const VWObjectWPtr &weakThis,
        SentMessage* sentMessageBase,
        const RoutableMessageHeader &hdr,
        uint64 returnStatus,//type Persistence::Protocol::Response::ReturnStatus
        int32 queryId,
        const ObjLoc &objLoc) {

    SpaceID space = hdr.destination_space();

    using namespace Persistence::Protocol;
    std::auto_ptr<Persistence::SentReadWriteSet> sentMessage(Persistence::SentReadWriteSet::cast_sent_message(sentMessageBase));
    VWObjectPtr realThis(weakThis.lock());
    if (!realThis) {
        return;
    }
    ProxyManager *proxyMgr = realThis->getProxyManager(sentMessage->getSpace());
    if (!proxyMgr) {
        return;
    }
    SpaceObjectReference proximateObjectId(sentMessage->getSpace(), sentMessage->getRecipient());
    bool persistence_error = false;
    if (hdr.return_status() != RoutableMessageHeader::SUCCESS ||returnStatus!=Persistence::Protocol::Response::SUCCESS) {
        SILOG(cppoh,info,"FAILURE receiving prox object properties "<<
              proximateObjectId.object()<<": Error = "<<(int)hdr.return_status());
        persistence_error = true;
    }
    SILOG(cppoh,debug,"Received prox object properties "<<proximateObjectId.object()<<": reads size = ?");
    int index = 0;
    bool haveAll = true;
    for (int i = 0; i < sentMessage->body().reads_size(); i++) {
        if (!sentMessage->body().reads(i).has_data() && !sentMessage->body().reads(i).has_return_status()) {
            haveAll = false;
        }
    }
    if (persistence_error || haveAll) {

    } else {
        assert(false&&"Received incomplete persistence callback for the same read/write set: should only ever get one");
        return; // More messages would have come before the new design.
    }
    bool hasMesh=false;
    bool hasLight=false;
    bool isCamera=false;
    bool isWebView=false;
    ProxyObjectPtr proxyObj;
    for (int i = 0; i < sentMessage->body().reads_size(); ++i) {
        if (sentMessage->body().reads(i).has_return_status()) {
            continue;
        }
        const std::string &field = sentMessage->body().reads(i).field_name();
        if (field == "MeshURI") {
            hasMesh = true;
        }
        if (field == "LightInfo") {
            hasLight = true;
        }
        if (field == "IsCamera") {
            isCamera = true;
        }
        if (field == "WebViewURL") {
            isWebView = true;
        }
    }
    ObjectReference myObjectReference;
    if (isCamera) {
        SILOG(cppoh,info, "* I found a camera named " << proximateObjectId.object());
        proxyObj = ProxyObjectPtr(new ProxyCameraObject(proxyMgr, proximateObjectId, (ODP::Service*)realThis.get()));
    } else if (hasLight && !hasMesh) {
        SILOG(cppoh,info, "* I found a light named " << proximateObjectId.object());
        proxyObj = ProxyObjectPtr(new ProxyLightObject(proxyMgr, proximateObjectId, (ODP::Service*)realThis.get()));
    } else if (hasMesh && isWebView){
        SILOG(cppoh,info,"* I found a WEBVIEW known as "<<proximateObjectId.object());
        proxyObj = ProxyObjectPtr(new ProxyWebViewObject(proxyMgr, proximateObjectId, (ODP::Service*)realThis.get()));
    } else {
        SILOG(cppoh,info, "* I found a MESH named " << proximateObjectId.object());
        proxyObj = ProxyObjectPtr(new ProxyMeshObject(proxyMgr, proximateObjectId, (ODP::Service*)realThis.get()));
    }
    proxyObj->setLocal(false);
    realThis->applyPositionUpdate(proxyObj, objLoc, true);
    proxyMgr->createObject(proxyObj, realThis->getTracker(space));
    for (int i = 0; i < sentMessage->body().reads_size(); ++i) {
        if (sentMessage->body().reads(i).has_return_status()) {
            continue;
        }
            const std::string &field = sentMessage->body().reads(i).field_name();
            realThis->receivedPropertyUpdate(proxyObj, sentMessage->body().reads(i).field_name(), sentMessage->body().reads(i).data());
    }
    {
        RPCMessage *request = new RPCMessage(realThis->getTracker(space),std::tr1::bind(&receivedPositionUpdateResponse, weakThis, _1, _2, _3));
        request->header().set_destination_space(proximateObjectId.space());
        request->header().set_destination_object(proximateObjectId.object());
        ::Sirikata::Protocol::LocRequest loc;
        loc.SerializeToString(request->body().add_message("LocRequest"));
        request->serializeSend();
    }
    return;
}
void VWObject::applyPositionUpdate(
    const ProxyObjectPtr &proxy,
    const ObjLoc &objLoc,
    bool force_reset) {
    if (!proxy) {
        return;
    }
    force_reset = force_reset || (objLoc.update_flags() & ObjLoc::FORCE);
    if (!objLoc.has_timestamp()) {
        objLoc.set_timestamp(proxy->getProxyManager()->getTimeOffsetManager()->now(*proxy));
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

void VWObject::receivedPropertyUpdate(
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
    if (propertyName == "WebViewURL") {
        Protocol::StringProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyWebViewObject *proxywv = dynamic_cast<ProxyWebViewObject*>(proxy.get());
            if (proxywv) {
                proxywv->loadURL(parsedProperty.value());
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
            PhysicalParameters params;
            parsePhysicalParameters(params, parsedProperty);
            params.name = proxymesh->getPhysical().name;        /// otherwise setPhysical will wipe it
            proxymesh->setPhysical(params);
        }
    }
    if (propertyName == "Parent") {
        Protocol::ParentProperty parsedProperty;
        parsedProperty.ParseFromString(arguments);
        if (parsedProperty.has_value()) {
            ProxyObjectPtr obj =proxy->getProxyManager()->getProxyObject(
                SpaceObjectReference(
                    proxy->getObjectReference().space(),
                    ObjectReference(parsedProperty.value())));
            if (obj && obj != proxy) {
                proxy->setParent(obj, proxy->getProxyManager()->getTimeOffsetManager()->now(*proxy));
            }
        }
    }
    if (propertyName == "Name") {
        Protocol::StringProperty parsedProperty;
        ProxyMeshObject *proxymesh = dynamic_cast<ProxyMeshObject*>(proxy.get());
        parsedProperty.ParseFromString(arguments);
        if (proxymesh && parsedProperty.has_value()) {
            PhysicalParameters params = proxymesh->getPhysical();
            params.name = parsedProperty.value();
            proxymesh->setPhysical(params);
        }
    }
}

void VWObject::receivedPositionUpdateResponse(
        const VWObjectWPtr &weakThus,
        SentMessage* sentMessage,
        const RoutableMessageHeader &hdr,
        MemoryReference bodyData) {
    RoutableMessage responseMessage(hdr, bodyData.data(), bodyData.length());
    VWObjectPtr thus(weakThus.lock());
    if (!thus) {
        delete static_cast<RPCMessage*>(sentMessage);
    }
    if (responseMessage.header().return_status()) {
        return;
    }
    Protocol::ObjLoc loc;
    loc.ParseFromString(responseMessage.body().message_arguments(0));

    const SpaceID &space = sentMessage->getSpace();
    ProxyManager *pm = thus->getProxyManager(space);
    if (pm) {
        ProxyObjectPtr obj(pm->getProxyObject(
                               SpaceObjectReference(space, sentMessage->getRecipient())));
        if (obj) {
            thus->applyPositionUpdate(obj,loc,false);
        }
    }
    if(thus->isLocal(SpaceObjectReference(hdr.destination_space(),hdr.source_object()))) {
        delete static_cast<RPCMessage*>(sentMessage);
    }else {
        static_cast<RPCMessage*>(sentMessage)->serializeSend(); // Resend position update each time we get one.
    }

}

void VWObject::parsePhysicalParameters(PhysicalParameters &out, const Protocol::PhysicalParameters &parsedProperty) {
    switch (parsedProperty.mode()) {
    case Protocol::PhysicalParameters::NONPHYSICAL:
        out.mode = PhysicalParameters::Disabled;
        break;
    case Protocol::PhysicalParameters::STATIC:
        out.mode = PhysicalParameters::Static;
        break;
    case Protocol::PhysicalParameters::DYNAMICBOX:
        out.mode = PhysicalParameters::DynamicBox;
        break;
    case Protocol::PhysicalParameters::DYNAMICSPHERE:
        out.mode = PhysicalParameters::DynamicSphere;
        break;
    case Protocol::PhysicalParameters::DYNAMICCYLINDER:
        out.mode = PhysicalParameters::DynamicCylinder;
        break;
    case Protocol::PhysicalParameters::CHARACTER:
        out.mode = PhysicalParameters::Character;
        break;
    default:
        out.mode = PhysicalParameters::Disabled;
    }
    out.density = parsedProperty.density();
    out.friction = parsedProperty.friction();
    out.bounce = parsedProperty.bounce();
    out.colMsg = parsedProperty.collide_msg();
    out.colMask = parsedProperty.collide_mask();
    out.hull = parsedProperty.hull();
    out.gravity = parsedProperty.gravity();
}
void VWObject::processRPC(const RoutableMessageHeader&msg, const std::string &name, MemoryReference args, String *response){
    std::ostringstream printstr;
    printstr<<"\t";

    SpaceID space = msg.destination_space();

    if (name == "ProxCall") {
        if (false && msg.source_object() != ObjectReference::spaceServiceID()) {
            SILOG(objecthost, error, "ProxCall message not coming from space: "<<msg.source_object());
            return;
        }
        ProxyManager *proxyMgr=getProxyManager(msg.source_space());
        Protocol::ProxCall proxCall;
        proxCall.ParseFromArray(args.data(), args.length());
        SpaceObjectReference proximateObjectId (msg.source_space(), ObjectReference(proxCall.proximate_object()));
        ProxyObjectPtr proxyObj (proxyMgr->getProxyObject(proximateObjectId));
        switch (proxCall.proximity_event()) {
          case Protocol::ProxCall::EXITED_PROXIMITY:
            printstr<<"ProxCall EXITED "<<proximateObjectId.object();
            if (proxyObj) {
                removeQueryInterest(proxCall.query_id(),proxyObj,proximateObjectId);
            } else {
                printstr<<" (unknown obj)";
            }
            break;
          case Protocol::ProxCall::ENTERED_PROXIMITY:
            printstr<<"ProxCall ENTERED "<<proximateObjectId.object();
            addQueryInterest(proxCall.query_id(),proximateObjectId);
            if (!proxyObj) { // FIXME: We may get one of these for each prox query. Keep track of in-progress queries in ProxyManager.
                printstr<<" (Requesting information...)";

                {
                    RPCMessage *locRequest = new RPCMessage(getTracker(space),std::tr1::bind(&VWObject::receivedProxObjectLocation,
                                                                                     getWeakPtr(), _1, _2, _3,
                                                        proxCall.query_id()));
                    locRequest->header().set_destination_space(proximateObjectId.space());
                    locRequest->header().set_destination_object(proximateObjectId.object());
                    Protocol::LocRequest loc;
                    loc.SerializeToString(locRequest->body().add_message("LocRequest"));


                    locRequest->setTimeout(Duration::seconds(5.0));
                    locRequest->serializeSend();
                }
            } else {
                printstr<<" (Already known)";
                proxyMgr->createObject(proxyObj, this->getTracker(space));
            }
            break;
          case Protocol::ProxCall::STATELESS_PROXIMITY:
            printstr<<"ProxCall Stateless'ed "<<proximateObjectId.object();
            // Do not create a proxy object in this case: This message is for one-time queries
            break;
        }
        SILOG(proxyobject,debug,printstr.str());
    }
}

}
