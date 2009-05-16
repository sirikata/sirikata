/*  Sirikata Proximity Management -- Introduction Services
 *  ObjectSpaceBridgeProximitySystem.hpp
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
#ifndef _PROXIMITY_OBJECTSPACEBRIDGEPROXIMITYSYSTEM_HPP_
#define _PROXIMITY_OBJECTSPACEBRIDGEPROXIMITYSYSTEM_HPP_
namespace Sirikata { namespace Proximity {

/**
 * This proximity system allows an object to use the normal space communication system 
 * to merely forward the messages through the space to the destination proximity system.
 * It largely translates the function calls into a Protocol::Message and uses whatever
 * routing mechanism is present to forward those messages along.
 * Does not wish to be object implementation dependent so it uses that as a template argument
 */
template <class ObjectPtr> 
class ObjectSpaceBridgeProximitySystem : public ProximitySystem{
    ObjectPtr mObject;
protected:
    static const char* proxCallbackName() {
        return "ProxCallback";
    }
    virtual bool forwardThisName(const std::string&name) {
        return name=="NewProxQuery"||name==proxCallbackName()||name=="DelProxQuery";
    }
    enum DidAlterMessage {
        ALTERED_MESSAGE,
        UNSPOILED_MESSAGE
      
    };
    virtual DidAlterMessage addressMessage(Protocol::IMessage&output,
                                const ObjectReference*source,
                                const ObjectReference*destination) {
        return UNSPOILED_MESSAGE;
        if (0) {//you might want to do this if you are a forwarding class or something more advanced
            if (source)
                output.set_source_object(source->getObjectUUID());
            if (destination)
                output.set_destination_object(destination->getObjectUUID());
        }
    }
    template <typename T> void constructMessage(Protocol::Message&output,
                                                const ObjectReference*source,
                                                const ObjectReference*destination,
                                                const char* messageName,
                                                const T&opaqueMessage,
                                                const void *optionalSerializedSubMessage,
                                                size_t optionalSerializedSubMessageSize) {
        DidAlterMessage alteration=addressMessage(output,source,destination);
        output.add_message_names(messageName);
        if (optionalSerializedSubMessage&&optionalSerializedSubMessageSize) {
            output.add_message_arguments(optionalSerializedSubMessage,optionalSerializedSubMessageSize);
        }else {
            int which=output.message_arguments_size();
            output.add_message_arguments(std::string());
            opaqueMessage.SerializeToString(&output.message_arguments(which));
        }
    }
    virtual void deliverMessage(const ObjectReference& destination,
                                const Protocol::IMessage&opaqueMessage,
                                const void *optionalSerializedMessage,
                                size_t optionalSerializedMessageSize) {
        mObject->deliver(opaqueMessage,optionalSerializedMessage,optionalSerializedMessageSize);
    }
    virtual void sendMessage(const ObjectReference& source,
                             const Protocol::IMessage&opaqueMessage,
                             const void *optionalSerializedMessage,
                             size_t optionalSerializedMessageSize) {
        mObject->send(opaqueMessage,optionalSerializedMessage,optionalSerializedMessageSize);
    }

public:
    ObjectSpaceBridgeProximitySystem (ObjectPtr obj) {
        mObject=obj;
    }
    enum MessageBundle{
        DELIVER_TO_UNKNOWN,
        DELIVER_TO_OBJECT,
        DELIVER_TO_PROX,
        DELIVER_TO_BOTH
        
    };

