/*  Sirikata libspace -- Space
 *  Space.cpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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

#include <space/Platform.hpp>
#include <space/Space.hpp>
#include <util/ObjectReference.hpp>
#include <network/Stream.hpp>
#include <network/TCPStreamListener.hpp>
#include <network/IOServiceFactory.hpp>
#include <space/ObjectConnections.hpp>
#include <Space_Sirikata.pbj.hpp>
#include <util/RoutableMessage.hpp>
#include <proximity/Platform.hpp>
#include <proximity/ProximitySystem.hpp>
#include <proximity/ProximityConnection.hpp>
#include <proximity/ProximityConnectionFactory.hpp>
#include <proximity/BridgeProximitySystem.hpp>

#include <space/Loc.hpp>
#include <space/Registration.hpp>
namespace Sirikata {

Space::Space(const SpaceID&id):mID(id),mIO(Network::IOServiceFactory::makeIOService()) {
    unsigned char rsi[UUID::static_size]={1,0};
    unsigned char lsi[UUID::static_size]={2,0};
    unsigned char gsi[UUID::static_size]={3,0};
    unsigned char osi[UUID::static_size]={3,0};
    unsigned char csi[UUID::static_size]={3,0};
    unsigned char fsi[UUID::static_size]={3,0};
    unsigned char randomKey[SHA256::static_size]={3,2,1,4,5,6,3,8,235,124,24,15,26,165,123,95,
                                                  53,2,111,114,125,166,123,158,232,144,4,152,221,161,122,96};
    
    Protocol::SpaceServices spaceServices;
    spaceServices.set_registration(UUID(rsi,sizeof(rsi)));
    spaceServices.set_loc(UUID(lsi,sizeof(lsi)));
    spaceServices.set_geom(UUID(gsi,sizeof(gsi)));
    spaceServices.set_oseg(UUID(osi,sizeof(osi)));
    spaceServices.set_cseg(UUID(csi,sizeof(csi)));
    spaceServices.set_router(UUID(fsi,sizeof(fsi)));
    
    mRegistration = new Registration(ObjectReference(spaceServices.registration()),
                                     SHA256::convertFromBinary(randomKey));
    mLoc=new Loc(ObjectReference(spaceServices.loc()),
                 ObjectReference(spaceServices.registration()));
    Proximity::ProximityConnection*proxCon=Proximity::ProximityConnectionFactory::getSingleton().getDefaultConstructor()(mIO,"");
    mGeom=new Proximity::BridgeProximitySystem(proxCon,ObjectReference(spaceServices.registration()));
    mGeom->forwardMessagesTo(this);
    mRouter=NULL;
    mCoordinateSegmentation=NULL;
    mObjectSegmentation=NULL;
    String port="5943";
    String spaceServicesString;
    spaceServices.SerializeToString(&spaceServicesString);
    mObjectConnections=new ObjectConnections(new Network::TCPStreamListener(*mIO),
                                            Network::Address("localhost",port),
                                            ObjectReference(spaceServices.registration()),       
                                            spaceServicesString);
    mServices[ObjectReference(spaceServices.registration())]=mRegistration;
    mServices[ObjectReference(spaceServices.loc())]=mLoc;
    mServices[ObjectReference(spaceServices.geom())]=mGeom;
    //mServices[ObjectReference(spaceServices.oseg())]=mObjectSegmentation;
    //mServices[ObjectReference(spaceServices.cseg())]=mCoordinateSegmentation;
    mServices[ObjectReference(spaceServices.router())]=mRouter;
    mRegistration->forwardMessagesTo(mLoc);
    mRegistration->forwardMessagesTo(mGeom);
    mRegistration->forwardMessagesTo(mObjectConnections);
    mGeom->forwardMessagesTo(mObjectConnections);
    mLoc->forwardMessagesTo(mGeom);
    mLoc->forwardMessagesTo(mObjectConnections);//FIXME: is this necessary
}
void Space::run() {
    Network::IOServiceFactory::runService(mIO);
}
void Space::processMessage(const ObjectReference*ref,MemoryReference message){
    
    RoutableMessageHeader hdr;
    MemoryReference message_body=hdr.ParseFromArray(message.data(),message.size());
    if (!hdr.has_source_object()&&ref) {
        hdr.set_source_object(*ref);
    }
    this->processMessage(hdr,message_body);
}

void Space::processMessage(const RoutableMessageHeader&header,MemoryReference message_body) {
    std::tr1::unordered_map<ObjectReference,MessageService*,ObjectReference::Hasher>::iterator where=mServices.find(header.destination_object());
    if (where!=mServices.end()) {
        where->second->processMessage(header,message_body);
    }else if (mRouter) {
        mRouter->processMessage(header,message_body);
    }else {
        SILOG(space,warning,"Do not know where to forward message to "<<header.destination_object().toString());
    }
}
Space::~Space() {
}

} // namespace Sirikata
