/*  Sirikata
 *  Forwarder.cpp
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


#include "SpaceNetwork.hpp"
#include "Server.hpp"
#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include "ServerMessageQueue.hpp"
#include "ServerMessageReceiver.hpp"
#include <sirikata/core/trace/Trace.hpp>
#include "Options.hpp"
#include <sirikata/core/options/CommonOptions.hpp>

#include "Forwarder.hpp"
#include <sirikata/space/ObjectSegmentation.hpp>
#include "OSegLookupQueue.hpp"

#include "ObjectConnection.hpp"

#include "ForwarderServiceQueue.hpp"
#include "LocalForwarder.hpp"

#include <sirikata/core/util/Random.hpp>

#include "ODPFlowScheduler.hpp"
#include "RegionODPFlowScheduler.hpp"
#include "CSFQODPFlowScheduler.hpp"

#include <sirikata/core/odp/DelegateService.hpp>

// FIXME we shouldn't have oseg specific things here, this should be delegated
// to OSeg as necessary
#include "Protocol_OSeg.pbj.hpp"

#include "Protocol_Forwarder.pbj.hpp"

#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata
{

class ForwarderServerMessageRouter : public Router<Message*> {
  public:
    ForwarderServerMessageRouter(ForwarderServiceQueue* svc_queues, ForwarderServiceQueue::ServiceID service_id)
            : mForwarderServiceQueue(svc_queues),
              mServiceID(service_id)
    {
        mForwarderServiceQueue->addService(service_id);
    }

    WARN_UNUSED
    virtual bool route(Message* msg) {
        return (mForwarderServiceQueue->push(mServiceID, msg) == QueueEnum::PushSucceeded);
    }

  private:
    ForwarderServiceQueue* mForwarderServiceQueue;
    ForwarderServiceQueue::ServiceID mServiceID;
};

// -- First, all the boring boiler plate type stuff; initialization,
// -- destruction, delegation to base classes

#define ODP_SERVER_MESSAGE_SERVICE "object-messages"

  /*
    Constructor for Forwarder
  */
