/*  Sirikata Proximity Management -- Prox Plugin
 *  ProxBridge.hpp
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
#include <boost/thread.hpp>
#include "network/Stream.hpp"
#include "network/StreamListener.hpp"

namespace Sirikata { namespace Proximity {
class QueryListener;
class ProxCallback;
/**
 * Forwards all messages to the Prox library proximity system
 */
class ProxBridge : public ProximitySystem {
    OptionSet*mOptions;
    ///The IO service that will handle received messages as well as tick() operations
    Network::IOService*mIO;
    ///The Stream used to listen for incoming messages. 
    Network::StreamListener*mListener;
    //The query handler for proximity: This class will not finish its destructor until all references to mListener are gone.
    std::tr1::shared_ptr<Prox::QueryHandler> mQueryHandler;
    friend class QueryListener;
    friend class ProxCallback;
    class QueryState {
    public:
        Prox::Query *mQuery;
        Vector3d mOffset;
        enum {
            RELATIVE_STATEFUL=0,
            ABSOLUTE_STATEFUL=1,
            RELATIVE_STATELESS=2,
            ABSOLUTE_STATELESS=3
        } mQueryType;
    };
    typedef std::map<uint32,QueryState> QueryMap;
    class ObjectState {
    public:
        Prox::Object * mObject;
        QueryMap mQueries;
        std::tr1::shared_ptr<Network::Stream> mStream;
        ObjectState(Network::Stream*strm):mStream(strm){mObject=NULL;}
    };
    typedef std::tr1::unordered_map<ObjectReference,ObjectState*,ObjectReference::Hasher >ObjectStateMap;
    ObjectStateMap mObjectStreams;//should it be a shared ptr to the stream? I think not since this is the only place we hold the ref
    /**
     * Process a message that may be meant for the proximity system
     * \returns whether an object has been deleted, so the previous system can update its records
     */
    OpaqueMessageReturnValue processOpaqueProximityMessage(std::vector<ObjectReference>&newObjectReferences,
                                       ObjectStateMap::iterator where,
                                       const Sirikata::RoutableMessageBody&);
    /**
     * Register a new proximity query.
     * The callback may come from an ASIO response thread
     */
    void newProxQuery(ObjectStateMap::iterator where,
                      const Sirikata::Protocol::INewProxQuery&,
                      const void *optionalSerializedProximityQuery=NULL,
                      size_t optionalSerializedProximitySize=0);
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
    void objLoc(ObjectStateMap::iterator where,
                const Sirikata::Protocol::IObjLoc&, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLocSize=0);

    /**
     * Objects may lose interest in a particular query
     * when this function returns, no more responses will be given
     */
    void delProxQuery(ObjectStateMap::iterator where,
                      const Sirikata::Protocol::IDelProxQuery&del_query,
                      const void *optionalSerializedDelProxQuery=NULL,
                      size_t optionalSerializedDelProxQuerySize=0);
    /**
     * Objects may be destroyed: indicate loss of interest here
     */
    void delObj(ObjectStateMap::iterator wheres);
    ObjectStateMap::iterator newObj(ObjectReference&,const Sirikata::Protocol::IRetObj&objectData);
    void newObjectStreamCallback(Network::Stream*newStream, Network::Stream::SetCallbacks&setCallbacks);
    void incomingMessage(const std::tr1::weak_ptr<Network::Stream>&strm,
                         const std::tr1::shared_ptr<std::vector<ObjectReference> >&ref,
                         const Network::Chunk&data);
    void disconnectionCallback(const std::tr1::shared_ptr<Network::Stream>&strm,
                               const std::tr1::shared_ptr<std::vector<ObjectReference> >&ref,
                               Network::Stream::ConnectionStatus stat,
                               const std::string&reason);
    static void sendProxCallback(Network::Stream*, const RoutableMessageHeader&,const Sirikata::RoutableMessageBody&);

    void update(const Duration&timeSinceUpdate,const std::tr1::weak_ptr<Prox::QueryHandler>&);
    void updateThread(const Duration&optimalUpdateTime,const std::tr1::weak_ptr<Prox::QueryHandler>&);

public:



    ProxBridge(Network::IOService&io,const String&options, Prox::QueryHandler*, const Callback &cb=&sendProxCallback);
    virtual ~ProxBridge();

    virtual OpaqueMessageReturnValue processOpaqueProximityMessage(std::vector<ObjectReference>&newObjectReferences,
                                               const ObjectReference*object,
                                               const RoutableMessageHeader&,
                                               const void *serializedMessageBody,
                                               size_t serializedMessageSize);

    ///Do not forward any messages of interest to other plugins
    virtual bool forwardMessagesTo(MessageService*);
    virtual bool endForwardingMessagesTo(MessageService*);

    /**
     * Process a message that may be meant for the proximity system
     * \returns true if the object was deleted from the proximity system with the message
     */
    virtual void processMessage(const RoutableMessageHeader&hdr,
                                           MemoryReference body);

    /**
     * Pass the ReturnedObjectConnection info,
     * containing an Object UUID to the proximity manager,
     * so the proximity system knows about a new object
     */
    virtual ObjectReference newObj(const Sirikata::Protocol::IRetObj&,
                        const void *optionalSerializedReturnObjectConnection=NULL,
                        size_t optionalSerializedReturnObjectConnectionSize=0);
    /**
     * Register a new proximity query.
     * The callback may come from an ASIO response thread
     */
    virtual void newProxQuery(const ObjectReference&source,
                              const Sirikata::Protocol::INewProxQuery&,
                              const void *optionalSerializedProximityQuery=NULL,
                              size_t optionalSerializedProximitySize=0);
    /**
     * Since certain setups have proximity responses coming from another message stream
     * Those messages should be shunted to this function and processed
     */
    virtual void processProxCallback(const ObjectReference&destination,
                                     const Sirikata::Protocol::IProxCall&,
                                     const void *optionalSerializedProxCall=NULL,
                                     size_t optionalSerializedProxCallSize=0);
    /**
     * The proximity management system must be informed of all position updates
     * Pass an objects position updates to this function
     */
    virtual void objLoc(const ObjectReference&source, const Sirikata::Protocol::IObjLoc&, const void *optionalSerializedObjLoc=NULL,size_t optionalSerializedObjLocSize=0);

    /**
     * Objects may lose interest in a particular query
     * when this function returns, no more responses will be given
     */
    virtual void delProxQuery(const ObjectReference&source, const Sirikata::Protocol::IDelProxQuery&cb,  const void *optionalSerializedDelProxQuery=NULL,size_t optionalSerializedDelProxQuerySize=0);
    /**
     * Objects may be destroyed: indicate loss of interest here
     */
    virtual void delObj(const ObjectReference&source, const Sirikata::Protocol::IDelObj&, const void *optionalSerializedDelObj=NULL,size_t optionalSerializedDelObjSize=0);
protected:
    static const int sMaxMessageServices=16;
    MessageService* mMessageServices[sMaxMessageServices];
    Callback mCallback;

};

} }
