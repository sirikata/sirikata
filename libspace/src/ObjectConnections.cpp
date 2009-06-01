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
    mPerObjectTemporarySizeMaximum=8192;
    mPerObjectTemporaryNumMessagesMaximum=64;
    Protocol::SpaceServices svc;
    svc.set_pre_connection_buffer(mPerObjectTemporarySizeMaximum);
    svc.set_max_pre_connection_messages(mPerObjectTemporaryNumMessagesMaximum);
    svc.AppendToString(&mSpaceServiceIntroductionMessage);
    mListener=listener;
    mRegistrationService=registrationServiceIdentifier;
    using std::tr1::placeholders::_1;    using std::tr1::placeholders::_2;
    mListener->listen(listenAddress,
                      std::tr1::bind(&ObjectConnections::newStreamCallback,this,_1,_2));
}
void ObjectConnections::newStreamCallback(Network::Stream*stream, Network::Stream::SetCallbacks&callbacks) {
    if (stream!=NULL){
        UUID temporaryId=UUID::random();//generate a random ID as a placeholder ObjectReference until Registration service calls back with RetObj
        mStreams[stream].setId(temporaryId);//set it for the streams (mStreams is set to disconnected by default)
        mTemporaryStreams[temporaryId].mStream=stream;//record this stream to the mTemporaryStreams
        using std::tr1::placeholders::_1;    using std::tr1::placeholders::_2;
        callbacks(std::tr1::bind(&ObjectConnections::connectionCallback,this,stream,_1,_2),
                  std::tr1::bind(&ObjectConnections::bytesReceivedCallback,this,stream,_1));
    }else{
        //whole object host has disconnected
    }
}
void ObjectConnections::bytesReceivedCallback(Network::Stream*stream, const Network::Chunk&chunk) {
    //find the temporary stream ID and connected boolean
    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.find(stream);
    RoutableMessageHeader hdr;
    MemoryReference chunkRef(chunk);//parse header
    MemoryReference message_body=hdr.ParseFromArray(chunkRef.data(),chunkRef.size());
    //munge header to reflect known ID
    hdr.set_source_object(ObjectReference(where->second.uuid()));
    if (((!hdr.has_destination_object())||hdr.destination_object()==ObjectReference::null())&&message_body.size()==0) {
        //if our message is size 0 and header nowhere or to null(), assume it's the object host request for service addresses
        stream->send(MemoryReference(mSpaceServiceIntroductionMessage),Network::ReliableOrdered);//send the tuned packet with all information needed to know services
    }else if (hdr.has_destination_object()&&hdr.destination_object()==mRegistrationService) {
        //this is either a NewObj or DelObj request Parse the body to find out
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
            if (connection) {
                where->second.setConnecting();
            }
            //let a connection request through to the registration service
            if (connection||where->second.connected()) {
                mSpace->processMessage(hdr,MemoryReference(message_body));
            }else {//push other requests for registration to the queue
                std::map<UUID,TemporaryStreamData>::iterator twhere=mTemporaryStreams.find(where->second.uuid());
                if (twhere!=mTemporaryStreams.end()) {//find the queue on which the request should live
                    if (twhere->second.mTotalMessageSize+chunk.size()<mPerObjectTemporarySizeMaximum
                        &&twhere->second.mPendingMessages.size()<mPerObjectTemporaryNumMessagesMaximum) {//if message queue has space
                        twhere->second.mTotalMessageSize+=chunk.size();//annotate size
                        twhere->second.mPendingMessages.push_back(chunk);//push back to array
                    }
                }else{
                    SILOG(space,warning,"Dropping message from "<<where->second.uuid().toString()<<" due to already disconnected object");
                }
            }
            if (disconnection) {//do not let further messages through to space if the object is disconnected
                where->second.setDisconnected();
                //don't disconnect stream immediately, should get message back from disconnection service
            }
        }
    } else if (where->second.connected()) {//ordinary message to connected object
        mSpace->processMessage(hdr,MemoryReference(message_body));                
    } else {//Not sure if we should verify that a connection request is going through,
            // or if we should just find the size of bytes saved and cap that reasonably 
            // this check would have verified a good faith effort to start connecting if (where->second.isConnecting()) {
        std::map<UUID,TemporaryStreamData>::iterator twhere=mTemporaryStreams.find(where->second.uuid());
        if (twhere!=mTemporaryStreams.end()) {
            if (twhere->second.mTotalMessageSize<mPerObjectTemporarySizeMaximum
                &&twhere->second.mPendingMessages.size()<mPerObjectTemporaryNumMessagesMaximum) {//if message queue has space
                twhere->second.mTotalMessageSize+=chunk.size();//annotate size
                twhere->second.mPendingMessages.push_back(chunk);//push back to array
            }
        }else{
            SILOG(space,warning,"Dropping message from "<<where->second.uuid().toString()<<" due to already disconnected object");
        }
    }
}
void ObjectConnections::forgeDisconnectionMessage(const ObjectReference&ref) {
    RoutableMessage rm;
    Protocol::DelObj delObj;
    rm.header().set_destination_object(mRegistrationService);//pretend object has contacted registration service
    rm.header().set_source_object(ref);//and has appropriately set its identifier
    rm.body().add_message_names("DelObj");//with one purpose: to delete itself
    rm.body().add_message_arguments(NULL,0);    //and serialize the deleted object to the strong
    delObj.SerializeToString(&rm.body().message_arguments(0));
    std::string serialized_message_body;
    rm.body().SerializeToString(&serialized_message_body);
    mSpace->processMessage(rm.header(),MemoryReference(serialized_message_body));//tell the space to forward the message to the registration service
}
void ObjectConnections::connectionCallback(Network::Stream*stream, Network::Stream::ConnectionStatus status, const std::string&reason){
    if (status!=Network::Stream::Connected) {
        SILOG(space,debug,"Connection lost "<<reason);//log connection lost
        std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.find(stream);//find active stream
        if (where!=mStreams.end()) {
            std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator uwhere=mActiveStreams.find(where->second.uuid());
            std::map<UUID,TemporaryStreamData>::iterator twhere;
            if (uwhere!=mActiveStreams.end()) {
                if (where->second.connected()) {//As soon as discon message detected, stream is disconnected, so must have had no disconnect message, hence send forged disconnect
                    forgeDisconnectionMessage(ObjectReference(uwhere->first));              
                }
                mActiveStreams.erase(uwhere);//erase the active stream
            }else if ((twhere=mTemporaryStreams.find(where->second.uuid()))!=mTemporaryStreams.end()) {
                mTemporaryStreams.erase(twhere);//erase the temporary stream, destroying all pending messages
            }else {
                SILOG(space,error,"Stream with unknown reference "<<where->second.uuid().toString());
            }
            mStreams.erase(where);//delete stream from record of active streams
        }else {
            SILOG(space,error,"Stream not found "<<reason);
        }
        delete stream;//delete the stream pointer since all references to it have been waxed (aside from callbacks--which is ok due to single-threaded assumption))
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
    if (ref&&*ref==mRegistrationService) {//message from registration service
        if (!hdr.has_source_object())
            hdr.set_source_object(*ref);
        if (processNewObject(hdr,body_array, newRef)) {//it could be a new object
            hdr.set_destination_object(newRef);//it is a new object, complete with permanent ObjectReference
            ref=&newRef;
        }else {
            disconnectionAttempt=true;//it was a forcable disconnection (potentially at user request)
        }
    }else {
        if (ref&&!hdr.has_destination_object()) {
            hdr.set_destination_object(*ref);
        }
        if (hdr.has_source_object()&&hdr.source_object()==mRegistrationService) {//repeated logic if registration was set in packet
            if (processNewObject(hdr,body_array,newRef)) {
                hdr.set_destination_object(newRef);
                ref=&newRef;
            }else {
                disconnectionAttempt=true;
            }
        }
    }
    processExistingObject(hdr,body_array,!disconnectionAttempt);//process the message but do not forward if a disconection attemp was made
    if (disconnectionAttempt) {
        //Server ban of user or confirm of disconnect
        shutdownConnection(hdr.destination_object());//destroy stream if destination object was forcably removed by space
    }

}
void ObjectConnections::processMessage(const RoutableMessageHeader&header,
                    MemoryReference message_body){
    RoutableMessageHeader newHeader;
    const RoutableMessageHeader *hdr=&header;
    bool disconnectionAttempt=false;
    if (header.source_object()==mRegistrationService) {//message from registration service
        ObjectReference newRef;
        if (processNewObject(header,message_body,newRef)) {//it could be a new object
            newHeader=header;
            newHeader.set_destination_object(newRef);//it is a new object complete with a permanent ObjectReference...make a new Header
            hdr=&newHeader;//set the hdr pointer to the new header
        }else {
            disconnectionAttempt=true;//it was a forcable disconnection (potentially at user request)
        }
    }
    processExistingObject(*hdr,message_body,!disconnectionAttempt);//process the message but do not forward a disconnection attempt if stream is nonexistant
    if (disconnectionAttempt) {//Server ban of user, or confirm of disconnect
        shutdownConnection(header.destination_object());//destroy stream if destination object was forcably removed by space
    }
}

void ObjectConnections::shutdownConnection(const ObjectReference&ref) {
    Network::Stream*stream=NULL;
    std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator awhere=mActiveStreams.find(ref.getAsUUID());//find uuid in active streams
    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.end();
    if (awhere!=mActiveStreams.end()){
        stream=awhere->second;//if it was an active stream, erase it
        where=mStreams.find(stream);
        mActiveStreams.erase(awhere);
    }else {
        std::map<UUID,TemporaryStreamData>::iterator twhere=mTemporaryStreams.find(where->second.uuid());       //ok maybe it's a temporary stream
        if (twhere!=mTemporaryStreams.end()) {
            stream=twhere->second.mStream;//well erase it to avoid dangling references
            where=mStreams.find(stream);
            mTemporaryStreams.erase(twhere);//but this is a critical error: the registration service should not know about this
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
    if (hdr.has_destination_object()) {//destination_object is the temporary UUID given for pending connecting objects
        if (rmb.ParseFromArray(body_array.data(),body_array.size())) {
            if (rmb.message_arguments_size()) {
                if (rmb.message_names(0)=="RetObj") {//make sure the name of the message is RetObj so it contains the ID
                    Protocol::RetObj ro;
                    if (ro.ParseFromString(rmb.message_arguments(0))) {//grab the message argument
                        if (ro.has_object_reference()) {
                            newRef=ObjectReference(ro.object_reference());//get the new reference
                            std::map<UUID,TemporaryStreamData>::iterator where=mTemporaryStreams.find(hdr.destination_object().getAsUUID());
                            if (where!=mTemporaryStreams.end()) {//a currently existing temporary stream
                                Network::Stream*stream=where->second.mStream;
                                if (mActiveStreams.find(newRef.getAsUUID())!=mActiveStreams.end()) {//new stream connection trying to replace existing stream
                                    //we may eventually want to allow this kind of behavior to allow object replication
                                    //but then we would need to upgrade our unordered_maps into unordered_multimap
                                    SILOG(space,error,"Untested: replacing extant stream "<<newRef.toString()<<" when trying to create obj with UUID "<<hdr.destination_object());
                                    std::tr1::unordered_map<UUID,Network::Stream*,UUID::Hasher>::iterator awhere=mActiveStreams.find(newRef.getAsUUID());
                                    std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator swhere=mStreams.find(awhere->second);
                                    if (swhere!=mStreams.end()) {//find the stream belonging to the old ObjectReference
                                        if (swhere->first!=stream) {
                                            delete swhere->first;//nuke it
                                            mStreams.erase(swhere);
                                        }else {//must be two existing requests...this could be an indication of a leak--FIXME
                                            SILOG(space,error,"FATAL: Duplicate request to add "<<hdr.destination_object().toString()<<":"<<newRef.toString()<<" ...leaking");
                                        }
                                    }else {//no stream in stream map... cannot replace both copies of Stream* .  Should not reach this code
                                        SILOG(space,error,"FATAL: item in UUID map not in mStreams map "<<hdr.destination_object().toString());
                                    }
                                }
                                StreamMapUUID* iter=&mStreams[stream];
                                iter->setConnected();
                                iter->setDoneConnecting();
                                iter->setId(newRef.getAsUUID());//set the id of the stream map to the permanent ObjetReference
                                std::vector<Network::Chunk> pendingMessages;
                                where->second.mPendingMessages.swap(pendingMessages);//get ready to send pending messages
                                where->second.mTotalMessageSize=0;//reset bytes used
                                mActiveStreams[newRef.getAsUUID()]=stream;//setup mStream and 
                                mTemporaryStreams.erase(where);//erase temporary reference to stream
                                for (std::vector<Network::Chunk>::iterator i=pendingMessages.begin(),
                                         ie=pendingMessages.end();
                                     i!=ie;
                                     ++i) {
                                    bytesReceivedCallback(stream,*i);//process pending messages as if they were just received
                                }
                                return true;//new object ready to use
                            }else {
                                if (mActiveStreams.find(newRef.getAsUUID())!=mActiveStreams.end()) {//for some reason the registration service gave us another active registration
                                    //FIXME: this error case is potentially problematic
                                    mStreams[mActiveStreams[newRef.getAsUUID()]].setDoneConnecting();//make sure to mark the stream as done connecting
                                    SILOG(space,error,"Duplicate addition of "<<hdr.destination_object().toString()<<" as "<<newRef.toString());
                                    return true;
                                }else {
                                    forgeDisconnectionMessage(newRef);//Normal occurance: the stream has been disconnected while registration system got its act together
                                                                      //forge a disconnection message to purge it from other systems
                                    SILOG(space,warning,"Connection disconnected during registration process "<<hdr.destination_object().toString());
                                    return false;//return the object is deleted instead and not some new object
                                }
                            }
                        }else {
                            SILOG(space,warning,"UUID returned no object in making new object "<<hdr.destination_object().toString());
                        }
                    }else {
                        SILOG(space,warning,"Cannot parse RetObj in making new object "<<hdr.destination_object().toString());
                    }
                }else if (rmb.message_names(0)=="DelObj") {//it's a deletion stream... return that the object is deleted and not some new object
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
    SILOG(space,error,"null destination object for new object reference");//should not get here
    return true;
}
void ObjectConnections::processExistingObject(const RoutableMessageHeader&const_hdr,MemoryReference body_array, bool forward){
    if (const_hdr.has_destination_object()) {//only process if valid destination
        std::tr1::unordered_map<UUID,Network::Stream*,ObjectReference::Hasher>::iterator where;
        where=mActiveStreams.find(const_hdr.destination_object().getAsUUID());//find UUID from active streams
        if (where==mActiveStreams.end()&&forward) {//if this is not a message from the space, then forward it to the space for future forwarding
            mSpace->processMessage(const_hdr,body_array);
        }else {
            RoutableMessageHeader hdr(const_hdr);//send it to the found stream
            std::string header_data;
            hdr.clear_destination_object();//no reason to waste bytes
            hdr.SerializeToString(&header_data);//serialize then send out
            where->second->send(MemoryReference(header_data),body_array,Network::ReliableOrdered);//FIXME can this be unordered?
        }
    }else {
        SILOG(space,warning, "null destination object for message routing with source "<<const_hdr.source_object().toString());
    }
}
}