Forwarder::Forwarder(SpaceContext* ctx)
        :    mContext(ctx),
             mOutgoingMessages(NULL),
             mServerMessageQueue(NULL),
             mServerMessageReceiver(NULL),
             mLocalForwarder(NULL),
             mOSegLookups(NULL),
             mUniqueConnIDs(0),
             mServiceIDSource(0),
             mServerWeightPoller(
                 ctx->mainStrand,
                 std::tr1::bind(&Forwarder::updateServerWeights, this),
                 Duration::milliseconds((int64)10)),
             mReceivedMessages(Sirikata::SizedResourceMonitor(GetOptionValue<uint32>(FORWARDER_RECEIVE_QUEUE_SIZE)))
{
    mOutgoingMessages = new ForwarderServiceQueue(mContext->id(), GetOptionValue<uint32>(FORWARDER_SEND_QUEUE_SIZE), (ForwarderServiceQueue::Listener*)this);

    // Fill in the rest of the context
    mContext->mServerRouter = this;
    mContext->mServerDispatcher = this;

    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
    this->registerMessageRecipient(SERVER_PORT_FORWARDER_WEIGHT_UPDATE, this);

    // Generate router queues for services we provide
    // NOTE: See addODPServerMessageService(loc); in initialize.  We need loc
    // for it so we can't do it here.
    mOSegCacheUpdateRouter = createServerMessageService("oseg-cache-update");
    mForwarderWeightRouter = createServerMessageService("forwarder-weights");
}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
      for(ODPRouterMap::iterator it = mODPRouters.begin(); it != mODPRouters.end(); it++)
          delete it->second;
      mODPRouters.clear();

      delete mOSegCacheUpdateRouter;
      delete mForwarderWeightRouter;

      this->unregisterMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
      this->unregisterMessageRecipient(SERVER_PORT_FORWARDER_WEIGHT_UPDATE, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
void Forwarder::initialize(ObjectSegmentation* oseg, ServerMessageQueue* smq, ServerMessageReceiver* smr, LocationService* loc)
{
    addODPServerMessageService(loc);

    mOSegLookups = new OSegLookupQueue(mContext->mainStrand, oseg);
    mServerMessageQueue = smq;
    mServerMessageReceiver = smr;
}

void Forwarder::setODPService(ODP::DelegateService* odp) {
    mDelegateODPService = odp;
    mSSTDatagramLayer = BaseDatagramLayer<SpaceObjectReference>::createDatagramLayer(
        SpaceObjectReference(SpaceID::null(), ObjectReference::spaceServiceID()), mContext, odp
    );
}


void Forwarder::dispatchMessage(Sirikata::Protocol::Object::ObjectMessage* obj_msg) const {
    mDelegateODPService->deliver(
        ODP::Endpoint(SpaceID::null(), ObjectReference(obj_msg->source_object()), obj_msg->source_port()),
        ODP::Endpoint(SpaceID::null(), ObjectReference(obj_msg->dest_object()), obj_msg->dest_port()),
        MemoryReference(obj_msg->payload())
    );
    delete obj_msg;
}

void Forwarder::handleObjectMessageLoop(Sirikata::Protocol::Object::ObjectMessage* obj_msg) const {
    dispatchMessage(obj_msg);
}

// Service Implementation
void Forwarder::start() {
    mServerWeightPoller.start();
}

void Forwarder::stop() {
    mServerWeightPoller.stop();
}

// -- Object Connection Management - Object connections are available locally,
// -- and represent direct connections to endpoints.

void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
  UniqueObjConn uoc;
  uoc.id = mUniqueConnIDs;
  uoc.conn = conn;
  ++mUniqueConnIDs;

  mObjectConnections[dest_obj] = uoc;
}

void Forwarder::enableObjectConnection(const UUID& dest_obj) {
    ObjectConnection* conn = getObjectConnection(dest_obj);
    if (conn == NULL) {
        SILOG(forwarder,warn,"Tried to enable connection for unknown object.");
        return;
    }

    conn->enable();
}

ObjectConnection* Forwarder::removeObjectConnection(const UUID& dest_obj) {
    //    ObjectConnection* conn = mObjectConnections[dest_obj];
    UniqueObjConn uoc = mObjectConnections[dest_obj];
    ObjectConnection* conn = uoc.conn;
    mObjectConnections.erase(dest_obj);
    return conn;
}

ObjectConnection* Forwarder::getObjectConnection(const UUID& dest_obj) {
    ObjectConnectionMap::iterator it = mObjectConnections.find(dest_obj);
    return (it == mObjectConnections.end()) ? NULL : it->second.conn;
}

ObjectConnection* Forwarder::getObjectConnection(const UUID& dest_obj, uint64& ider )
{
    ObjectConnectionMap::iterator it = mObjectConnections.find(dest_obj);
    if (it == mObjectConnections.end())
    {
      ider = 0;
      return NULL;
    }
    ider = it->second.id;
    return it->second.conn;
}


// -- Server message handling.  These methods handle server to server messages,
// -- where the payload is either an already serialized ODP message, meaning its
// -- information isn't available, or may simply be between two space servers so
// -- that object information doesn't even exist.

void Forwarder::addODPServerMessageService(LocationService* loc) {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    assert(mServiceIDMap.find(ODP_SERVER_MESSAGE_SERVICE) == mServiceIDMap.end());

    ForwarderServiceQueue::ServiceID svc_id = mServiceIDSource++;
    mServiceIDMap[ODP_SERVER_MESSAGE_SERVICE] = svc_id;
    mOutgoingMessages->addService(
        svc_id,
        std::tr1::bind(&Forwarder::createODPFlowScheduler, this, loc, _1, _2)
    );
}

