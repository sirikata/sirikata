/*  Sirikata
 *  EveryoneProximitySystem.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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

#include "Protocol_Sirikata.pbj.hpp"
#include "EveryoneProximitySystem.hpp"
#include <sirikata/core/options/Options.hpp>
#include <sirikata/core/network/StreamListenerFactory.hpp>
#include <sirikata/core/util/RoutableMessage.hpp>
#include <sirikata/core/util/RoutableMessageHeader.hpp>
#include <sirikata/core/util/RoutableMessageBody.hpp>

namespace Sirikata {
namespace Proximity {

ProximitySystem* EveryoneProximitySystem::create(Network::IOService*io,const String&options,const ProximitySystem::Callback& callback) {
    return new EveryoneProximitySystem(*io, options, callback);
}

EveryoneProximitySystem::EveryoneProximitySystem(Network::IOService& io, const String& options, const Callback& cb)
: mIO(&io),
  mCallback(cb)
{
    OptionValue*port;
    OptionValue*updateDuration;
    OptionValue*streamlib;
    OptionValue*streamoptions;
    InitializeClassOptions("prox-everyone",this,
        port=new OptionValue("port","6408",OptionValueType<String>(),"sets the port that the proximity bridge should listen on"),
        updateDuration=new OptionValue("updateDuration","60ms",OptionValueType<Duration>(),"sets the ammt of time between proximity updates"),
        streamlib=new OptionValue("protocol","",OptionValueType<String>(),"Sets the stream library to connect"),
        streamoptions=new OptionValue("options","",OptionValueType<String>(),"Options for the created stream"),
        NULL);

    (mOptions=OptionSet::getOptions("prox-everyone",this))->parse(options);

    mListener=Network::StreamListenerFactory::getSingleton()
        .getConstructor(streamlib->as<String>())(&io,
            Network::StreamListenerFactory::getSingleton()
            .getOptionParser(streamlib->as<String>())(streamoptions->as<String>()));

    mListener->listen(Network::Address("127.0.0.1",port->as<String>()),
        std::tr1::bind(&EveryoneProximitySystem::newObjectStreamCallback,this,_1,_2));
}

EveryoneProximitySystem::~EveryoneProximitySystem()
{
}

void EveryoneProximitySystem::newObjectStreamCallback(Network::Stream* newStream, Network::Stream::SetCallbacks& setCallbacks)
{
    if (newStream) {
        mConnectionInfo[newStream] = ConnectionInfo();
        setCallbacks(
            std::tr1::bind(&EveryoneProximitySystem::disconnectionCallback,this,newStream,_1,_2),
            std::tr1::bind(&EveryoneProximitySystem::incomingMessage,this,newStream,_1,_2),
            &Network::Stream::ignoreReadySendCallback
        );
    }else {
        //whole object host has disconnected;
    }
}

void EveryoneProximitySystem::disconnectionCallback(Network::Stream* stream, Network::Stream::ConnectionStatus status, const std::string& reason)
{
    if (status!=Network::Stream::Connected) {
        ConnectionInfoMap::iterator conn_it = mConnectionInfo.find(stream);
        if (conn_it == mConnectionInfo.end())
            return;

        ConnectionInfo& conninfo = conn_it->second;
        for(ObjectSet::iterator querier_it = conninfo.queriers.begin(); querier_it != conninfo.queriers.end(); querier_it++)
            delObj(*querier_it);
        mConnectionInfo.erase(conn_it);
    }
}

void EveryoneProximitySystem::incomingMessage(Network::Stream* stream, const Network::Chunk& data, const Network::Stream::PauseReceiveCallback& pauseReceive)
{
    RoutableMessageHeader hdr;

    if (data.size() == 0)
        return;

    MemoryReference bodyData = hdr.ParseFromArray(&*data.begin(),data.size());
    if (hdr.has_source_object()) {
        ObjectReference source_object(hdr.source_object());
        ObjectSet newObjects;
        if (processOpaqueProximityMessage(stream, newObjects, &source_object, hdr,bodyData.data(),bodyData.size()) == OBJECT_DELETED) {
            ObjectSet::iterator del_obj_it = mConnectionInfo[stream].queriers.find(source_object);
            if (del_obj_it != mConnectionInfo[stream].queriers.end())
                mConnectionInfo[stream].queriers.erase(del_obj_it);
            mObjectConnections.erase(source_object);
        }
    } else {
        ObjectSet newObjects;
        if (processOpaqueProximityMessage(stream, newObjects, NULL, hdr, bodyData.data(), bodyData.size()) != OBJECT_DELETED && !newObjects.empty()) {
            mConnectionInfo[stream].queriers.insert(newObjects.begin(), newObjects.end());
            for(ObjectSet::iterator it = newObjects.begin(); it != newObjects.end(); it++)
                mObjectConnections[*it] = stream;
        }
    }
}

void EveryoneProximitySystem::sendProxCallback(Network::Stream* stream, const RoutableMessageHeader& destination, const Sirikata::RoutableMessageBody& unaddressed_prox_callback_msg) {
    std::string str;
    destination.SerializeToString(&str);
    unaddressed_prox_callback_msg.AppendToString(&str);
    stream->send(MemoryReference(str),Network::ReliableOrdered);
}

bool EveryoneProximitySystem::forwardMessagesTo(MessageService* ms) {
    MessageServiceList::iterator it = std::find(mMessageServices.begin(), mMessageServices.end(), ms);
    if (it != mMessageServices.end())
        return false;
    mMessageServices.push_back(ms);
    return true;
}

bool EveryoneProximitySystem::endForwardingMessagesTo(MessageService* ms) {
    MessageServiceList::iterator it = std::find(mMessageServices.begin(), mMessageServices.end(), ms);
    if (it == mMessageServices.end()) return false;
    mMessageServices.erase(it);
    return true;
}

void EveryoneProximitySystem::processMessage(const RoutableMessageHeader&msg, MemoryReference body_reference) {
    RoutableMessageBody body;
    if (body.ParseFromArray(body_reference.data(),body_reference.size())) {
        ObjectSet newObjectReferences;
        if (msg.has_source_object())
            processOpaqueProximityMessage(newObjectReferences, msg.source_object(), msg, body);
        else if (msg.has_destination_object())
            processOpaqueProximityMessage(newObjectReferences, msg.destination_object(), msg, body);
    }
}

ProximitySystem::OpaqueMessageReturnValue EveryoneProximitySystem::processOpaqueProximityMessage (
        Network::Stream* stream,
        ObjectSet& new_objects,
        ObjectReference* source,
        const RoutableMessageHeader&msg,
        const void *serializedMessageBody,
        size_t serializedMessageBodySize)
{
    RoutableMessageBody body;
    if (body.ParseFromArray(serializedMessageBody, serializedMessageBodySize)) {
        if (source)
            return processOpaqueProximityMessage(new_objects, *source, msg, body);
        else if (msg.has_source_object())
            return processOpaqueProximityMessage(new_objects, msg.source_object(), msg, body);
        else if (msg.has_destination_object())
            return processOpaqueProximityMessage(new_objects, msg.destination_object(), msg, body);
    }

    return OBJECT_NOT_DESTROYED;
}

ProximitySystem::OpaqueMessageReturnValue EveryoneProximitySystem::processOpaqueProximityMessage(
        ObjectSet& new_objects,
        const ObjectReference& source,
        const Sirikata::RoutableMessageHeader& hdr,
        const Sirikata::RoutableMessageBody& msg)
{
    int numMessages = msg.message_size();
    OpaqueMessageReturnValue retval = OBJECT_NOT_DESTROYED;
    ObjectReference last_obj = source;

    for (int i = 0; i < numMessages; ++i) {
        if (msg.message_names(i)=="RetObj") {
            Sirikata::Protocol::RetObj new_obj;
            if (new_obj.ParseFromString(msg.message_arguments(i))) {
                ObjectReference ref;
                last_obj = this->newObj(ref,new_obj);
                new_objects.insert(ref);
            }
        }
        else if (msg.message_names(i)=="NewProxQuery") {
            Sirikata::Protocol::NewProxQuery new_query;
            if (new_query.ParseFromString(msg.message_arguments(i))) {
                this->newProxQuery(last_obj,hdr.source_port(),new_query,msg.message_arguments(i).data(),msg.message_arguments(i).size());
            }
        }
        else if (msg.message_names(i)=="DelProxQuery") {
            Sirikata::Protocol::DelProxQuery del_query;
            if (del_query.ParseFromString(msg.message_arguments(i))) {
                this->delProxQuery(last_obj,hdr.source_port(),del_query,msg.message_arguments(i).data(),msg.message_arguments(i).size());
            }
        }
        else if (msg.message_names(i)=="ObjLoc"){
            Sirikata::Protocol::ObjLoc obj_loc;
            if (obj_loc.ParseFromString(msg.message_arguments(i))) {
                this->objLoc(last_obj,obj_loc,msg.message_arguments(i).data(),msg.message_arguments(i).size());
            }
        }
        else if (msg.message_names(i)=="DelObj") {
            Sirikata::Protocol::DelObj del_obj;
            if (del_obj.ParseFromString(msg.message_arguments(i))) {
                this->delObj(source);
                retval=OBJECT_DELETED;
                return retval;//once an object is deleted, the iterator is invalid
            }
        }
    }
    return retval;
}

ObjectReference EveryoneProximitySystem::newObj(const Sirikata::Protocol::IRetObj& obj_status,
    const void *optionalSerializedReturnObjectConnection,
    size_t optionalSerializedReturnObjectConnectionSize)
{
    ObjectReference retval;
    return newObj(retval, obj_status);
}

ObjectReference EveryoneProximitySystem::newObj(ObjectReference& retval, const Sirikata::Protocol::IRetObj& obj_status) {
    ObjectReference source(obj_status.object_reference());
    retval = source;

    ObjectSet::iterator it = mObjects.find(source);
    if (it != mObjects.end()) return source;

    mObjects.insert(source);
    notifyAll(source, Added);

    return source;
}

void EveryoneProximitySystem::newProxQuery(const ObjectReference&source,
    Sirikata::uint32 source_port,
    const Sirikata::Protocol::INewProxQuery& new_query,
    const void *optionalSerializedProximityQuery,
    size_t optionalSerializedProximitySize)
{
    ObjectSet::iterator it = mQueriers.find(source);
    if (it != mQueriers.end()) return;

    mQueriers.insert(source);
    mQuerierPorts[source] = source_port;
    mQueryIDs[source] = new_query.query_id();

    // FIXME we could wrap all of these into one result message
    for(ObjectSet::iterator obj_it = mObjects.begin(); obj_it != mObjects.end(); obj_it++)
        notify(source, *obj_it, Added);
}

void EveryoneProximitySystem::processProxCallback(const ObjectReference&destination,
    const Sirikata::Protocol::IProxCall& prox_callback,
    const void *optionalSerializedProxCall,
    size_t optionalSerializedProxCallSize)
{
    ObjectSet::iterator where = mQueriers.find(destination);
    if (where == mQueriers.end()) {
        SILOG(prox,warning,"Cannot callback to "<<destination<<" unknown stream");
        return;
    }

    RoutableMessageHeader dest;
    dest.set_destination_object(destination);
    Sirikata::RoutableMessageBody msg;
    if (optionalSerializedProxCallSize)
        msg.add_message("ProxCall",optionalSerializedProxCall,optionalSerializedProxCallSize);
    else
        prox_callback.SerializeToString(msg.add_message("ProxCall"));
    ObjectConnectionMap::iterator conn_it = mObjectConnections.find(destination);
    mCallback(conn_it == mObjectConnections.end() ? NULL : conn_it->second, dest, msg);
}

void EveryoneProximitySystem::objLoc(const ObjectReference&source,
    const Sirikata::Protocol::IObjLoc&,
    const void *optionalSerializedObjLoc,
    size_t optionalSerializedObjLocSize)
{
    // Loc updates don't do anything since everybody knows about each other
}

void EveryoneProximitySystem::delProxQuery(const ObjectReference&source,
    Sirikata::uint32 source_port,
    const Sirikata::Protocol::IDelProxQuery& cb,
    const void *optionalSerializedDelProxQuery,
    size_t optionalSerializedDelProxQuerySize)
{
    ObjectSet::iterator it = mQueriers.find(source);
    if (it == mQueriers.end()) return;
    mQueriers.erase(it);
}

void EveryoneProximitySystem::delObj(const ObjectReference& source,
    const Sirikata::Protocol::IDelObj&,
    const void *optionalSerializedDelObj,
    size_t optionalSerializedDelObjSize)
{
    delObj(source);
}

void EveryoneProximitySystem::delObj(const ObjectReference& source) {
    ObjectSet::iterator it = mObjects.find(source);
    if (it == mObjects.end()) return;

    mObjects.erase(it);
    notifyAll(source, Removed);
}

void EveryoneProximitySystem::notify(ObjectReference querier, ObjectReference member, ProxEventType evtType) {
    Protocol::ProxCall callback_message;
    RoutableMessage message_container;
    message_container.set_destination_object(querier);
    message_container.set_destination_port(mQuerierPorts[querier]);

    callback_message.set_proximity_event(evtType == Added ? Protocol::ProxCall::ENTERED_PROXIMITY : Protocol::ProxCall::EXITED_PROXIMITY);
    callback_message.set_proximate_object(member.getAsUUID());
    callback_message.set_query_id(mQueryIDs[querier]);
    callback_message.SerializeToString(message_container.body().add_message("ProxCall", std::string()));

    if (message_container.body().message_size()) {
        ObjectConnectionMap::iterator conn_it = mObjectConnections.find(querier);
        mCallback(conn_it == mObjectConnections.end() ? NULL : conn_it->second, message_container, message_container.body());
    }

    std::string toSerialize;
    message_container.body().SerializeToString(&toSerialize);
    for(MessageServiceList::iterator it = mMessageServices.begin(); it != mMessageServices.end(); it++) {
        MessageService* svc = *it;
        svc->processMessage(message_container.header(), MemoryReference(toSerialize));
    }
}

// Notifies all objects of a new member of their result set
void EveryoneProximitySystem::notifyAll(ObjectReference member, ProxEventType evtType) {
    for(ObjectSet::iterator querier_it = mQueriers.begin(); querier_it != mQueriers.end(); querier_it++)
        notify(*querier_it, member, evtType);
}

} // namespace Proximity
} // namespace Sirikata