    /**
     * Process a message that may be meant for the proximity system
     * \returns true if object was deleted
     */
    virtual OpaqueMessageReturnValue processOpaqueProximityMessage(std::vector<ObjectReference>&newObjectReferences,
                                               const ObjectReference*object,
                                               const Sirikata::Protocol::IMessage& mesg,
                                               const void *optionalSerializedMessage=NULL,
                                               size_t optionalSerializedMessageSize=0) {
        Protocol::Message msg(mesg);
        MessageBundle sendState=DELIVER_TO_UNKNOWN;
        bool deliverAllMessages=true;
        int len=msg.message_names_size();
        OpaqueMessageReturnValue obj_is_deleted=OBJECT_NOT_DESTROYED;
        for (int i=0;i<len;++i) {
            if(msg.message_names(i)=="DelObj")
                obj_is_deleted=OBJECT_DELETED;
            if (!forwardThisName(msg.message_names(i))) {
                if (len==1) return obj_is_deleted;
                deliverAllMessages=false;
            }else {
                if (msg.message_names(i)==proxCallbackName()) {
                    if (sendState==DELIVER_TO_UNKNOWN||sendState==DELIVER_TO_OBJECT)
                        sendState=DELIVER_TO_OBJECT;
                    else 
                        sendState=DELIVER_TO_BOTH;
                }else{
                    if (sendState==DELIVER_TO_UNKNOWN||sendState==DELIVER_TO_PROX)
                        sendState=DELIVER_TO_PROX;
                    else 
                        sendState=DELIVER_TO_BOTH;
                }
            }
        }
        if (sendState==DELIVER_TO_UNKNOWN)
            return obj_is_deleted;//no messages considered worth forwarding to the proximity system
        if (sendState==DELIVER_TO_BOTH) {
            SILOG(prox,info,"Message with recipients both proximity and object bundled into the same message: not delivering.");
            return obj_is_deleted;
        }
/* NOT SURE THIS IS NECESSARY -- do these messages already have addresses on them?*/
        DidAlterMessage alteration;
        if (sendState==DELIVER_TO_PROX) 
            alteration=addressMessage(msg,object,NULL);
        else
            alteration=addressMessage(msg,NULL,object);
        if (deliverAllMessages&&sendState==DELIVER_TO_PROX) {
            sendMessage(*object,msg,alteration==UNSPOILED_MESSAGE?optionalSerializedMessage:NULL,alteration==UNSPOILED_MESSAGE?optionalSerializedMessageSize:0);
        } else if (deliverAllMessages&&sendState==DELIVER_TO_OBJECT) {
            deliverMessage(*object,msg,alteration==UNSPOILED_MESSAGE?optionalSerializedMessage:NULL,alteration==UNSPOILED_MESSAGE?optionalSerializedMessageSize:0);
        }else {
            //some messages are not considered worth forwarding to the proximity system or there's a mishmash of destinations
            if (msg.message_arguments_size()<len) {
                len=msg.message_arguments_size();
            }
            Protocol::Message newMsg (msg);
            newMsg.clear_message_names();
            newMsg.clear_message_arguments();
            for (int i=0;i<len;++i) {
                if (forwardThisName(msg.message_names(i))) {
                    newMsg.add_message_names(newMsg.message_names(i));
                    newMsg.add_message_arguments(newMsg.message_arguments(i));
                }
            }
            if (sendState==DELIVER_TO_OBJECT)
                deliverMessage(*object,newMsg,NULL,0);
            if (sendState==DELIVER_TO_PROX)
                sendMessage(*object,newMsg,NULL,0);
            
        }
        return obj_is_deleted;
    }

    /**
     * Pass the ReturnedObjectConnection info, 
     * containing an Object UUID to the proximity manager, 
     * so the proximity system knows about a new object
     */
    virtual ObjectReference newObj(const Sirikata::Protocol::IRetObj&iro,
                        const void *optionalSerializedReturnObjectConnection=NULL,
                        size_t optionalSerializedReturnObjectConnectionSize=0){
        return ObjectReference(iro.object_reference());
        //does nothing since the space is forwarding proximity messages with this bridge
    }
    /**
     * Register a new proximity query.
     * The callback may come from an ASIO response thread
     */
    virtual void newProxQuery(const ObjectReference&source,
                              const Sirikata::Protocol::INewProxQuery&newProxQueryMsg, 
                              const void *optionalSerializedProximityQuery=NULL,
                              size_t optionalSerializedProximitySize=0){
        Protocol::Message toSend;
        constructMessage(toSend,&source,NULL,"NewProxQuery",newProxQueryMsg,optionalSerializedProximityQuery,optionalSerializedProximitySize);
        sendMessage(source,toSend,NULL,0);
    }
    /**
     * Since certain setups have proximity responses coming from another message stream
     * Those messages should be shunted to this function and processed
     */
    virtual void processProxCallback(const ObjectReference&destination,
                                     const Sirikata::Protocol::IProxCall&callInfo,
                                     const void *optionalSerializedProxCall=NULL,
                                     size_t optionalSerializedProxCallSize=0) {
        Protocol::Message delivery;
        constructMessage(delivery,NULL,&destination,"ProcessProxCallback",callInfo,optionalSerializedProxCall,optionalSerializedProxCallSize);
        deliverMessage(destination,delivery,NULL,0);
    }
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
    virtual void objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLocSize=0) {
        //does nothing since the space is forwarding proximity messages with this bridge
    }

    /**
     * Objects may lose interest in a particular query
     * when this function returns, no more responses will be given
     */
    virtual void delProxQuery(const ObjectReference&source, const Sirikata::Protocol::IDelProxQuery&delProx,  const void *optionalSerializedDelProxQuery=NULL,size_t optionalSerializedDelProxQuerySize=0){
        Protocol::Message toSend;
        constructMessage(toSend,&source,NULL,"DelProxQuery",delProx,optionalSerializedDelProxQuery,optionalSerializedDelProxQuerySize);
        sendMessage(source,toSend,NULL,0);
    }
    /**
     * Objects may be destroyed: indicate loss of interest here
     */
    virtual void delObj(const ObjectReference&source, const Sirikata::Protocol::IDelObj&, const void *optionalSerializedDelObj=NULL,size_t optionalSerializedDelObjSize=0) {
        //does nothing since the space is forwarding proximity messages with this bridge
    }
   
};

} }
#endif
