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
#include "network/Stream.hpp"
#include "util/RoutableMessageHeader.hpp"
#include "util/SpaceObjectReference.hpp"
#include "oh/SpaceConnection.hpp"
#include "oh/TopLevelSpaceConnection.hpp"
#include "oh/HostedObject.hpp"
#include "oh/ObjectHost.hpp"
namespace Sirikata {

HostedObject::PerSpaceData::PerSpaceData(const std::tr1::shared_ptr<TopLevelSpaceConnection>&topLevel,Network::Stream*stream) :mReference(ObjectReference::null()),mSpaceConnection(topLevel,stream) {}

HostedObject::HostedObject(ObjectHost*parent):mInternalObjectReference(UUID::null()) {
    mObjectHost=parent;
}
namespace {
    static void connectionEvent(const std::tr1::weak_ptr<HostedObject>&thus,
                                const SpaceID&sid,
                                Network::Stream::ConnectionStatus ce,
                                const String&reason) {
        if (ce!=Network::Stream::Connected) {
            HostedObject::disconnectionEvent(thus,sid,reason);
        }
    }
}
void HostedObject::cloneTopLevelStream(const SpaceID&sid,const std::tr1::shared_ptr<TopLevelSpaceConnection>&tls) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;
    mObjectStreams.insert(ObjectStreamMap::value_type(sid,
                                                      PerSpaceData(tls,
                                                                   tls->topLevelStream()->clone(std::tr1::bind(&connectionEvent,
                                                                                                               getWeakPtr(),
                                                                                                               sid,
                                                                                                               _1,
                                                                                                               _2),
                                                                                                std::tr1::bind(&HostedObject::processMessage,
                                                                                                               getWeakPtr(),
                                                                                                               sid,
                                                                                                               _1)))));
}
void HostedObject::initialize(const UUID &objectName) {
    mInternalObjectReference=objectName;
    SpaceID desiredSpace=SpaceID::null();    //FIXME this should be restored from database
    std::tr1::shared_ptr<TopLevelSpaceConnection> topLevelConnection=mObjectHost->connectToSpace(desiredSpace);
    cloneTopLevelStream(desiredSpace,topLevelConnection);
    Network::Chunk initializationPacket;//FIXME
    ObjectStreamMap::iterator where=mObjectStreams.find(desiredSpace);
    if (where==mObjectStreams.end()) {
        SILOG(oh,error,"Object Stream End reached during intiialization");
    }else {
        where->second.mSpaceConnection->send(initializationPacket,Network::ReliableOrdered);
    }
}
void HostedObject::initialize(const UUID&objectName, const String& script, const SpaceID&id,const std::tr1::weak_ptr<HostedObject>&spaceConnectionHint) {
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
        cloneTopLevelStream(id,topLevelConnection);
        Network::Chunk initializationPacket;//FIXME
        where=mObjectStreams.find(id);
        if (where==mObjectStreams.end()) {
            SILOG(oh,error,"Object Stream End reached during intiialization");
        }else {
            where->second.mSpaceConnection->send(initializationPacket,Network::ReliableOrdered);
        }
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
void HostedObject::processMessage(const std::tr1::weak_ptr<HostedObject>&thus,const SpaceID&sid, const Network::Chunk&) {
    assert(false);//FIXME implement
}
void HostedObject::disconnectionEvent(const std::tr1::weak_ptr<HostedObject>&weak_thus,const SpaceID&sid, const String&reason) {
    std::tr1::shared_ptr<HostedObject>thus=weak_thus.lock();
    if (thus) {
        ObjectStreamMap::iterator where=thus->mObjectStreams.find(sid);
        if (where!=thus->mObjectStreams.end()) {
            thus->mObjectStreams.erase(where);//FIXME do we want to back this up to the database first?
        }
    }
}


}
