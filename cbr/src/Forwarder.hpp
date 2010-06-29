/*  Sirikata
 *  Forwarder.hpp
 *
 *  Copyright (c) 2010, Daniel Reiter Horn
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

#ifndef _SIRIKATA_FORWARDER_HPP_
#define _SIRIKATA_FORWARDER_HPP_

#include <sirikata/core/util/Platform.hpp>
#include "SpaceContext.hpp"
#include <sirikata/cbrcore/Message.hpp>
#include "SpaceNetwork.hpp"

#include <sirikata/core/queue/Queue.hpp>
#include <sirikata/core/queue/FairQueue.hpp>

#include "OSegLookupQueue.hpp"

#include "ServerMessageQueue.hpp"
#include "ServerMessageReceiver.hpp"

#include <sirikata/cbrcore/SSTImpl.hpp>

#include "ForwarderServiceQueue.hpp"

#include <sirikata/core/queue/SizedThreadSafeQueue.hpp>
#include <sirikata/core/queue/ThreadSafeQueueWithNotification.hpp>

namespace Sirikata
{
  class Object;
  class ObjectSegmentation;
  class CoordinateSegmentation;
  class SpaceNetwork;
namespace Trace {
  class Trace;
}
  class ObjectConnection;
  class OSegLookupQueue;
class ForwarderServiceQueue;
class ODPFlowScheduler;
class LocationService;
class LocalForwarder;

class Forwarder : public ServerMessageDispatcher, public ObjectMessageDispatcher,
		    public ServerMessageRouter, public ObjectMessageRouter,
                    public MessageRecipient,
                    public ServerMessageQueue::Sender,
                  public ServerMessageReceiver::Listener,
                  private ForwarderServiceQueue::Listener,
                  public Service
{
private:
    SpaceContext* mContext;
    ForwarderServiceQueue *mOutgoingMessages;
    ServerMessageQueue* mServerMessageQueue;
    ServerMessageReceiver* mServerMessageReceiver;

    LocalForwarder* mLocalForwarder;
    OSegLookupQueue* mOSegLookups; //this maps the object ids to a list of messages that are being looked up in oseg.
    boost::shared_ptr<BaseDatagramLayer<UUID> >  mSSTDatagramLayer;


    // Object connections, identified by a separate unique ID to handle fast migrations
    uint64 mUniqueConnIDs; // Connection ID generator
    struct UniqueObjConn
    {
      uint64 id;
      ObjectConnection* conn;
    };
    typedef std::tr1::unordered_map<UUID, UniqueObjConn, UUID::Hasher> ObjectConnectionMap;
    ObjectConnectionMap mObjectConnections;

    typedef std::vector<ServerID> ListServersUpdate;
    typedef std::tr1::unordered_map<UUID,ListServersUpdate, UUID::Hasher> ObjectServerUpdateMap;
    ObjectServerUpdateMap mServersToUpdate; // Map of object id -> servers which should be notified of new result


    // ServerMessageRouter data
    ForwarderServiceQueue::ServiceID mServiceIDSource;
    typedef std::map<String, ForwarderServiceQueue::ServiceID> ServiceMap;
    ServiceMap mServiceIDMap;

    // Per-Service ServerMessage Router's
    Router<Message*>* mOSegCacheUpdateRouter;
    Router<Message*>* mForwarderWeightRouter;
    typedef std::tr1::unordered_map<ServerID, ODPFlowScheduler*> ODPRouterMap;
    boost::recursive_mutex mODPRouterMapMutex;
    ODPRouterMap mODPRouters;
    Poller mServerWeightPoller; // For updating ServerMessageQueue, remote
                                // ServerMessageReceiver with per-server weights

    // Note: This is kinda stupid, but we need to protect this thread safe queue
    // with another lock because we don't have a sized thread safe queue with
    // notification.
    boost::mutex mReceivedMessagesMutex;
    Sirikata::SizedThreadSafeQueue<Message*> mReceivedMessages;

    // -- Boiler plate stuff - initialization, destruction, methods to satisfy interfaces
  public:
      Forwarder(SpaceContext* ctx);
      ~Forwarder();
    void initialize(ObjectSegmentation* oseg, ServerMessageQueue* smq, ServerMessageReceiver* smr, LocationService* loc);

    void setLocalForwarder(LocalForwarder* lf) { mLocalForwarder = lf; }

    // Service Implementation
    void start();
    void stop();
  protected:

    virtual void dispatchMessage(const Sirikata::Protocol::Object::ObjectMessage& msg) const;

  private:
    // Init method: adds an odp routing service to the ForwarderServiceQueue and
    // sets up the callback used to create new ODP input queues.
    void addODPServerMessageService(LocationService* loc);
    // Allocates a new ODPFlowScheduler, invoked by ForwarderServiceQueue when a
    // new server connection is made.  This creates it and gets it setup so the
    // Forwarder can get weight updates sent to the remote endpoint.
    ODPFlowScheduler* createODPFlowScheduler(LocationService* loc, ServerID remote_server, uint32 max_size);

    // Invoked periodically by an (internal) poller to update server fair queue
    // weights. Updates local ServerMessageQueue and sends messages to remote
    // ServerMessageReceivers.
    void updateServerWeights();

    // -- Public routing interface
  public:
    virtual Router<Message*>* createServerMessageService(const String& name);

    // Used only by Server.  Called from networking thready to try to forward
    // quickly (avoiding going through OSeg Lookup Queue) by checking OSeg
    // cache.
    WARN_UNUSED
    bool tryCacheForward(Sirikata::Protocol::Object::ObjectMessage* msg);

    WARN_UNUSED
    bool route(Sirikata::Protocol::Object::ObjectMessage* msg);

    // -- Real routing interface + implementation


    // --- Inputs
  public:
    // Received from OH networking, needs forwarding decision.  Forwards or
    // drops -- ownership is given to Forwarder either way
    void routeObjectHostMessage(Sirikata::Protocol::Object::ObjectMessage* obj_msg);
  private:
    // Received from other space server, needs forwarding decision
    void receiveMessage(Message* msg);

    void receiveObjectRoutingMessage(Message* msg);
    void receiveWeightUpdateMessage(Message* msg);

  private:
    // --- Worker Methods - do the real forwarding decision making and work

    /** Try to forward a message to get it closer to the destination object.
     *  This checks if we have a direct connection to the object, then does an
     *  OSeg lookup if necessary.
     */
    WARN_UNUSED
    bool forward(Sirikata::Protocol::Object::ObjectMessage* msg, ServerID forwardFrom = NullServerID);

    // This version is provided if you already know which server the message should be sent to
    void routeObjectMessageToServerNoReturn(Sirikata::Protocol::Object::ObjectMessage* msg, const CraqEntry& dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, ServerID forwardFrom = NullServerID);
    WARN_UNUSED
    bool routeObjectMessageToServer(Sirikata::Protocol::Object::ObjectMessage* msg, const CraqEntry& dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, ServerID forwardFrom = NullServerID);

    // Handles the case where OSeg told us we have the object. Post this to the
    // main strand.
    void handleObjectMessageLoop(Sirikata::Protocol::Object::ObjectMessage* msg) const;

    // ServerMessageQueue::Sender Interface
    virtual Message* serverMessagePull(ServerID dest);
    virtual bool serverMessageEmpty(ServerID dest);
    // ServerMessageReceiver::Listener Interface
    virtual void serverConnectionReceived(ServerID sid);
    virtual void serverMessageReceived(Message* msg);

    void scheduleProcessReceivedServerMessages();
    void processReceivedServerMessages();

    // ForwarderServiceQueue::Listener Interface (passed on to ServerMessageQueue)
    virtual void forwarderServiceMessageReady(ServerID dest_server);


    // -- Object Connection Management used by Server
  public:
    void addObjectConnection(const UUID& dest_obj, ObjectConnection* conn);
    void enableObjectConnection(const UUID& dest_obj);
    ObjectConnection* removeObjectConnection(const UUID& dest_obj);
  //private: FIXME these should not be public
  public:
    ObjectConnection* getObjectConnection(const UUID& dest_obj);
    ObjectConnection* getObjectConnection(const UUID& dest_obj, uint64& uniqueconnid );
}; // class Forwarder

} //end namespace Sirikata


#endif //_SIRIKATA_FORWARDER_HPP_