ODPFlowScheduler* Forwarder::createODPFlowScheduler(LocationService* loc, ServerID remote_server, uint32 max_size) {
    String flow_sched_type = GetOptionValue<String>(SERVER_ODP_FLOW_SCHEDULER);
    ODPFlowScheduler* new_flow_scheduler = NULL;

    if (flow_sched_type == "region") {
        new_flow_scheduler =
            new RegionODPFlowScheduler(mContext, mOutgoingMessages, remote_server, mServiceIDMap[ODP_SERVER_MESSAGE_SERVICE], max_size);
    }
    else if (flow_sched_type == "csfq") {
        new_flow_scheduler =
            new CSFQODPFlowScheduler(mContext, mOutgoingMessages, remote_server, mServiceIDMap[ODP_SERVER_MESSAGE_SERVICE], max_size, loc);
    }

    assert(new_flow_scheduler != NULL);

    {
        boost::lock_guard<boost::recursive_mutex> lck(mODPRouterMapMutex);
        mODPRouters[remote_server] = new_flow_scheduler;
    }
    return new_flow_scheduler;
}

void Forwarder::updateServerWeights() {
    boost::lock_guard<boost::recursive_mutex> lck(mODPRouterMapMutex);

    for(ODPRouterMap::iterator it = mODPRouters.begin(); it != mODPRouters.end(); it++) {
        ServerID serv_id = it->first;
        ODPFlowScheduler* serv_flow_sched = it->second;

        double odp_total_weight = serv_flow_sched->totalActiveWeight();
        double odp_sender_used_weight = serv_flow_sched->totalSenderUsedWeight();
        double odp_receiver_used_weight = serv_flow_sched->totalReceiverUsedWeight();
        double sender_total_weight = mServerMessageQueue->totalUsedWeight();
        double sender_capacity = mServerMessageQueue->capacity();
        double receiver_total_weight = mServerMessageReceiver->totalUsedWeight();
        double receiver_capacity = mServerMessageReceiver->capacity();

        // Update remote server, i.e. the receive scheduler, with
        // stats from the ODP server.
        Sirikata::Protocol::Forwarder::WeightUpdate weight_update;

        // Note that the naming here is a bit confusing.
        weight_update.set_server_pair_total_weight( odp_total_weight );
        weight_update.set_server_pair_used_weight( odp_receiver_used_weight );

        weight_update.set_receiver_total_weight( receiver_total_weight );
        weight_update.set_receiver_capacity( receiver_capacity );

        SILOG(forwarder,insane,"Sending weights: " << mContext->id() << " -> " << serv_id <<
            " odp_total_weight: " << odp_total_weight <<
            " odp_sender_used_weight: " << odp_sender_used_weight <<
            " odp_receiver_used_weight: " << odp_receiver_used_weight <<
            " sender_total_weight: " << sender_total_weight <<
            " sender_capacity: " << sender_capacity <<
            "sender_blok: "<<mServerMessageQueue->isBlocked() <<
            " receiver_total_weight: " << receiver_total_weight <<
            " receiver_capacity: " << receiver_capacity<<
            "receiver_blok: "<<mServerMessageReceiver->isBlocked()
        );

        Message* weight_up_msg = new Message(
            mContext->id(),
            SERVER_PORT_FORWARDER_WEIGHT_UPDATE,
            serv_id,
            SERVER_PORT_FORWARDER_WEIGHT_UPDATE,
            serializePBJMessage(weight_update)
        );

        bool success = mForwarderWeightRouter->route(weight_up_msg);
        if (!success)
            SILOG(forwarder,insane,"Overflow in forwarder weight message queue!");

        // Update send scheduler with values from the ODP flow scheduler.
        mServerMessageQueue->updateReceiverStats(serv_id, odp_total_weight, odp_sender_used_weight);
    }
}

