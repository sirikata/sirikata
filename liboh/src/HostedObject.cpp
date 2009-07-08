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
    ObjectStreamMap::iterator iter = mObjectStreams.insert(
        ObjectStreamMap::value_type(
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

using Sirikata::Protocol::NewObj;
using Sirikata::Protocol::IObjLoc;

void HostedObject::initializeConnect(const UUID &objectName, const Location&startingLocation,const String&mesh, const BoundingSphere3f&meshBounds, const SpaceID&spaceID, const HostedObjectWPtr&spaceConnectionHint) {

    std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection;

    HostedObjectPtr spaceConnectionHintPtr;
    if (spaceConnectionHintPtr = spaceConnectionHint.lock()) {
        ObjectStreamMap::const_iterator iter = spaceConnectionHintPtr->mObjectStreams.find(spaceID);
        if (iter != spaceConnectionHintPtr->mObjectStreams.end()) {
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

    mInternalObjectReference=objectName;

    std::string serializedNewObj;
    newObj.SerializeToString(&serializedNewObj);
    assert(send(messageHeader, MemoryReference(serializedNewObj))); //conn should be connected
}

void HostedObject::initializeRestoreFromDatabase(const UUID &objectName) {
    initializeConnect(objectName, Location(), String(), BoundingSphere3f(), SpaceID::null(), HostedObjectWPtr());
}
void HostedObject::initializeScripted(const UUID&objectName, const String& script, const SpaceID&id,const HostedObjectWPtr&spaceConnectionHint) {
    mInternalObjectReference=objectName;
    if (id!=SpaceID::null()) {
        //bind script to object...script might be a remote ID, so need to bind download target, etc
        std::tr1::shared_ptr<HostedObject> parentObject=spaceConnectionHint.lock();
        std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection;
        ObjectStreamMap::iterator where;
        if (parentObject&&(where=parentObject->mObjectStreams.find(id))!=mObjectStreams.end()) {
            topLevelConnection=where->second.mSpaceConnection.getTopLevelStream();
        }else {
            topLevelConnection=mObjectHost->connectToSpace(id);
        }
        // sending initial packet is done by the script!
        //conn->send(initializationPacket,Network::ReliableOrdered);
    }
}
bool HostedObject::send(RoutableMessageHeader hdr, const MemoryReference&body) {
    ObjectStreamMap::iterator where=mObjectStreams.find(hdr.destination_space());
    hdr.clear_destination_space();
    if (where!=mObjectStreams.end()) {
        String serialized_header;
        hdr.SerializeToString(&serialized_header);
        where->second.mSpaceConnection->send(MemoryReference(serialized_header),body, Network::ReliableOrdered);
        return true;
    }
    return false;
}

void HostedObject::processMessage(const HostedObjectWPtr&thus, const ReceivedMessage &msg) {
    std::cout << "Response Message from: " << msg.sourceObject << " port " << msg.sourcePort << " to " << msg.destinationPort << std::endl;
    std::cout << "message contents: "<<msg.name<<std::endl;
}

void HostedObject::receivedRoutableMessage(const HostedObjectWPtr&thus,const SpaceID&sid, const Network::Chunk&msg) {
    HostedObjectPtr realThis (thus.lock());
    RoutableMessage message;
    message.ParseFromArray(&msg[0],msg.size());
    if (!realThis) {
        SILOG(objecthost,error,"Received message for dead HostedObject. SpaceID = "<<sid<<"; DestObject = "<<message.header().destination_object());
        return;
    }
    int numNames = message.body().message_names_size();
    int numBodies = message.body().message_arguments_size();
    for (int i = 0; i < numNames && i < numBodies; ++i) {
        const std::string &name = message.body().message_names(i);
        MemoryReference body (message.body().message_arguments(i));
        ReceivedMessage msg (sid, message.header().source_object(), message.header().source_port(), message.header().destination_port(), name, body);
        realThis->processMessage(realThis, msg);
    }
}
void HostedObject::disconnectionEvent(const HostedObjectWPtr&weak_thus,const SpaceID&sid, const String&reason) {
    std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
    if (thus) {
        ObjectStreamMap::iterator where=thus->mObjectStreams.find(sid);
        if (where!=thus->mObjectStreams.end()) {
            thus->mObjectStreams.erase(where);//FIXME do we want to back this up to the database first?
        }
    }
}


}
