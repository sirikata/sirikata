/*  Sirikata
 *  EveryoneProximitySystem.hpp
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

#ifndef _PROXIMITY_EVERYONE_PROXIMITY_SYSTEM_HPP_
#define _PROXIMITY_EVERYONE_PROXIMITY_SYSTEM_HPP_

#include <sirikata/proximity/Platform.hpp>
#include <sirikata/proximity/ProximitySystem.hpp>
#include <sirikata/core/network/Stream.hpp>
#include <sirikata/core/network/StreamListener.hpp>

namespace Sirikata {
namespace Proximity {

class EveryoneProximitySystem : public ProximitySystem {
public:
    static ProximitySystem* create(Network::IOService*io, const String& options, const ProximitySystem::Callback& callback);

    EveryoneProximitySystem(Network::IOService& io, const String& options, const Callback& cb = &sendProxCallback);
    virtual ~EveryoneProximitySystem();

    // Message Service Interface
    virtual bool forwardMessagesTo(MessageService*);
    virtual bool endForwardingMessagesTo(MessageService*);
    virtual void processMessage(const RoutableMessageHeader&msg, MemoryReference body_reference);

    // ProximitySystem Interface
    virtual ObjectReference newObj(const Sirikata::Protocol::IRetObj&,
                                   const void *optionalSerializedReturnObjectConnection=NULL,
                                   size_t optionalSerializedReturnObjectConnectionSize=0);

    virtual void newProxQuery(const ObjectReference&source,
                              Sirikata::uint32  source_port,
                              const Sirikata::Protocol::INewProxQuery&,
                              const void *optionalSerializedProximityQuery=NULL,
                              size_t optionalSerializedProximitySize=0);

    virtual void processProxCallback(const ObjectReference&destination,
                                     const Sirikata::Protocol::IProxCall&,
                                     const void *optionalSerializedProxCall=NULL,
                                     size_t optionalSerializedProxCallSize=0);

    virtual void objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLocSize=0);

    virtual void delProxQuery(const ObjectReference&source, Sirikata::uint32  source_port,const Sirikata::Protocol::IDelProxQuery&cb,  const void *optionalSerializedDelProxQuery=NULL,size_t optionalSerializedDelProxQuerySize=0);

    virtual void delObj(const ObjectReference&source, const Sirikata::Protocol::IDelObj&, const void *optionalSerializedDelObj=NULL,size_t optionalSerializedDelObjSize=0);

private:
    typedef std::vector<MessageService*> MessageServiceList;
    typedef std::set<ObjectReference> ObjectSet;

    static void sendProxCallback(Network::Stream*, const RoutableMessageHeader&,const Sirikata::RoutableMessageBody&);

    ObjectReference newObj(ObjectReference& retval, const Sirikata::Protocol::IRetObj& obj_status);

    ProximitySystem::OpaqueMessageReturnValue processOpaqueProximityMessage(
        Network::Stream* stream,
        ObjectSet& new_objects,
        ObjectReference* source,
        const RoutableMessageHeader&msg,
        const void *serializedMessageBody,
        size_t serializedMessageBodySize);

    ProximitySystem::OpaqueMessageReturnValue processOpaqueProximityMessage(
        ObjectSet& new_objects,
        const ObjectReference& source,
        const Sirikata::RoutableMessageHeader& hdr,
        const Sirikata::RoutableMessageBody& msg);

    enum ProxEventType {
        Added,
        Removed
    };


    typedef std::tr1::shared_ptr<Network::Stream> StreamPtr;
    typedef std::tr1::weak_ptr<Network::Stream> StreamWPtr;
    void newObjectStreamCallback(Network::Stream* newStream, Network::Stream::SetCallbacks& setCallbacks);
    void disconnectionCallback(Network::Stream* stream, Network::Stream::ConnectionStatus status, const std::string& reason);
    void incomingMessage(Network::Stream* stream, const Network::Chunk& data, const Network::Stream::PauseReceiveCallback& pauseReceive);

    void notify(ObjectReference querier, ObjectReference member, ProxEventType evtType);
    void notifyAll(ObjectReference member, ProxEventType evtType);

    void delObj(const ObjectReference& source);

    OptionSet* mOptions;
    Network::IOService* mIO;
    Network::StreamListener* mListener;
    Callback mCallback;

    MessageServiceList mMessageServices;

    ObjectSet mQueriers;
    ObjectSet mObjects;

    struct ConnectionInfo {
        ObjectSet queriers;
    };
    typedef std::map<Network::Stream*, ConnectionInfo> ConnectionInfoMap;
    ConnectionInfoMap mConnectionInfo;
    typedef std::map<ObjectReference, Network::Stream*> ObjectConnectionMap;
    ObjectConnectionMap mObjectConnections;
    typedef std::map<ObjectReference, uint32> QuerierPortMap;
    QuerierPortMap mQuerierPorts;
    typedef std::map<ObjectReference, uint32> QueryIDMap;
    QueryIDMap mQueryIDs;
};

} // namespace Proximity
} // namespace Sirikata

#endif //_PROXIMITY_EVERYONE_PROXIMITY_SYSTEM_HPP_
