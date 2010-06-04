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

#include <sirikata/space/Platform.hpp>
#include <sirikata/core/util/BoundingBox.hpp>
#include <sirikata/space/Space.hpp>
#include <sirikata/core/util/ObjectReference.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/network/IOServiceFactory.hpp>
#include <sirikata/core/network/IOService.hpp>
#include <sirikata/space/ObjectConnections.hpp>
#include <Space_Sirikata.pbj.hpp>
#include <sirikata/core/util/RoutableMessage.hpp>
#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/proximity/Platform.hpp>
#include <sirikata/proximity/ProximitySystem.hpp>
#include <sirikata/proximity/ProximityConnection.hpp>
#include <sirikata/proximity/ProximityConnectionFactory.hpp>
#include <sirikata/proximity/BridgeProximitySystem.hpp>
#include <sirikata/core/options/Options.hpp>
#include <sirikata/space/Loc.hpp>
#include <sirikata/space/Registration.hpp>
#include <sirikata/space/Router.hpp>
#include <sirikata/space/Physics.hpp>
#include <sirikata/space/Subscription.hpp>
#include <proxyobject/Platform.hpp>
namespace Sirikata {
namespace Space {
Time Space::now()const{
    return Time::now(Duration::zero());//FIXME for distribution
}
Space::Space(const SpaceID&id, const String&options):mID(id),mNodeID(UUID::random()),mIO(Network::IOServiceFactory::makeIOService()),mBounds(Vector3d(-1.0e38,-1.0e38,-1.0e38),Vector3d(1.0e38,1.0e38,1.0e38)) {

    unsigned int rsi=Services::REGISTRATION;
    unsigned int lsi=Services::LOC;
    unsigned int gsi=Services::GEOM;
    //unsigned int osi;
    //unsigned int csi;
    unsigned int fsi=Services::ROUTER;
    unsigned char randomKey[SHA256::static_size]={3,2,1,4,5,6,3,8,235,124,24,15,26,165,123,95,
                                                  53,2,111,114,125,166,123,158,232,144,4,152,221,161,122,96};

    Protocol::SpaceServices spaceServices;
    spaceServices.set_registration_port(rsi);
    spaceServices.set_loc_port(lsi);//UUID(lsi,sizeof(lsi)));
    spaceServices.set_geom_port(gsi);//UUID(gsi,sizeof(gsi)));
    spaceServices.set_persistence_port(Services::PERSISTENCE);//UUID(gsi,sizeof(gsi)));
    spaceServices.set_rpc_port(Services::RPC);//UUID(gsi,sizeof(gsi)));
    spaceServices.set_physics_port(Services::PHYSICS);//UUID(gsi,sizeof(gsi)));
    //spaceServices.set_oseg(osi);//UUID(osi,sizeof(osi)));
    //spaceServices.set_cseg(csi);//UUID(csi,sizeof(csi)));
    spaceServices.set_router_port(fsi);//UUID(fsi,sizeof(fsi)));

    mRegistration = new Registration(SHA256::convertFromBinary(randomKey));
    mLoc=new Loc;
    Proximity::ProximityConnection*proxCon=Proximity::ProximityConnectionFactory::getSingleton().getDefaultConstructor()(mIO,"");
    mGeom=new Proximity::BridgeProximitySystem(proxCon,spaceServices.registration_port());
    mPhysics =Physics::construct<Physics>(this,mIO,SpaceObjectReference(mID,mNodeID),spaceServices.physics_port());
    mPhysicsProxyObjects=mPhysics->getProxyManager();
    mSubscription=new Subscription(mIO);
    mRouter=NULL;
    mCoordinateSegmentation=NULL;
    mObjectSegmentation=NULL;
    OptionValue*host;
    OptionValue*port;

    String spaceServicesString;
    spaceServices.SerializeToString(&spaceServicesString);

    OptionValue*streamlib;
    OptionValue*streamoptions;
    InitializeClassOptions("space",this,
                           host=new OptionValue("host","0.0.0.0",OptionValueType<String>(),"sets the hostname that runs the space."),
                           port=new OptionValue("port","5943",OptionValueType<String>(),"sets the port that runs the space."),
                           streamlib=new OptionValue("protocol","",OptionValueType<String>(),"Sets the stream library to listen upon"),
                           streamoptions=new OptionValue("options","",OptionValueType<String>(),"Options for the created listener"),
						   NULL);
    OptionSet::getOptions("space",this)->parse(options);

    mObjectConnections=new ObjectConnections(Network::StreamListenerFactory::getSingleton()
                                                .getConstructor(streamlib->as<String>())(mIO,
                                                                                         Network::StreamListenerFactory::getSingleton()
                                                                                         .getOptionParser(streamlib->as<String>())(streamoptions->as<String>())),
                                             Network::Address(host->as<String>(),port->as<String>(),streamlib->as<String>())
                                             //spaceServicesString
                                             );
    mObjectConnections->forwardMessagesTo(this);
    mPhysics->forwardMessagesTo(mObjectConnections);
    mServices[spaceServices.registration_port()]=mRegistration;
    mServices[spaceServices.loc_port()]=mLoc;
    mServices[spaceServices.geom_port()]=mGeom;
    mServices[spaceServices.physics_port()]=&*mPhysics;
    mServices[Services::SUBSCRIPTION]=mSubscription->getSubscriptionService();
    mServices[Services::BROADCAST]=mSubscription->getBroadcastService();
    //mServices[ObjectReference(spaceServices.oseg_port())]=mObjectSegmentation;
    //mServices[ObjectReference(spaceServices.cseg_port())]=mCoordinateSegmentation;
    //mServices[ObjectReference(spaceServices.router_port())]=mRouter;
    mRegistration->forwardMessagesTo(mObjectConnections);
    mRegistration->forwardMessagesTo(mLoc);
    mRegistration->forwardMessagesTo(mGeom);
    mGeom->forwardMessagesTo(mObjectConnections);
    mLoc->forwardMessagesTo(mGeom);
    mSubscription->getSendService()->forwardMessagesTo(mObjectConnections);
    mSubscription->getBroadcastService()->forwardMessagesTo(mObjectConnections);
}
void Space::run() {
    RoutableMessageHeader nodeObjectHeader;
    RoutableMessageBody body;
    nodeObjectHeader.set_destination_port(Services::GEOM);
    nodeObjectHeader.set_source_port(Services::REGISTRATION);
    nodeObjectHeader.set_source_object(ObjectReference::null());
    nodeObjectHeader.set_destination_object(ObjectReference::null());
    nodeObjectHeader.set_destination_space(id());
    Protocol::RetObj spaceNodeObject;
    spaceNodeObject.set_object_reference(mNodeID.getObjectUUID());
    String newNodeObjStr;
    spaceNodeObject.SerializeToString(&newNodeObjStr);
    body.add_message("RetObj", newNodeObjStr);
    String nodeRegistration;
    body.SerializeToString(&nodeRegistration);
    mGeom->processMessage(nodeObjectHeader,MemoryReference(nodeRegistration));
    mPhysics->setBounds(mBounds);
    mIO->run();
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
    if (header.destination_object()==ObjectReference::spaceServiceID()||header.destination_object()==mNodeID) {
        uint64 port=header.destination_port();
        std::tr1::unordered_map<unsigned int,MessageService*>::iterator where=mServices.find(port);
        if (where!=mServices.end()) {
            where->second->processMessage(header,message_body);
        }else {
            SILOG(space,warning,"Do not know where to forward space-destined message to "<<header.destination_port()<< " aka "<<port);
        }
    }else if (mRouter) {
        mRouter->processMessage(header,message_body);
    }else {
        SILOG(space,warning,"Do not know where to forward message to "<<header.destination_object().toString());
    }
}
ProxyManager *Space::getProxyManager(){
    return mPhysicsProxyObjects;
}
Space::~Space() {
}
}
} // namespace Sirikata
