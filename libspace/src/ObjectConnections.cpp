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
#include "util/KnownServices.hpp"
#include "space/Registration.hpp"
#include "space/ObjectConnections.hpp"
namespace Sirikata {
ObjectConnections::ObjectConnections(Network::StreamListener*listener,
                                     const Network::Address&listenAddress) {
    
    //mSpaceServiceIntroductionMessage=introductoryMessage;
    mSpace=NULL;
    mPerObjectTemporarySizeMaximum=8192;
    mPerObjectTemporaryNumMessagesMaximum=64;
//    Protocol::SpaceServices svc;
//    svc.set_pre_connection_buffer(mPerObjectTemporarySizeMaximum);
//    svc.set_max_pre_connection_messages(mPerObjectTemporaryNumMessagesMaximum);
//    svc.AppendToString(&mSpaceServiceIntroductionMessage);
    mListener=listener;
    using std::tr1::placeholders::_1;    using std::tr1::placeholders::_2;
    mListener->listen(listenAddress,
                      std::tr1::bind(&ObjectConnections::newStreamCallback,this,_1,_2));
}
void ObjectConnections::newStreamCallback(Network::Stream*stream, Network::Stream::SetCallbacks&callbacks) {
    if (stream!=NULL){
        UUID temporaryId=UUID::random();//generate a random ID as a placeholder ObjectReference until Registration service calls back with RetObj
        mStreams[stream].setId(temporaryId);//set it for the streams (mStreams is set to disconnected by default)
        TemporaryStreamData data;
        data.mStream=stream;
        mTemporaryStreams.insert(TemporaryStreamMultimap::value_type(temporaryId,data));//record this stream to the mTemporaryStreams
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
    if (false&&((!hdr.has_destination_object())||hdr.destination_object()==ObjectReference::null())&&message_body.size()==0) {
        //if our message is size 0 and header nowhere or to null(), assume it's the object host request for service addresses
        stream->send(MemoryReference(mSpaceServiceIntroductionMessage),Network::ReliableOrdered);//send the tuned packet with all information needed to know services
    }else if (hdr.has_destination_object()&&hdr.destination_object()==ObjectReference::spaceServiceID()&&hdr.destination_port()==Services::REGISTRATION) {
        //this is a NewObj request Parse the body to find out
        RoutableMessageBody rmb;
        bool success=rmb.ParseFromArray(message_body.data(),message_body.size());
        Protocol::RetObj ro;
        if (success) {
            bool connection=false;
            int i;
            for (i=0;i<rmb.message_arguments_size();++i){
                connection=connection||(rmb.message_names(i)=="NewObj");
            }
            if (connection) {
                where->second.setConnecting();
            }
            //let a connection request through to the registration service
            if (connection||where->second.connected()) {
                mSpace->processMessage(hdr,MemoryReference(message_body));
            }else {//push other requests for registration to the queue
                TemporaryStreamMultimap::iterator twhere=mTemporaryStreams.find(where->second.uuid());
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
        }
    } else if (where->second.connected()) {//ordinary message to connected object
        mSpace->processMessage(hdr,MemoryReference(message_body));                
    } else {//Not sure if we should verify that a connection request is going through,
            // or if we should just find the size of bytes saved and cap that reasonably 
            // this check would have verified a good faith effort to start connecting if (where->second.isConnecting()) {
        TemporaryStreamMultimap::iterator twhere=mTemporaryStreams.find(where->second.uuid());
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
    rm.header().set_destination_object(ObjectReference::spaceServiceID());//pretend object has contacted registration service
    rm.header().set_destination_port(Services::REGISTRATION);    
    rm.header().set_source_object(ObjectReference::spaceServiceID());//and has appropriately set its identifier
    rm.header().set_source_port(Services::OBJECT_CONNECTIONS);
    rm.body().add_message_names("DelObj");//with one purpose: to delete itself
    rm.body().add_message_arguments(NULL,0);    //and serialize the deleted object to the strong
    delObj.set_object_reference(ref.getAsUUID());
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
            StreamMap::iterator uwhere=mActiveStreams.find(where->second.uuid());
            TemporaryStreamMultimap::iterator twhere;
            StreamSet::iterator stream_set_iterator;
            if (uwhere!=mActiveStreams.end()&&(stream_set_iterator=std::find(uwhere->second.begin(),uwhere->second.end(),stream))!=uwhere->second.end()) {
                if (uwhere->second.size()==1&&where->second.connected()) {//As soon as discon message detected, stream is disconnected, so must have had no disconnect message, hence send forged disconnect
                    forgeDisconnectionMessage(ObjectReference(uwhere->first));              
                    mActiveStreams.erase(uwhere);//erase the active stream
                }else {
                    uwhere->second.erase(stream_set_iterator);
                }
            }else if ((twhere=mTemporaryStreams.find(where->second.uuid()))!=mTemporaryStreams.end()) {
                for (;twhere!=mTemporaryStreams.end()&&twhere->first==where->second.uuid();++twhere) {
                    if (twhere->second.mStream==stream) {
                        mTemporaryStreams.erase(twhere);//erase the temporary stream, destroying all pending messages
                    }
                }
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
    StreamMap::iterator where=mActiveStreams.find(ref.getAsUUID());
    if (where==mActiveStreams.end()) 
        return NULL;
    double percent=((double)rand())/(RAND_MAX+1);
    if (where->second.empty()) {
        SILOG(space,error,"Empty connection vector in object streams");
        mActiveStreams.erase(where);
        return NULL;
    }
    return where->second[((size_t)(percent*where->second.size()))%where->second.size()];
}

Network::Stream* ObjectConnections::temporaryConnectionTo(const UUID&ref) {
    TemporaryStreamMultimap::iterator where=mTemporaryStreams.find(ref);
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

void ObjectConnections::processMessage(const RoutableMessageHeader&header,
                    MemoryReference message_body){
    RoutableMessageHeader newHeader;
    const RoutableMessageHeader *hdr=&header;
    bool disconnectionAttempt=false;
    if (header.has_source_object()&&header.source_object()==ObjectReference::spaceServiceID()&&header.source_port()==Services::REGISTRATION) {//message from registration service
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
    StreamMap::iterator awhere=mActiveStreams.find(ref.getAsUUID());//find uuid in active streams
    if (awhere!=mActiveStreams.end()){
        if (awhere->second.size()>1) {
            for (StreamSet::iterator i=awhere->second.begin(),ie=awhere->second.end();i!=ie;++i) {
                std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where=mStreams.find(*i);
                if (where!=mStreams.end()) {
                    mStreams.erase(where);
                }
                delete *i;
            }
        }
        mActiveStreams.erase(awhere);
    }else {
        UUID uuid=ref.getAsUUID();
        TemporaryStreamMultimap::iterator twhere=mTemporaryStreams.find(uuid);       //ok maybe it's a temporary stream
        if (twhere!=mTemporaryStreams.end()) {
            for (;twhere!=mTemporaryStreams.end()&&twhere->first==uuid;++twhere) {
                std::tr1::unordered_map<Network::Stream*,StreamMapUUID>::iterator where;
                
                stream=twhere->second.mStream;//well erase it to avoid dangling references
                where=mStreams.find(stream);
                mTemporaryStreams.erase(twhere);//but this is a critical error: the registration service should not know about this
                SILOG(space,error,"FATAL: Stream connected yet found in temporary streams" << where->second.uuid().toString());
                if (where!=mStreams.end()) {
                    mStreams.erase(where);
                }
                delete stream;
            }
        }else {
            SILOG(space,warning,"Cannot find stream to shutdown for object reference "<<ref.toString());
        }
    }
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
                            UUID uuid=hdr.destination_object().getAsUUID();
                            TemporaryStreamMultimap::iterator start,where=mTemporaryStreams.find(uuid);
                            if (where!=mTemporaryStreams.end()) {//a currently existing temporary stream
                                start=where;//messages are always added to first mTemporaryStream, so go from back to front when sending them out                                
                                for (;where!=mTemporaryStreams.end()&&where->first==uuid;++where) {
                                }
                                Network::Stream*stream=NULL;
                                std::vector<Network::Chunk> pendingMessages;
                                std::vector<std::pair<Network::Stream*,Network::Chunk> > taggedPendingMessages;
                                do  {
                                    StreamMapUUID* iter=&mStreams[stream];
                                    iter->setConnected();
                                    iter->setDoneConnecting();
                                    iter->setId(newRef.getAsUUID());//set the id of the stream map to the permanent ObjetReference
                                    if (pendingMessages.empty()) {
                                        stream=where->second.mStream;

                                        where->second.mPendingMessages.swap(pendingMessages);//get ready to send pending messages
                                    }else {
                                        for (std::vector<Network::Chunk>::iterator i=where->second.mPendingMessages.begin(),ie=where->second.mPendingMessages.end();i!=ie;++i) {
                                            std::pair<Network::Stream*,Network::Chunk> newChunk;
                                            newChunk.first=where->second.mStream;
                                        
                                            
                                            taggedPendingMessages.push_back(newChunk);
                                            taggedPendingMessages.back().second.swap(*i);
                                        }
                                    }
                                    where->second.mTotalMessageSize=0;//reset bytes used
                                    mActiveStreams[newRef.getAsUUID()].push_back(stream);//setup mStream and 
                                    --where;
                                }while (where!=start);
                                mTemporaryStreams.erase(start);
                                while ((where=mTemporaryStreams.find(uuid))!=mTemporaryStreams.end()) {
                                    mTemporaryStreams.erase(where);
                                }
                                for (std::vector<Network::Chunk>::iterator i=pendingMessages.begin(),
                                         ie=pendingMessages.end();
                                     i!=ie;
                                     ++i) {
                                    bytesReceivedCallback(stream,*i);//process pending messages as if they were just received
                                }
                                for (std::vector<std::pair<Network::Stream*,Network::Chunk> >::iterator i=taggedPendingMessages.begin(),
                                         ie=taggedPendingMessages.end();
                                     i!=ie;
                                     ++i) {
                                    bytesReceivedCallback(i->first,i->second);//process pending messages as if they were just received
                                }
                                return true;//new object ready to use
                            }else {
                                StreamMap::iterator replicatedObject=mActiveStreams.find(newRef.getAsUUID());
                                if (replicatedObject!=mActiveStreams.end()) {//for some reason the registration service gave us another active registration
                                    //FIXME: this error case is potentially problematic
                                    for (StreamSet::iterator i=replicatedObject->second.begin(),ie=replicatedObject->second.end();i!=ie;++i) {
                                        mStreams[*i].setDoneConnecting();//make sure to mark the stream as done connecting
                                    }
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
        StreamMap::iterator where;
        where=mActiveStreams.find(const_hdr.destination_object().getAsUUID());//find UUID from active streams
        if (where==mActiveStreams.end()&&forward) {//if this is not a message from the space, then forward it to the space for future forwarding
            mSpace->processMessage(const_hdr,body_array);
        }else {
            RoutableMessageHeader hdr(const_hdr);//send it to the found stream
            std::string header_data;
            hdr.clear_destination_object();//no reason to waste bytes
            hdr.SerializeToString(&header_data);//serialize then send out
            double percent=((double)rand())/(RAND_MAX+1);
            if (where->second.empty()) {
                SILOG(space,error,"Somehow got empty object connection stream.");
                mActiveStreams.erase(where);
            }else {
                where->second[((size_t)(percent*where->second.size()))%where->second.size()]->send(MemoryReference(header_data),body_array,Network::ReliableOrdered);//FIXME can this be unordered?
            }
        }
    }else {
        SILOG(space,warning, "null destination object for message routing with source "<<const_hdr.source_object().toString());
    }
}
}