Router<Message*>* Forwarder::createServerMessageService(const String& name) {
    ServiceMap::iterator it = mServiceIDMap.find(name);
    assert(it == mServiceIDMap.end());

    ForwarderServiceQueue::ServiceID svc_id = mServiceIDSource++;
    mServiceIDMap[name] = svc_id;
    return new ForwarderServerMessageRouter(mOutgoingMessages, svc_id);
}

void Forwarder::forwarderServiceMessageReady(ServerID dest_server) {
    // Note: this must occur in the main thread so we can call this
    // synchronously (instead of posting to main strand) so the FairQueue in the
    // SMQ can be updated correctly.
    mServerMessageQueue->messageReady(dest_server);
}


// -- Object Message Entry Points - entry points into the forwarder for object
// -- messages.  Sources include object hosts and other space servers.

// --- From object hosts
void Forwarder::routeObjectHostMessage(Sirikata::Protocol::Object::ObjectMessage* obj_msg) {
    // Messages destined for the space skip the object message queue and just get dispatched
    if (obj_msg->dest_object() == UUID::null()) {
        dispatchMessage(obj_msg);
        return;
    }

    bool forwarded = forward(obj_msg);
    if (!forwarded) {
        TIMESTAMP(obj_msg, Trace::DROPPED_DURING_FORWARDING);
        TRACE_DROP(DROPPED_DURING_FORWARDING);
        delete obj_msg;
    }
}

// --- Forwarded from other space servers
void Forwarder::receiveMessage(Message* msg) {
    // Forwarder only subscribes as a recipient for object messages
    // so it can easily check whether it can deliver directly
    // or needs to forward them.
    assert(msg->dest_port() == SERVER_PORT_OBJECT_MESSAGE_ROUTING ||
        msg->dest_port() == SERVER_PORT_FORWARDER_WEIGHT_UPDATE);

    if (msg->dest_port() == SERVER_PORT_OBJECT_MESSAGE_ROUTING)
        receiveObjectRoutingMessage(msg);
    else if (msg->dest_port() == SERVER_PORT_FORWARDER_WEIGHT_UPDATE)
        receiveWeightUpdateMessage(msg);
}

void Forwarder::receiveObjectRoutingMessage(Message* msg) {
    Sirikata::Protocol::Object::ObjectMessage* obj_msg = new Sirikata::Protocol::Object::ObjectMessage();
    bool parsed = parsePBJMessage(obj_msg, msg->payload());
    if (!parsed) {
        LOG_INVALID_MESSAGE(forwarder, error, msg->payload());
        delete obj_msg;
        delete msg;
        return;
    }

    TIMESTAMP(obj_msg, Trace::HANDLE_SPACE_MESSAGE);

    UUID dest = obj_msg->dest_object();

    // Special case: Object 0 is the space itself
    if (dest == UUID::null()) {
        dispatchMessage(obj_msg);
        return;
    }


    // Otherwise, try to forward it
    bool forward_success = forward(obj_msg, msg->source_server());

    if (!forward_success) {
        TIMESTAMP(obj_msg, Trace::DROPPED_DURING_FORWARDING);
        TRACE_DROP(DROPPED_DURING_FORWARDING_ROUTING);
        delete obj_msg;
    }

    delete msg;
}

