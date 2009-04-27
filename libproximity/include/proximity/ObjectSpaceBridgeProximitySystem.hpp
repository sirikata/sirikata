/*  Sirikata Proximity Management -- Introduction Services
 *  ProximitySystem.hpp
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
#ifndef _PROXIMITY_PROXIMITYSYSTEM_HPP_
#define _PROXIMITY_PROXIMITYSYSTEM_HPP_
namespace Sirikata { namespace Proximity {
template <class ObjectPtr> 
class ObjectSpaceBridgeProximitySystem : public ProximitySystem{
    ObjectPtr mObject;
protected:
    virtual bool forwardThisName(const std::string&name) {
        return name=="NewProxQuery"||name=="ProxCallback"||name=="DelProxQuery";
    }
    virtual void addressMessage(Protocol::Message&output,
                                const ObjectReference*source,
                                const ObjectReference*destination) {
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
                                                const void *optionalSerializedMessage,
                                                size_t optionalSerializedMessageSize) {
        delivery.addressMessage(output,source,destination);
        delivery.add_message_name(messageName);
        if (optionalSerializedProxCall&&optionalSerializedProxCallSize) {
            delivery.add_message_arguments(optionalSerializedProxCall,optionalSerializedProxCallSize);
        }else {
            delivery.add_message_names(std::string());
            callInfo.serializePartialToString(&delivery.message_names(0));
        }
    }

public:
    ObjectSpaceBridgeProximitySystem (ObjectPtr obj) {
        mObject=obj;
    }
    /**
     * Process a message that may be meant for the proximity system
     * \returns true for proximity-specific message not of interest to others
     */
    virtual bool processOpaqueProximityMessage(const Sirikata::Protocol::IMessage& msg,
                                               const void *optionalSerializedMessage=NULL,
                                               size_t optionalSerializedMessageSize=0) {
        std::vector<int> unnecessaryMessages;
        int len=msg->message_names_size();
        for (int i=0;i<len;++i) {
            if (!forwardMessageName(msg->message_name(i))) {
                if (len==1) return false;
                unnecessaryMessages.push_back(i);
            }
        }
        if (unnecessaryMessages.length()==0) {
            mObject->send(msg,optionalSerializedMessage,optionalSerializedMessageSize);
        }else {
            //some messages are not considered worth forwarding to the proximity system
            if (msg->message_arugments_size()<len) {
                len=msg->message_arugments_size();
            }
            if (unnecessaryMessages.size()==(size_t)len)
                return false;//no messages considered worth forwarding to the proximity system
            Protocol::Message newMsg (msg);
            newMsg->clear_message_names();
            newMsg->clear_message_arguments();
            std::vector<int>::iterator umsg=unnecessaryMessages.end();
            for (int i=0;i<len;++i) {
                if (umsg!=unnecessaryMessages.end()&&*umsg==i) {
                    ++umsg;
                    continue;
                }
                newMessage.add_message_names(newMsg->message_names(i));
                newMessage.add_message_arguments(newMsg->message_arguments(i));
            }
            mObject->send(newMsg);
        }
        return true;
    }

    /**
     * Pass the ReturnedObjectConnection info, 
     * containing an Object UUID to the proximity manager, 
     * so the proximity system knows about a new object
     */
    virtual void newObj(const Sirikata::Protocol::INewObj&,
                        const void *optionalSerializedReturnObjectConnection=NULL,
                        size_t optionalSerializedReturnObjectConnection=0){
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
        constructMessage(toSend,&source,NULL,"NewProxQuery",messageName,newProxQueryMsg,optionalSerializedProximityQuery,optionalSerializedProximitySize);
        mObject->deliver(toSend);
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
        mObject->deliver(delivery);
    }
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
    virtual void objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLoc=0) {
        //does nothing since the space is forwarding proximity messages with this bridge
    }

    /**
     * Objects may lose interest in a particular query
     * when this function returns, no more responses will be given
     */
    virtual void delProxQuery(const ObjectReference&source, const Sirikata::Protocol::IDelProxQuery&delProx,  const void *optionalSerializedDelProxQuery=NULL,size_t optionalSerializedDelProxQuerySize=0){
        Protocol::Message toSend;
        constructMessage(toSend,&source,NULL,"DelProxQuery",messageName,delProx,optionalSerializedDelProxQuery,optionalSerializedDelproxQuerySize);
        mObject->deliver(toSend);
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
