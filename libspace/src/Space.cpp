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
#include <util/KnownServices.hpp>
#include <proximity/Platform.hpp>
#include <proximity/ProximitySystem.hpp>
#include <proximity/ProximityConnection.hpp>
#include <proximity/ProximityConnectionFactory.hpp>
#include <proximity/BridgeProximitySystem.hpp>

#include <space/Loc.hpp>
#include <space/Registration.hpp>
#include <space/Router.hpp>
namespace Sirikata {

Space::Space(const SpaceID&id):mID(id),mIO(Network::IOServiceFactory::makeIOService()) {
    unsigned int rsi=Services::REGISTRATION;
    unsigned int lsi=Services::LOC;
    unsigned int gsi=Services::GEOM;
    unsigned int osi;
    unsigned int csi;
    unsigned int fsi=Services::ROUTER;
    unsigned char randomKey[SHA256::static_size]={3,2,1,4,5,6,3,8,235,124,24,15,26,165,123,95,
                                                  53,2,111,114,125,166,123,158,232,144,4,152,221,161,122,96};
    
    Protocol::SpaceServices spaceServices;
    spaceServices.set_registration_port(rsi);
    spaceServices.set_loc_port(lsi);//UUID(lsi,sizeof(lsi)));
    spaceServices.set_geom_port(gsi);//UUID(gsi,sizeof(gsi)));
    //spaceServices.set_oseg(osi);//UUID(osi,sizeof(osi)));
    //spaceServices.set_cseg(csi);//UUID(csi,sizeof(csi)));
    spaceServices.set_router_port(fsi);//UUID(fsi,sizeof(fsi)));
    
    mRegistration = new Registration(SHA256::convertFromBinary(randomKey));
    mLoc=new Loc;
    Proximity::ProximityConnection*proxCon=Proximity::ProximityConnectionFactory::getSingleton().getDefaultConstructor()(mIO,"");
    mGeom=new Proximity::BridgeProximitySystem(proxCon,spaceServices.registration_port());
    mGeom->forwardMessagesTo(this);
    mRouter=NULL;
    mCoordinateSegmentation=NULL;
    mObjectSegmentation=NULL;
    String port="5943";
    String spaceServicesString;
    spaceServices.SerializeToString(&spaceServicesString);
    mObjectConnections=new ObjectConnections(new Network::TCPStreamListener(*mIO),
                                             Network::Address("localhost",port)
                                             //spaceServicesString
                                             );
    mServices[spaceServices.registration_port()]=mRegistration;
    mServices[spaceServices.loc_port()]=mLoc;
    mServices[spaceServices.geom_port()]=mGeom;
    //mServices[ObjectReference(spaceServices.oseg_port())]=mObjectSegmentation;
    //mServices[ObjectReference(spaceServices.cseg_port())]=mCoordinateSegmentation;
    //mServices[ObjectReference(spaceServices.router_port())]=mRouter;
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
    if (header.destination_object()==ObjectReference::spaceServiceID()) {
        std::tr1::unordered_map<unsigned int,MessageService*>::iterator where=mServices.find(header.destination_port());
        if (where!=mServices.end()) {
            where->second->processMessage(header,message_body);
        }else {
            SILOG(space,warning,"Do not know where to forward space-destined message to "<<header.destination_port());
        }
    }else if (mRouter) {
        mRouter->processMessage(header,message_body);
    }else {
        SILOG(space,warning,"Do not know where to forward message to "<<header.destination_object().toString());
    }
}

Space::~Space() {
}

} // namespace Sirikata