void Forwarder::receiveWeightUpdateMessage(Message* msg) {
    ServerID source = msg->source_server();

    Sirikata::Protocol::Forwarder::WeightUpdate weight_update;
    bool parsed = parsePBJMessage(&weight_update, msg->payload());
    if (!parsed) {
        LOG_INVALID_MESSAGE(forwarder, error, msg->payload());
        return;
    }

    SILOG(forwarder,insane,"Received weights: " << source << " -> " << mContext->id() <<
        " server_pair_total_weight: " << weight_update.server_pair_total_weight() <<
        " server_pair_used_weight: " << weight_update.server_pair_used_weight() <<
        " receiver_total_weight: " << weight_update.receiver_total_weight() <<
        " receiver_capacity: " << weight_update.receiver_capacity()
    );

    mServerMessageReceiver->updateSenderStats(
        source,
        weight_update.server_pair_total_weight(),
        weight_update.server_pair_used_weight()
    );

    ODPFlowScheduler* serv_flow_sched = NULL;
    {
        boost::lock_guard<boost::recursive_mutex> lck(mODPRouterMapMutex);
        ODPRouterMap::iterator flow_sched_it = mODPRouters.find(source);
        if (flow_sched_it != mODPRouters.end())
            serv_flow_sched = flow_sched_it->second;
    }
    if (serv_flow_sched != NULL) {
        // Update with receiver stats from this remote server.
        serv_flow_sched->updateReceiverStats(
            weight_update.receiver_total_weight(),
            weight_update.receiver_capacity()
        );
        // And with sender stats from our own server (arbitrarily
        // chose now to update, see similar choice in
        // updateServerWeights for updates flowing in the other
        // direction).
        serv_flow_sched->updateSenderStats(
            mServerMessageQueue->totalUsedWeight(),
            mServerMessageQueue->capacity()
        );
    }
}


// -- Real Routing - Given an object message, from any source, decide where it
// -- needs to go and send it out in that direction.

bool Forwarder::forward(Sirikata::Protocol::Object::ObjectMessage* msg, ServerID forwardFrom)
{
    TIMESTAMP_START(tstamp, msg);
    TIMESTAMP_END(tstamp, Trace::FORWARDING_STARTED);

    // Note: We used to try forwarding locally here.  Now we don't bother
    // because we should have tried this when the message arrived, either
    // from the OH or another SS.

    // If we can't forward locally, do an OSeg lookup to find out where we need
    // to forward the message to
    TIMESTAMP_END(tstamp, Trace::OSEG_LOOKUP_STARTED);

    bool accepted = mOSegLookups->lookup(
        msg,
        std::tr1::bind(&Forwarder::routeObjectMessageToServerNoReturn, this, std::tr1::placeholders::_1, std::tr1::placeholders::_2,std::tr1::placeholders:: _3, forwardFrom)
    );

    return accepted;
}

WARN_UNUSED
bool Forwarder::tryCacheForward(Sirikata::Protocol::Object::ObjectMessage* msg) {
    TIMESTAMP_START(tstamp, msg);

    TIMESTAMP_END(tstamp, Trace::OSEG_CACHE_CHECK_STARTED);
    OSegEntry destserver = mOSegLookups->cacheLookup(msg->dest_object());
    TIMESTAMP_END(tstamp, Trace::OSEG_CACHE_CHECK_FINISHED);
    if (destserver.isNull())
        return false;

    if (destserver.server() == mContext->id())
        return false;

    // Use normal routing mechanism if we have a non-local dest
    bool send_success = routeObjectMessageToServer(msg, destserver, OSegLookupQueue::ResolvedFromCache, NullServerID);
    return true; // If we got here, the cache was successful, we just dropped it.
}

void Forwarder::routeObjectMessageToServerNoReturn(Sirikata::Protocol::Object::ObjectMessage* obj_msg, const OSegEntry &dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, ServerID forwardFrom) {
    routeObjectMessageToServer(obj_msg, dest_serv, resolved_from, forwardFrom);
}

