/*  Sirikata Object Host -- Proximity Connection Class
 *  SingleStreamProximityConnection.cpp
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

#include <proximity/Platform.hpp>
#include <sirikata/core/options/Options.hpp>
#include "Proximity_Sirikata.pbj.hpp"
#include <sirikata/core/util/RoutableMessage.hpp>
#include "proximity/ProximityConnection.hpp"
#include "proximity/SingleStreamProximityConnection.hpp"
#include "proximity/ProximitySystem.hpp"
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamFactory.hpp>

namespace Sirikata { namespace Proximity {
namespace {

void connectionCallback(ProximityConnection* con, Network::Stream::ConnectionStatus status, const std::string&reason) {
    if (status!=Network::Stream::Connected)
        con->streamDisconnected();
}
void readProximityMessage(std::tr1::weak_ptr<Network::Stream> mLock,
                          MessageService** system,
                          const ObjectReference &object,
    const Network::Chunk&chunk,
    const Network::Stream::PauseReceiveCallback& pauseReceive) {
    std::tr1::shared_ptr<Network::Stream> lok=mLock.lock();//make sure this proximity connection will not disappear;
    if (lok) {
        MessageService*sys=*system;
        if (sys&&!chunk.empty()) {
            RoutableMessageHeader hdr;
            MemoryReference body=hdr.ParseFromArray(&chunk[0],chunk.size());
            hdr.set_destination_object(object);
            sys->processMessage(hdr,body);
        }
    }
    return;
}
}

bool SingleStreamProximityConnection::forwardMessagesTo(MessageService*parent) {
    if (mParent!=NULL)
        return false;
    mParent=parent;
    return true;
}
bool SingleStreamProximityConnection::endForwardingMessagesTo(MessageService*parent) {
    if (mParent==parent) {
        mParent=NULL;
        return true;
    }
    return false;
}

ProximityConnection* SingleStreamProximityConnection::create(Network::IOService*io, const String&options){
    OptionValue*address=new OptionValue("address","localhost:6408",Network::Address("localhost","6408"),"sets the fully qualified address that runs the proximity manager.");
    OptionValue*host;
    OptionValue*port;
    OptionValue*streamlib;
    OptionValue*streamoptions;
    InitializeClassOptions("proximityconnection",address,
                           address,
                           host=new OptionValue("host","",OptionValueType<String>(),"sets the hostname that runs the proximity manager."),
                           port=new OptionValue("port","",OptionValueType<String>(),"sets the port that runs the proximity manager."),
                           streamlib=new OptionValue("protocol","",OptionValueType<String>(),"Sets the stream library to connect"),
                           streamoptions=new OptionValue("options","",OptionValueType<String>(),"Options for the created stream"),
						   NULL);
    OptionSet::getOptions("proximityconnection",address)->parse(options);
    if (host->as<String>().empty()||port->as<String>().empty()) {
        return new SingleStreamProximityConnection(address->as<Network::Address>(),*io,streamlib->as<String>(),streamoptions->as<String>());
    }
    return new SingleStreamProximityConnection(Network::Address(host->as<String>(),port->as<String>()),*io,streamlib->as<String>(),streamoptions->as<String>());

}
SingleStreamProximityConnection::SingleStreamProximityConnection(const Network::Address&addy, Network::IOService&io,const String&streamlib, const String&streamoptions):mParent(NULL),mConnectionStream(Network::StreamFactory::getSingleton().getConstructor(streamlib)(&io,Network::StreamFactory::getSingleton().getOptionParser(streamlib)(streamoptions))) {
        mConnectionStream->connect(addy,
                                   &Network::Stream::ignoreSubstreamCallback,
                                   std::tr1::bind(&connectionCallback,this,_1,_2),
                                   &Network::Stream::ignoreReceivedCallback,
                                   &Network::Stream::ignoreReadySendCallback);

    }
void SingleStreamProximityConnection::streamDisconnected() {
    SILOG(proximity,error,"Lost connection with proximity manager");
}

void SingleStreamProximityConnection::processMessage(const RoutableMessageHeader&hdr,
                                                     MemoryReference message_body) {
    ObjectStreamMap::iterator where=mObjectStreams.find(hdr.has_source_object()?hdr.source_object():ObjectReference::null());
    if (where==mObjectStreams.end()) {
        where=mObjectStreams.find(hdr.has_destination_object()?hdr.destination_object():ObjectReference::null());
    }
    if (where==mObjectStreams.end()) {
        SILOG(proximity,error,"Cannot locate object with OR "<<hdr.source_object()<<" in the proximity connection map: "<<hdr.has_source_object());
    } else {
        std::string data;
        RoutableMessageHeader rmh;
        rmh.set_source_port(hdr.source_port());
        rmh.SerializeToString(&data);
        where->second->send(MemoryReference(data),message_body,Network::ReliableOrdered);
    }
}


SingleStreamProximityConnection::~SingleStreamProximityConnection() {
    for (ObjectStreamMap::iterator i=mObjectStreams.begin(),ie=mObjectStreams.end();i!=ie;++i) {
        delete i->second;
    }
    std::tr1::weak_ptr<Network::Stream> lok(mConnectionStream);
    mConnectionStream=std::tr1::shared_ptr<Network::Stream>();
    while (lok.lock()) {
        //sleep? wait for callback to complete
    }
    mObjectStreams.clear();
}
void SingleStreamProximityConnection::constructObjectStream(const ObjectReference&obc) {
    mObjectStreams[obc]
        = mConnectionStream->clone(&Network::Stream::ignoreConnectionCallback,
            std::tr1::bind(&readProximityMessage,std::tr1::weak_ptr<Network::Stream>(mConnectionStream), &mParent, obc,_1,_2),
            &Network::Stream::ignoreReadySendCallback);
}
void SingleStreamProximityConnection::deleteObjectStream(const ObjectReference&obc) {
    ObjectStreamMap::iterator where=mObjectStreams.find(obc);
    if (where==mObjectStreams.end()) {
        SILOG(proximity,error,"Cannot locate object with OR "<<obc<<" in the proximity connection map");
    }else {
        where->second->close();
        delete where->second;
        mObjectStreams.erase(where);
    }
}
} }
