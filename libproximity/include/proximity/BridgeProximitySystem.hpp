/*  Sirikata Proximity Management -- Introduction Services
 *  BridgeProximitySystem.hpp
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

#ifndef _PROXIMITY_BRIDGEPROXIMITYSYSTEM_HPP_
#define _PROXIMITY_BRIDGEPROXIMITYSYSTEM_HPP_
#include "ObjectSpaceBridgeProximitySystem.hpp"
namespace Sirikata { namespace Proximity {
class BridgeProximitySystem : public ObjectSpaceBridgeProximitySystem<MessageService*> {
    class MulticastProcessMessage:public MessageService {
        std::vector<MessageService*>mMessageServices;
    public:
        virtual bool forwardMessagesTo(MessageService*ms){
            mMessageServices.push_back(ms);
            return true;
        }
        virtual bool endForwardingMessagesTo(MessageService*ms) {
            std::vector<MessageService*>::iterator where=std::find(mMessageServices.begin(),mMessageServices.end(),ms);
            if (where!=mMessageServices.end()) {
                mMessageServices.erase(where);
                return true;
            }
            return false;
        }
        /**
         * Process a message that may be meant for the space system
         */
        virtual void processMessage(const RoutableMessageHeader&hdr,
                                    MemoryReference message_body){
            for (std::vector<MessageService*>::iterator i=mMessageServices.begin(),ie=mMessageServices.end();i!=ie;++i) {
                (*i)->processMessage(hdr,message_body);
            }
        }
        
    }mMulticast;
protected:
    virtual bool forwardThisName(bool registration_or_disconnection, const std::string&name) {
        if (registration_or_disconnection) return true;
        return name=="ObjLoc"||this->ObjectSpaceBridgeProximitySystem<MessageService*>::forwardThisName(registration_or_disconnection,name);
    }

    virtual bool forwardMessagesTo(MessageService*ms){
        return mMulticast.forwardMessagesTo(ms);
    }
    virtual bool endForwardingMessagesTo(MessageService*ms) {
        return mMulticast.endForwardingMessagesTo(ms);
    }
    virtual ObjectSpaceBridgeProximitySystem<MessageService*>::DidAlterMessage addressMessage(RoutableMessageHeader&output,
                                const ObjectReference*source,
                                const ObjectReference*destination) {
        //if (source) //proximity management will have source through 
        //    output.set_source_object(source->getObjectUUID());
        if (destination) {
            output.set_destination_object(*destination);
            return ObjectSpaceBridgeProximitySystem<MessageService*>::ALTERED_MESSAGE;
        }
        return ObjectSpaceBridgeProximitySystem<MessageService*>::UNSPOILED_MESSAGE;
    }
    virtual void sendMessage(const ObjectReference&source,
                             const RoutableMessage&opaqueMessage,
                             const void *optionalSerializedMessageBody,
                             size_t optionalSerializedMessageBodySize) {
        if (optionalSerializedMessageBodySize) {
            RoutableMessageHeader hdr;
            hdr.set_source_object(source);
            mProximityConnection->processMessage(hdr,MemoryReference(optionalSerializedMessageBody,optionalSerializedMessageBodySize));
        }else {
            RoutableMessageHeader hdr;
            hdr.set_source_object(source);
            std::string data;
            opaqueMessage.body().SerializeToString(&data);
            mProximityConnection->processMessage(hdr,MemoryReference(data));
        }
    }
    virtual void readProximityMessage(const ObjectReference&objectConnection,Network::Chunk&msg) {
        if (msg.size()) {
            RoutableMessage message;
            MemoryReference ref=message.RoutableMessageHeader::ParseFromArray(&msg[0],msg.size());
            addressMessage(message,NULL,&objectConnection);
            //ObjectSpaceBridgeProximitySystem<MessageService*>::UNSPOILED_MESSAGE) {
            this->deliverMessage(objectConnection,message,ref.data(),ref.size());//FIXME: this deliver happens in a thread---do we need a distinction
        }
    }
public:
    enum {
        PORT=3
    };

    ProximityConnection *mProximityConnection;
    BridgeProximitySystem(ProximityConnection*connection,const unsigned int registrationPort) : ObjectSpaceBridgeProximitySystem<MessageService*>(&mMulticast,registrationPort),mProximityConnection(connection) {
        bool retval=mProximityConnection->forwardMessagesTo(this);
        assert(retval);
    }
    enum MessageBundle{
        DELIVER_TO_UNKNOWN,
        DELIVER_TO_OBJECT,
        DELIVER_TO_PROX,
        DELIVER_TO_BOTH
        
    };

    /**
     * Pass the ReturnedObjectConnection info, 
     * containing an Object UUID to the proximity manager, 
     * so the proximity system knows about a new object
     */
    virtual ObjectReference newObj(const Sirikata::Protocol::IRetObj& newObjMsg,
                        const void *optionalSerializedReturnObjectConnection=NULL,
                        size_t optionalSerializedReturnObjectConnectionSize=0){
        ObjectReference source(newObjMsg.object_reference());
        mProximityConnection->constructObjectStream(source);
        RoutableMessage toSend;
        this->constructMessage(toSend,&source,NULL,"RetObj",newObjMsg,optionalSerializedReturnObjectConnection,optionalSerializedReturnObjectConnectionSize);
        this->sendMessage(source,toSend,NULL,0);
        return source;
    }
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
    virtual void objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&objLocMsg, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLocSize=0) {
        RoutableMessage toSend;
        this->constructMessage(toSend,&source,NULL,"ObjLoc",objLocMsg,optionalSerializedObjLoc,optionalSerializedObjLocSize);
        this->sendMessage(source,toSend,NULL,0);
    }
    /**
     * Objects may be destroyed: indicate loss of interest here
     */
    virtual void delObj(const ObjectReference&source, const Sirikata::Protocol::IDelObj&delObjMsg, const void *optionalSerializedDelObj=NULL,size_t optionalSerializedDelObjSize=0) {
        RoutableMessage toSend;
        this->constructMessage(toSend,&source,NULL,"DelObj",delObjMsg,optionalSerializedDelObj,optionalSerializedDelObjSize);
        this->sendMessage(source,toSend,NULL,0);
        this->mProximityConnection->deleteObjectStream(source);
    }
    /**
     * Process a message that may be meant for the proximity system
     * \returns true if object was deleted
     */
    virtual void processMessage(const RoutableMessageHeader& mesg,
                                MemoryReference message_body) {
        if (this->internalProcessOpaqueProximityMessage(mesg.has_source_object()?&mesg.source_object():NULL,
                                                        mesg,
                                                        message_body.data(),
                                                        message_body.size(),false)==ProximitySystem::OBJECT_DELETED) {
            if (mesg.has_source_object())
                this->mProximityConnection->deleteObjectStream(mesg.source_object());
        }
    }
   
};

} }
#endif