bool Forwarder::routeObjectMessageToServer(Sirikata::Protocol::Object::ObjectMessage* obj_msg, const OSegEntry &dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, ServerID forwardFrom)
{
    Trace::MessagePath mp = (resolved_from == OSegLookupQueue::ResolvedFromCache)
        ? Trace::OSEG_CACHE_LOOKUP_FINISHED
        : Trace::OSEG_SERVER_LOOKUP_FINISHED;

    // Timestamp the message as having finished an OSeg Lookup
    TIMESTAMP(obj_msg, mp);
    TIMESTAMP(obj_msg, Trace::OSEG_LOOKUP_FINISHED);

    // It's possible we got the object after we started the query, resulting in
    // use pointing the message back at ourselves.  In this case, just dispatch
    // it.
    // This is rare, so we suffer the cost of posting back to the main thread.
    if (dest_serv.server() == mContext->id()) {
        mContext->mainStrand->post(
            std::tr1::bind(&Forwarder::handleObjectMessageLoop, this, obj_msg)
        );
        return true;
    }

  //send out all server updates associated with an object with this message:
  TIMESTAMP(obj_msg, Trace::SPACE_TO_SPACE_ENQUEUED);

  // And then we can actually push
  // We try to look up the ODPFlowScheduler efficiently first, and only prePush
  // if we fail to find it.
  ODPFlowScheduler* flow_sched = NULL;
  {
      boost::lock_guard<boost::recursive_mutex> lck(mODPRouterMapMutex);
      ODPRouterMap::iterator odp_it = mODPRouters.find(dest_serv.server());
      if (odp_it != mODPRouters.end())
          flow_sched = odp_it->second;
  }
  if (flow_sched == NULL) {
      // Will force allocation of ODPFlowScheduler if its not there already
      {
          boost::lock_guard<boost::recursive_mutex> lck(mODPRouterMapMutex);
          mOutgoingMessages->prePush(dest_serv.server());
          flow_sched = mODPRouters[dest_serv.server()];
      }
  }

  OSegEntry source_object_data(OSegEntry::null());//FIXME: do we want mandatory lookup for nonlocal guys?! = mOSegLookups->cacheLookup(obj_msg->source_object());
  if (source_object_data.isNull()) {
      source_object_data=OSegEntry(mContext->id(),1.0);//FIXME dumb default: RADIUS of reforwarded messages are 1.0
  }
  bool send_success = flow_sched->push(obj_msg,source_object_data,dest_serv);
  if (!send_success) {
      TIMESTAMP(obj_msg, Trace::DROPPED_AT_SPACE_ENQUEUED);
      TRACE_DROP(DROPPED_AT_SPACE_ENQUEUED);
  }
  delete obj_msg;

  // Note that this is done *after* the real message is sent since it is an optimization and
  // we don't want it blocking useful traffic
  // NOTE: Again, not thread safe, but the OH networking thread will never hit this.
  if (forwardFrom != NullServerID) {
      UUID obj_id =  obj_msg->dest_object();
      // FIXME we used to kind of keep track of sending the same OSeg cache fix to a server multiple
      // times, but it really only applied for the rate of lookups/migrations.  We should a) determine
      // if this is actually a problem and b) if it is, take a more principled approach to solving it.
#ifdef CRAQ_DEBUG
      std::cout<<"\n\n bftm debug Sending an oseg cache update message at time:  "<<mContext->simTime().raw()<<"\n";
      std::cout<<"\t for object:  "<<obj_id.toString()<<"\n";
      std::cout<<"\t to server:   "<<mServersToUpdate[obj_id][s]<<"\n";
      std::cout<<"\t obj on:      "<<dest_serv<<"\n\n";
#endif

      Sirikata::Protocol::OSeg::UpdateOSegMessage contents;
      contents.set_servid_sending_update(mContext->id());
      contents.set_servid_obj_on(dest_serv.server());
      contents.set_m_objid(obj_id);
      contents.set_m_objradius(dest_serv.radius());
      Message* up_os = new Message(
          mContext->id(),
          SERVER_PORT_OSEG_UPDATE,
          forwardFrom,
          SERVER_PORT_OSEG_UPDATE,
          serializePBJMessage(contents)
      );

      bool oseg_cache_msg_success = mOSegCacheUpdateRouter->route(up_os);
      // Ignore the success of this send.  If it failed the remote ends cache
      // will just continue to be incorrect, but forwarding will cover the error
  }

  return send_success;
}

