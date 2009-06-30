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
    unsigned int mRegistrationPort;
protected:
    static const char* proxCallbackName() {
        return "ProxCall";
    }
    virtual bool forwardThisName(bool registration_or_disconnection,const std::string&name) {        
        if (registration_or_disconnection) return false;
        return name==proxCallbackName()||name=="NewProxQuery"||name=="DelProxQuery";
    }
    enum DidAlterMessage {
        ALTERED_MESSAGE,
        UNSPOILED_MESSAGE
      
    };
    virtual DidAlterMessage addressMessage(RoutableMessageHeader&output,
                                const ObjectReference*source,
                                const ObjectReference*destination) {
        return UNSPOILED_MESSAGE;
        if (0) {//you might want to do this if you are a forwarding class or something more advanced
            if (source)
                output.set_source_object(*source);
            if (destination)
                output.set_destination_object(*destination);
        }
    }
    template <typename T> void constructMessage(RoutableMessage&output,
                                                const ObjectReference*source,
                                                const ObjectReference*destination,
                                                const char* messageName,
                                                const T&opaqueMessage,
                                                const void *optionalSerializedSubMessage,
                                                size_t optionalSerializedSubMessageSize) {
        DidAlterMessage alteration=addressMessage(output,source,destination);
        output.body().add_message_names(messageName);
        if (optionalSerializedSubMessage&&optionalSerializedSubMessageSize) {
            output.body().add_message_arguments(optionalSerializedSubMessage,optionalSerializedSubMessageSize);
        }else {
            int which=output.body().message_arguments_size();
            output.body().add_message_arguments(std::string());
            opaqueMessage.SerializeToString(&output.body().message_arguments(which));
        }
    }
    virtual void deliverMessage(const ObjectReference& destination,
                                const RoutableMessage&opaqueMessage,
                                const void *optionalSerializedMessageBody,
                                size_t optionalSerializedMessageBodySize) {
        if (optionalSerializedMessageBody) {
            mObject->processMessage(opaqueMessage.header(),MemoryReference(optionalSerializedMessageBody,optionalSerializedMessageBodySize));
        }else {
            std::string bodyMsg;
            if (opaqueMessage.body().SerializeToString(&bodyMsg)) {
                mObject->processMessage(opaqueMessage.header(),MemoryReference(bodyMsg.data(),bodyMsg.size()));
            }
        }
    }
    ///needs to be the following send message for  Objects, but not for spaces, so make it an abstract class for now and deal with sendMessage in subclass
    virtual void sendMessage(const ObjectReference& source,
                             const RoutableMessage&opaqueMessage,
                             const void *optionalSerializedMessageBody,
                             size_t optionalSerializedMessageBodySize)=0;/* {
        if (optionalSerializedMessageBody) {
            mObject->send(opaqueMessage.header(),
                          MemoryReference(optionalSerializedMessageBody,optionalSerializedMessageBodySize));
        }else {
            std::string bodyMsg;
            if (opaqueMessage.body().SerializeToString(&bodyMsg)) {
                mObject->send(opaqueMessage.header(),
                              MemoryReference(bodyMsg.data(),bodyMsg.size()));
            }
        }
        }*/
    /**
     * Process a message that may be meant for the proximity system
     * \returns true if object was deleted
     */
    OpaqueMessageReturnValue internalProcessOpaqueProximityMessage(
                                               const ObjectReference*object,
                                               const RoutableMessageHeader& hdr,
                                               const void *serializedMessageBody,
                                               size_t serializedMessageBodySize,
                                               bool trusted) {
        try {
            RoutableMessage msg(hdr,serializedMessageBody,serializedMessageBodySize);
            MessageBundle sendState=DELIVER_TO_UNKNOWN;
            bool deliverAllMessages=true;
            int len=msg.body().message_names_size();
            OpaqueMessageReturnValue obj_is_deleted=OBJECT_NOT_DESTROYED;
            bool disconnection=false;
            bool registration=false;
            for (int i=0;i<len;++i) {
                if (trusted||(hdr.has_source_object()&&hdr.source_object()==ObjectReference::spaceServiceID()&&hdr.source_port()==mRegistrationPort)) {
                    if(msg.body().message_names(i)=="DelObj") {
                        disconnection=true;
                        obj_is_deleted=OBJECT_DELETED;
                    }
                    if(msg.body().message_names(i)=="RetObj") {
                        registration=true;
                        int maxnum=i+1;
                        if (maxnum==len)
                            maxnum=msg.body().message_arguments_size();
                        for (int j=i;j<maxnum;++j){
                            Sirikata::Protocol::RetObj ro;
                            ro.ParseFromString(msg.body().message_arguments(j));
                            newObj(ro,msg.body().message_arguments(j).data(),msg.body().message_arguments(j).size());
                        }
                    }
                }
                if (!forwardThisName(registration||disconnection,msg.body().message_names(i))) {
                    if (len==1) return obj_is_deleted;
                    deliverAllMessages=false;
                }else {
                    if (msg.body().message_names(i)==proxCallbackName()) {
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
                sendMessage(*object,msg,serializedMessageBody,serializedMessageBodySize);
            } else if (deliverAllMessages&&sendState==DELIVER_TO_OBJECT) {
                deliverMessage(*object,msg,serializedMessageBody,serializedMessageBodySize);
            }else {
                //some messages are not considered worth forwarding to the proximity system or there's a mishmash of destinations
                if (msg.body().message_arguments_size()<len) {
                    len=msg.body().message_arguments_size();
                }
                RoutableMessage newMsg (msg);
                newMsg.body().clear_message_names();
                newMsg.body().clear_message_arguments();
                bool forwardLastMessageType=false;
                for (int i=0;i<len;++i) {
                    if (forwardThisName(registration||disconnection,msg.body().message_names(i))) {
                        forwardLastMessageType=true;
                        newMsg.body().add_message_names(newMsg.body().message_names(i));
                        newMsg.body().add_message_arguments(newMsg.body().message_arguments(i));
                    }else {
                        forwardLastMessageType=false;
                    }
                }
                if (forwardLastMessageType) {
                    int fullSize=msg.body().message_arguments_size();
                    for (int i=len;i<fullSize;++i) {
                        newMsg.body().add_message_arguments(newMsg.body().message_arguments(i));
                    }
                }
                if (sendState==DELIVER_TO_OBJECT)
                    deliverMessage(*object,newMsg,NULL,0);
                if (sendState==DELIVER_TO_PROX)
                    sendMessage(*object,newMsg,NULL,0);
                
            }
            return obj_is_deleted;
        }catch(std::invalid_argument&ia) {
            SILOG(proximity,warning,"Could not parse message");
            return OBJECT_NOT_DESTROYED;
        }

    }

public:
    ObjectSpaceBridgeProximitySystem (ObjectPtr obj,unsigned int registrationMessagePort) {
        mObject=obj;
        mRegistrationPort=registrationMessagePort;
    }
    enum MessageBundle{
        DELIVER_TO_UNKNOWN,
        DELIVER_TO_OBJECT,
        DELIVER_TO_PROX,
        DELIVER_TO_BOTH
        
    };
    ///Do not forward any messages of interest to other plugins
    virtual bool forwardMessagesTo(MessageService*){return false;}
    virtual bool endForwardingMessagesTo(MessageService*){return false;}

    /**
     * Process a message that may be meant for the proximity system
     */
    virtual void processMessage(const RoutableMessageHeader& mesg,
                                MemoryReference message_body) {
        internalProcessOpaqueProximityMessage(mesg.has_source_object()?&mesg.source_object():NULL,
                                              mesg,
                                              message_body.data(),
                                              message_body.size(),true);
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
        RoutableMessage toSend;
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
        RoutableMessage delivery;
        constructMessage(delivery,NULL,&destination,"ProxCall",callInfo,optionalSerializedProxCall,optionalSerializedProxCallSize);
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
        RoutableMessage toSend;
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
