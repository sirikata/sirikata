/*  Sirikata libspace -- Object Connection Services
 *  ObjectConnections.hpp
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
#include <space/Platform.hpp>
#include "network/Stream.hpp"
#include "network/StreamListener.hpp"
#include "util/UUID.hpp"
#include "util/ObjectReference.hpp"
#include "Space_Sirikata.pbj.hpp"
#include "util/RoutableMessage.hpp"
#include "space/ObjectConnections.hpp"
namespace Sirikata {
ObjectConnections::ObjectConnections(Network::StreamListener*listener,
                                     const Network::Address&listenAddress,
                                     const ObjectReference&registrationServiceIdentifier,
                                     const String&introductoryMessage) {

    mSpaceServiceIntroductionMessage=introductoryMessage;
    mSpace=NULL;
    mListener=listener;
    mRegistrationService=registrationServiceIdentifier;
    using std::tr1::placeholders::_1;    using std::tr1::placeholders::_2;
    mListener->listen(listenAddress,
                      std::tr1::bind(&ObjectConnections::newStreamCallback,this,_1,_2));
}
void ObjectConnections::newStreamCallback(Network::Stream*stream, Network::Stream::SetCallbacks&callbacks) {
    UUID temporaryId=UUID::random();
    mStreams[stream].setId(temporaryId);
    mTemporaryStreams[temporaryId].mStream=stream;
    using std::tr1::placeholders::_1;    using std::tr1::placeholders::_2;
    callbacks(std::tr1::bind(&ObjectConnections::connectionCallback,this,stream,_1,_2),
              std::tr1::bind(&ObjectConnections::bytesReceivedCallback,this,stream,_1));
}
void ObjectConnections::bytesReceivedCallback(Network::Stream*stream, const Network::Chunk&chunk) {
    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.find(stream);
    RoutableMessageHeader hdr;
    MemoryReference chunkRef(chunk);
    MemoryReference message_body=hdr.ParseFromArray(chunkRef.data(),chunkRef.size());
    hdr.set_source_object(ObjectReference(where->second.uuid()));
    if (((!hdr.has_destination_object())||hdr.destination_object()==ObjectReference::null())&&message_body.size()==0) {
        stream->send(MemoryReference(mSpaceServiceIntroductionMessage),Network::ReliableOrdered);
    }else
    if (hdr.has_destination_object()&&hdr.destination_object()==mRegistrationService) {
        RoutableMessageBody rmb;
        bool success=rmb.ParseFromArray(message_body.data(),message_body.size());
        Protocol::RetObj ro;
        if (success) {
            bool disconnection=false;
            bool connection=false;
            int i;
            for (i=0;i<rmb.message_arguments_size();++i){
                disconnection=disconnection||(rmb.message_names(i)=="DelObj");
                connection=connection||(rmb.message_names(i)=="NewObj");
            }
            if (connection||where->second.connected()) {
                mSpace->processMessage(hdr,MemoryReference(message_body));
            }else {
                std::map<UUID,TemporaryStreamData>::iterator twhere=mTemporaryStreams.find(where->second.uuid());
                if (twhere!=mTemporaryStreams.end()) {
                    twhere->second.mPendingMessages.push_back(chunk);
                }else{
                    SILOG(space,warning,"Dropping message from "<<where->second.uuid().toString()<<" due to already disconnected object");
                }
            }
            if (disconnection) {
                where->second.setDisconnected();
                //don't disconnect stream immediately, should get message back from disconnection service
            }
        }
    } else if (where->second.connected()) {
        mSpace->processMessage(hdr,MemoryReference(message_body));                
    } else {//if not connected, this item will be inserted to list of to-be-sent
        std::map<UUID,TemporaryStreamData>::iterator twhere=mTemporaryStreams.find(where->second.uuid());
        if (twhere!=mTemporaryStreams.end()) {
            twhere->second.mPendingMessages.push_back(chunk);
        }else{
            SILOG(space,warning,"Dropping message from "<<where->second.uuid().toString()<<" due to already disconnected object");
        }
    }
}
void ObjectConnections::forgeDisconnectionMessage(const ObjectReference&ref) {
    RoutableMessage rm;
    Protocol::DelObj delObj;
    rm.header().set_destination_object(mRegistrationService);
    rm.header().set_source_object(ref);
    rm.body().add_message_names("DelObj");
    rm.body().add_message_arguments(NULL,0);    
    delObj.SerializeToString(&rm.body().message_arguments(0));
    std::string serialized_message_body;
    rm.body().SerializeToString(&serialized_message_body);
    mSpace->processMessage(rm.header(),MemoryReference(serialized_message_body));
}
void ObjectConnections::connectionCallback(Network::Stream*stream, Network::Stream::ConnectionStatus status, const std::string&reason){
    if (status!=Network::Stream::Connected) {
        SILOG(space,debug,"Connection lost "<<reason);
        std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.find(stream);
        if (where!=mStreams.end()) {
            std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator uwhere=mActiveStreams.find(where->second.uuid());
            std::map<UUID,TemporaryStreamData>::iterator twhere;
            if (uwhere!=mActiveStreams.end()) {
                if (where->second.connected())
                    forgeDisconnectionMessage(ObjectReference(uwhere->first));              
                mActiveStreams.erase(uwhere);
            }else if ((twhere=mTemporaryStreams.find(where->second.uuid()))!=mTemporaryStreams.end()) {
                mTemporaryStreams.erase(twhere);
            }else {
                SILOG(space,error,"Stream with unknown reference "<<where->second.uuid().toString());
            }
            mStreams.erase(where);
        }else {
            SILOG(space,error,"Stream not found "<<reason);
        }
        delete stream;
    }else {
        SILOG(space,insane,"Connected "<<reason);
    }
}
ObjectConnections::~ObjectConnections(){
    delete mListener;
    for (std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator i=mStreams.begin(),
             ie=mStreams.end();
         i!=ie;
         ++i) {
        delete i->first;
    }
}
Network::Stream* ObjectConnections::activeConnectionTo(const ObjectReference&ref) {
    std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator where=mActiveStreams.find(ref.getAsUUID());
    if (where==mActiveStreams.end()) 
        return NULL;
    return where->second;
}

Network::Stream* ObjectConnections::temporaryConnectionTo(const UUID&ref) {
    std::map<UUID,TemporaryStreamData>::iterator where=mTemporaryStreams.find(ref);
    if (where==mTemporaryStreams.end()) 
        return NULL;
    return where->second.mStream;
}

bool ObjectConnections::forwardMessagesTo(MessageService*ms) {
    if (mSpace==NULL) {
        mSpace=ms;
        return true;
    }
    return false;
}

bool ObjectConnections::endForwardingMessagesTo(MessageService*ms) {
    if (mSpace!=NULL) {
        mSpace=NULL;
        return true;
    }
    return false;
}

void ObjectConnections::processMessage(const ObjectReference*ref,MemoryReference message){
    ObjectReference newRef;
    RoutableMessageHeader hdr;
    MemoryReference body_array=hdr.ParseFromArray(message.data(),message.size());
    bool disconnectionAttempt=false;
    if (ref&&*ref==mRegistrationService) {
        if (!hdr.has_source_object())
            hdr.set_source_object(*ref);
        if (processNewObject(hdr,body_array, newRef)) {
            hdr.set_destination_object(newRef);
            ref=&newRef;
        }else {
            disconnectionAttempt=true;
        }
    }else {
        if (ref&&!hdr.has_destination_object()) {
            hdr.set_destination_object(*ref);
        }
        if (hdr.has_source_object()&&hdr.source_object()==mRegistrationService) {
            if (processNewObject(hdr,body_array,newRef)) {
                hdr.set_destination_object(newRef);
                ref=&newRef;
            }else {
                disconnectionAttempt=true;
            }
        }
    }
    processExistingObject(hdr,body_array,!disconnectionAttempt);
    if (disconnectionAttempt) {
        //Server ban of user or confirm of disconnect
        shutdownConnection(hdr.destination_object());
    }

}
void ObjectConnections::processMessage(const RoutableMessageHeader&header,
                    MemoryReference message_body){
    RoutableMessageHeader newHeader;
    const RoutableMessageHeader *hdr=&header;
    bool disconnectionAttempt=false;
    if (header.source_object()==mRegistrationService) {
        ObjectReference newRef;
        if (processNewObject(header,message_body,newRef)) {
            newHeader=header;
            newHeader.set_destination_object(newRef);
            hdr=&newHeader;
        }else {
            disconnectionAttempt=true;
        }
    }
    processExistingObject(*hdr,message_body,!disconnectionAttempt);
    if (disconnectionAttempt) {//Server ban of user, or confirm of disconnect
        shutdownConnection(header.destination_object());
    }
}

void ObjectConnections::shutdownConnection(const ObjectReference&ref) {
    Network::Stream*stream=NULL;
    std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator awhere=mActiveStreams.find(ref.getAsUUID());
    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.end();
    if (awhere!=mActiveStreams.end()){
        stream=awhere->second;
        where=mStreams.find(stream);
        mActiveStreams.erase(awhere);
    }else {
        std::map<UUID,TemporaryStreamData>::iterator twhere=mTemporaryStreams.find(where->second.uuid());       
        if (twhere!=mTemporaryStreams.end()) {
            stream=twhere->second.mStream;
            where=mStreams.find(stream);
            mTemporaryStreams.erase(twhere);
            SILOG(space,error,"FATAL: Stream connected yet found in temporary streams" << where->second.uuid().toString());
        }
    }
    if (where!=mStreams.end()) {
        mStreams.erase(where);
    }
    delete stream;
}

bool ObjectConnections::processNewObject(const RoutableMessageHeader&hdr,MemoryReference body_array, ObjectReference&newRef) {
    bool new_object=false;
    RoutableMessageBody rmb;
    if (hdr.has_destination_object()) {
        if (rmb.ParseFromArray(body_array.data(),body_array.size())) {
            if (rmb.message_arguments_size()) {
                if (rmb.message_names(0)=="RetObj") {
                    Protocol::RetObj ro;
                    if (ro.ParseFromString(rmb.message_arguments(0))) {
                        if (ro.has_object_reference()) {
                            newRef=ObjectReference(ro.object_reference());
                            std::map<UUID,TemporaryStreamData>::iterator where=mTemporaryStreams.find(hdr.destination_object().getAsUUID());
                            if (where!=mTemporaryStreams.end()) {
                                Network::Stream*stream=where->second.mStream;
                                if (mActiveStreams.find(newRef.getAsUUID())!=mActiveStreams.end()) {
                                    SILOG(space,error,"Untested: replacing extant stream "<<newRef.toString()<<" when trying to create obj with UUID "<<hdr.destination_object());
                                    std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator awhere=mActiveStreams.find(newRef.getAsUUID());
                                    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator swhere=mStreams.find(awhere->second);
                                    if (swhere!=mStreams.end()) {
                                        if (swhere->first!=stream) {
                                            delete swhere->first;
                                            mStreams.erase(swhere);
                                        }else {
                                            SILOG(space,warning,"Duplicate request to add "<<hdr.destination_object().toString()<<":"<<newRef.toString());
                                        }
                                    }else {
                                        SILOG(space,error,"FATAL: item in UUID map not in mStreams map "<<hdr.destination_object().toString());
                                    }
                                }
                                mStreams[stream].setConnected();
                                std::vector<Network::Chunk> pendingMessages;
                                where->second.mPendingMessages.swap(pendingMessages);
                                mActiveStreams[newRef.getAsUUID()]=stream;
                                mTemporaryStreams.erase(where);
                                for (std::vector<Network::Chunk>::iterator i=pendingMessages.begin(),
                                         ie=pendingMessages.end();
                                     i!=ie;
                                     ++i) {
                                    bytesReceivedCallback(stream,*i);
                                }
                                return true;
                            }else {
                                if (mActiveStreams.find(newRef.getAsUUID())!=mActiveStreams.end()) {
                                    SILOG(space,error,"Duplicate addition of "<<hdr.destination_object().toString()<<" as "<<newRef.toString());
                                    return true;
                                }else {
                                    forgeDisconnectionMessage(newRef);
                                    SILOG(space,error,"Connection disconnected during registration process "<<hdr.destination_object().toString());
                                    return false;
                                }
                            }
                        }else {
                            SILOG(space,warning,"UUID returned no object in making new object "<<hdr.destination_object().toString());
                        }
                    }else {
                        SILOG(space,warning,"Cannot parse RetObj in making new object "<<hdr.destination_object().toString());
                    }
                }else if (rmb.message_names(0)=="DelObj") {
                    return false;
                }
            }else {
                SILOG(space,warning,"0 message arguments to new object stream "<<hdr.destination_object().toString());
            }
        }else {
            SILOG(space,warning,"Cannot parse MessageBody body_data in making new object "<<hdr.destination_object().toString());
        }
        newRef=hdr.destination_object();
        return true;
    }
    SILOG(space,warning,"null destination object for new object reference");
    return true;
}
void ObjectConnections::processExistingObject(const RoutableMessageHeader&const_hdr,MemoryReference body_array, bool forward){
    if (const_hdr.has_destination_object()) {
        std::tr1::unordered_map<UUID,Network::Stream*,ObjectReference::Hasher>::iterator where;
        where=mActiveStreams.find(const_hdr.destination_object().getAsUUID());
        if (where==mActiveStreams.end()&&forward) {
            mSpace->processMessage(const_hdr,body_array);
        }else {
            RoutableMessageHeader hdr(const_hdr);
            std::string header_data;
            hdr.clear_destination_object();//no reason to waste bytes
            hdr.SerializeToString(&header_data);
            where->second->send(MemoryReference(header_data),body_array,Network::ReliableOrdered);//can this be unordered?
        }
    }else {
        SILOG(space,warning, "null destination object for message routing with source "<<const_hdr.source_object().toString());
    }
}
}
