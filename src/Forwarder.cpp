
#include "Network.hpp"
#include "Server.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerMessageQueue.hpp"
#include "ServerMessageReceiver.hpp"
#include "Statistics.hpp"
#include "Options.hpp"

#include "Forwarder.hpp"
#include "ObjectSegmentation.hpp"
#include "OSegLookupQueue.hpp"

#include "ObjectConnection.hpp"

#include "ForwarderServiceQueue.hpp"

#include "Random.hpp"

#include "ODPFlowScheduler.hpp"
#include "RegionODPFlowScheduler.hpp"

// FIXME we shouldn't have oseg specific things here, this should be delegated
// to OSeg as necessary
#include "CBR_OSeg.pbj.hpp"

#include "CBR_Forwarder.pbj.hpp"

#include <sirikata/network/IOStrandImpl.hpp>

namespace CBR
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
             mOSegLookups(NULL),
             mUniqueConnIDs(0),
             mServiceIDSource(0),
             mServerWeightPoller(
                 ctx->mainStrand,
                 std::tr1::bind(&Forwarder::updateServerWeights, this),
                 Duration::milliseconds((int64)100)),
             mReceivedMessages(std::tr1::bind(&Forwarder::scheduleProcessReceivedServerMessages, this))
{
    mOutgoingMessages = new ForwarderServiceQueue(mContext->id(), 16384, (ForwarderServiceQueue::Listener*)this);

    // Fill in the rest of the context
    mContext->mServerRouter = this;
    mContext->mObjectRouter = this;
    mContext->mServerDispatcher = this;
    mContext->mObjectDispatcher = this;

    mSSTDatagramLayer = BaseDatagramLayer<UUID>::createDatagramLayer(UUID::null(), this, this);


    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
    this->registerMessageRecipient(SERVER_PORT_FORWARDER_WEIGHT_UPDATE, this);

    // Generate router queues for services we provide
    addODPServerMessageService();
    mOSegCacheUpdateRouter = createServerMessageService("oseg-cache-update");
    mForwarderWeightRouter = createServerMessageService("forwarder-weights");
}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
      delete mOSegCacheUpdateRouter;
      delete mForwarderWeightRouter;

      this->unregisterMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
      this->unregisterMessageRecipient(SERVER_PORT_FORWARDER_WEIGHT_UPDATE, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
void Forwarder::initialize(ObjectSegmentation* oseg, ServerMessageQueue* smq, ServerMessageReceiver* smr)
  {
    mOSegLookups = new OSegLookupQueue(mContext->mainStrand, oseg);
    mServerMessageQueue = smq;
    mServerMessageReceiver = smr;
  }

void Forwarder::dispatchMessage(Message*msg) const {
    CONTEXT_TRACE(serverDatagramReceived, mContext->simTime(), msg->source_server(), msg->id(), msg->serializedSize());

    ServerMessageDispatcher::dispatchMessage(msg);
}

void Forwarder::dispatchMessage(const CBR::Protocol::Object::ObjectMessage&msg) const {
    ObjectMessageDispatcher::dispatchMessage(msg);
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

void Forwarder::addODPServerMessageService() {
    using std::tr1::placeholders::_1;
    using std::tr1::placeholders::_2;

    assert(mServiceIDMap.find(ODP_SERVER_MESSAGE_SERVICE) == mServiceIDMap.end());

    ForwarderServiceQueue::ServiceID svc_id = mServiceIDSource++;
    mServiceIDMap[ODP_SERVER_MESSAGE_SERVICE] = svc_id;
    mOutgoingMessages->addService(
        svc_id,
        std::tr1::bind(&Forwarder::createODPFlowScheduler, this, _1, _2)
    );
}

ODPFlowScheduler* Forwarder::createODPFlowScheduler(ServerID remote_server, uint32 max_size) {
    ODPFlowScheduler* new_flow_scheduler =
        new RegionODPFlowScheduler(mContext, mOutgoingMessages, remote_server, mServiceIDMap[ODP_SERVER_MESSAGE_SERVICE], max_size);
    mODPRouters[remote_server] = new_flow_scheduler;
    return new_flow_scheduler;
}

void Forwarder::updateServerWeights() {
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
        CBR::Protocol::Forwarder::WeightUpdate weight_update;

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
            " receiver_total_weight: " << receiver_total_weight <<
            " receiver_capacity: " << receiver_capacity
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
            SILOG(forwarder,warn,"Overflow in forwarder weight message queue!");

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
void Forwarder::routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg) {
    // Messages destined for the space skip the object message queue and just get dispatched
    if (obj_msg->dest_object() == UUID::null()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return;
    }

    bool forwarded = forward(obj_msg);
    if (!forwarded) {
        TIMESTAMP(obj_msg, Trace::DROPPED_DURING_FORWARDING);
        delete obj_msg;
    }
}

// --- From local space server services
bool Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg) {
    msg->set_unique(GenerateUniqueID(mContext->id()));
    return forward(msg, NullServerID);
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
    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    bool parsed = parsePBJMessage(obj_msg, msg->payload());
    assert(parsed);


    TIMESTAMP(obj_msg, Trace::HANDLE_SPACE_MESSAGE);

    UUID dest = obj_msg->dest_object();

    // Special case: Object 0 is the space itself
    if (dest == UUID::null()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return;
    }

    // Otherwise, try to forward it
    bool forward_success = forward(obj_msg, msg->source_server());

    if (!forward_success) {
        TIMESTAMP(obj_msg, Trace::DROPPED_DURING_FORWARDING);
        delete obj_msg;
    }

    delete msg;
}

void Forwarder::receiveWeightUpdateMessage(Message* msg) {
    ServerID source = msg->source_server();

    CBR::Protocol::Forwarder::WeightUpdate weight_update;
    bool parsed = parsePBJMessage(&weight_update, msg->payload());
    assert(parsed);

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

    ODPRouterMap::iterator flow_sched_it = mODPRouters.find(source);
    if (flow_sched_it != mODPRouters.end()) {
        ODPFlowScheduler* serv_flow_sched = flow_sched_it->second;
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

bool Forwarder::forward(CBR::Protocol::Object::ObjectMessage* msg, ServerID forwardFrom)
{
    UUID dest_obj = msg->dest_object();

    TIMESTAMP_START(tstamp, msg);
    TIMESTAMP_END(tstamp, Trace::FORWARDING_STARTED);

    // Check if we can forward locally
    ObjectConnection* conn = getObjectConnection(msg->dest_object());
    if (conn != NULL) {
        TIMESTAMP_END(tstamp, Trace::FORWARDED_LOCALLY_SLOW_PATH);

        bool send_success = false;
        if (conn->enabled())
            send_success = conn->send(msg);

        return send_success;
    }

    // If we can't forward locally, do an OSeg lookup to find out where we need
    // to forward the message to
    TIMESTAMP_END(tstamp, Trace::OSEG_LOOKUP_STARTED);

    bool accepted = mOSegLookups->lookup(
        msg,
        mContext->mainStrand->wrap(
            boost::bind(&Forwarder::routeObjectMessageToServer, this, _1, _2, _3, forwardFrom)
                                   )
                                         );

    return accepted;
}


bool Forwarder::routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* obj_msg, ServerID dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, ServerID forwardFrom)
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
    if (dest_serv == mContext->id()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return true;
    }

  //send out all server updates associated with an object with this message:
  UUID obj_id =  obj_msg->dest_object();

  TIMESTAMP(obj_msg, Trace::SPACE_TO_SPACE_ENQUEUED);

  // Will force allocation of ODPFlowScheduler if its not there already
  mOutgoingMessages->prePush(dest_serv);
  // And then we can actually push
  bool send_success = mODPRouters[dest_serv]->push(obj_msg);
  if (!send_success) {
      TIMESTAMP(obj_msg, Trace::DROPPED_AT_SPACE_ENQUEUED);
  }
  delete obj_msg;

  // Note that this is done *after* the real message is sent since it is an optimization and
  // we don't want it blocking useful traffic
  if (forwardFrom != NullServerID) {
      // FIXME we used to kind of keep track of sending the same OSeg cache fix to a server multiple
      // times, but it really only applied for the rate of lookups/migrations.  We should a) determine
      // if this is actually a problem and b) if it is, take a more principled approach to solving it.
#ifdef CRAQ_DEBUG
      std::cout<<"\n\n bftm debug Sending an oseg cache update message at time:  "<<mContext->simTime().raw()<<"\n";
      std::cout<<"\t for object:  "<<obj_id.toString()<<"\n";
      std::cout<<"\t to server:   "<<mServersToUpdate[obj_id][s]<<"\n";
      std::cout<<"\t obj on:      "<<dest_serv<<"\n\n";
#endif

      CBR::Protocol::OSeg::UpdateOSegMessage contents;
      contents.set_servid_sending_update(mContext->id());
      contents.set_servid_obj_on(dest_serv);
      contents.set_m_objid(obj_id);

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
    Message* next_msg = mOutgoingMessages->front(dest);
    if (next_msg == NULL)
        return NULL;

    CONTEXT_TRACE(serverDatagramQueued, next_msg->dest_server(), next_msg->id(), next_msg->serializedSize());

    Message* pop_msg = mOutgoingMessages->pop(dest);
    assert(pop_msg == next_msg);
    return pop_msg;
}

bool Forwarder::serverMessageEmpty(ServerID dest) {
    return mOutgoingMessages->empty(dest);
}

void Forwarder::serverMessageReceived(Message* msg) {
    assert(msg != NULL);
    TIMESTAMP_PAYLOAD(msg, Trace::SPACE_TO_SPACE_SMR_DEQUEUED);
    mReceivedMessages.push(msg);
}

void Forwarder::scheduleProcessReceivedServerMessages() {
    mContext->mainStrand->post(
        std::tr1::bind(&Forwarder::processReceivedServerMessages, this)
    );
}

void Forwarder::processReceivedServerMessages() {
    while(!mReceivedMessages.empty()) {
        Message* msg;
        mReceivedMessages.pop(msg);
        dispatchMessage(msg);
    }
}

} //end namespace