Message* Forwarder::serverMessagePull(ServerID dest) {
    Message* next_msg = mOutgoingMessages->pop(dest);
    if (next_msg == NULL)
        return NULL;

    CONTEXT_SPACETRACE(serverDatagramQueued, next_msg->dest_server(), next_msg->id(), next_msg->serializedSize());

    return next_msg;
}

bool Forwarder::serverMessageEmpty(ServerID dest) {
    return mOutgoingMessages->empty(dest);
}

void Forwarder::serverConnectionReceived(ServerID sid) {
    ///need to take the lock because createODPFlowScheduler will want the lock later but an intervening lock will be taken
    boost::lock_guard<boost::recursive_mutex> lck(mODPRouterMapMutex);

    // With a new connection, we just need to make sure we get
    // information flowing back and forth (e.g. capacity/weight for
    // ODPFlowScheduler). This just gets that process started.
    mOutgoingMessages->prePush(sid);
}

void Forwarder::serverMessageReceived(Message* msg) {
    assert(msg != NULL);
    TIMESTAMP_PAYLOAD(msg, Trace::SPACE_TO_SPACE_SMR_DEQUEUED);

    // Routing, check if we can route immediately.
    if (msg->dest_port() == SERVER_PORT_OBJECT_MESSAGE_ROUTING) {
        Sirikata::Protocol::Object::ObjectMessage* obj_msg = new Sirikata::Protocol::Object::ObjectMessage();
        bool parsed = parsePBJMessage(obj_msg, msg->payload());
        if (!parsed) {
            LOG_INVALID_MESSAGE(forwarder, error, msg->payload());
            delete obj_msg;
            delete msg;
            return;
        }

        // This process is very similar to the one followed in Server for
        // handling OH messages.  We should probably merge them....

        // Local
        if (mLocalForwarder->tryForward(obj_msg)) {
            delete msg;
            return;
        }

        // OSeg Cache
        // 4. Try to shortcut them main thread. Use forwarder to try to forward
        // using the cache. FIXME when we do this, we skip over some checks that
        // happen during the full forwarding
        if (tryCacheForward(obj_msg)) {
            delete msg;
            return;
        }

        // Couldn't get rid of it, forward normally.
        delete obj_msg;
    }

    bool got_empty;
    bool push_success;
    {
        boost::lock_guard<boost::mutex> lock(mReceivedMessagesMutex);
        got_empty = mReceivedMessages.probablyEmpty();
        push_success = mReceivedMessages.push(msg, false);
    }

    if (!push_success) {
        SILOG(forwarder,fatal,"FATAL: Unhandled drop in Forwarder.");
        delete msg;
    }

    if (got_empty && push_success)
        scheduleProcessReceivedServerMessages();
}

void Forwarder::scheduleProcessReceivedServerMessages() {
    mContext->mainStrand->post(
        std::tr1::bind(&Forwarder::processReceivedServerMessages, this)
    );
}

void Forwarder::processReceivedServerMessages() {
#define MAX_RECEIVED_MESSAGES_PROCESSED 20 // need a better way to decide this

    // First, pull out messages we're going to process in this round
    uint32 pulled = 0;
    Message* messages[MAX_RECEIVED_MESSAGES_PROCESSED];
    bool got_empty;
    {
        boost::lock_guard<boost::mutex> lock(mReceivedMessagesMutex);
        while(!mReceivedMessages.probablyEmpty() && pulled < MAX_RECEIVED_MESSAGES_PROCESSED) {
            bool popped = mReceivedMessages.pop( messages[pulled] );
            if (!popped) break;
            pulled++;
        }
        got_empty = mReceivedMessages.probablyEmpty();
    }

    for(uint32 i = 0; i < pulled; i++)
        ServerMessageDispatcher::dispatchMessage(messages[i]);

    if (!got_empty)
        scheduleProcessReceivedServerMessages();
}

} //end namespace
