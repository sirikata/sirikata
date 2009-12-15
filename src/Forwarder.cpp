
#include "Network.hpp"
#include "Server.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "ServerMessageQueue.hpp"
#include "Statistics.hpp"
#include "Options.hpp"

#include "Forwarder.hpp"
#include "ObjectSegmentation.hpp"
#include "OSegLookupQueue.hpp"

#include "ObjectConnection.hpp"

#include "Random.hpp"

namespace CBR
{

bool AlwaysPush(const UUID&, size_t cursize , size_t totsize) {return true;}

/** Automatically samples and logs current queue information for the ServerMessageQueue. */
class ForwarderSampler : public PollingService {
public:
    ForwarderSampler(SpaceContext* ctx, const Duration& rate, ServerMessageQueue* smq)
     : PollingService(ctx->mainStrand, rate),
       mContext(ctx),
       mServerMessageQueue(smq)
    {
    }
private:
    virtual void poll() {
        mServerMessageQueue->reportQueueInfo(mContext->time);

    }

    SpaceContext* mContext;
    ServerMessageQueue* mServerMessageQueue;
}; // class Sampler



  /*
    Constructor for Forwarder
  */
Forwarder::Forwarder(SpaceContext* ctx)
 : PollingService(ctx->mainStrand),
   mContext(ctx),
   mOutgoingMessages(NULL),
   mServerMessageQueue(NULL),
   mOSegLookups(NULL),
   mSampler(NULL),
   mUniqueConnIDs(0)
{
    //no need to initialize mOutgoingMessages.

    // Fill in the rest of the context
    mContext->mRouter = this;
    mContext->mDispatcher = this;

    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
    this->registerMessageRecipient(SERVER_PORT_NOISE, this);

    mNoiseStage = mContext->profiler->addStage("Noise");
    mForwarderQueueStage = mContext->profiler->addStage("Forwarder Queue");
    mReceiveStage = mContext->profiler->addStage("Forwarder Receive");
}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
      delete mSampler;

      this->unregisterMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
      this->unregisterMessageRecipient(SERVER_PORT_NOISE, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
  void Forwarder::initialize(ObjectSegmentation* oseg, ServerMessageQueue* smq, uint32 oseg_lookup_queue_size)
  {
    mOSegLookups = new OSegLookupQueue(mContext->mainStrand, oseg, &AlwaysPush, oseg_lookup_queue_size);
    mServerMessageQueue = smq;
    mOutgoingMessages = new ForwarderQueue(smq,16384);

    Duration sample_rate = GetOption(STATS_SAMPLE_RATE)->as<Duration>();
    mSampler = new ForwarderSampler(mContext, sample_rate, mServerMessageQueue);
    mContext->add(mSampler);
  }

void Forwarder::poll()
{
    mForwarderQueueStage->started();
    for (uint32 sid=0;sid<mOutgoingMessages->numServerQueues();++sid) {
        while(true)
        {

            uint64 size=1<<30;
            MessageRouter::SERVICES svc;
            Message* next_msg = mOutgoingMessages->getFairQueue(sid).front(&size,&svc);
            if (!next_msg)
                break;

            if (!mServerMessageQueue->canSend(next_msg))
                break;

            mContext->trace()->serverDatagramQueued(mContext->time, next_msg->dest_server(), next_msg->id(), next_msg->serializedSize());
            bool send_success = mServerMessageQueue->addMessage(next_msg);
            if (!send_success)
                break;

            Message* pop_msg = mOutgoingMessages->getFairQueue(sid).pop(&size);
            assert(pop_msg == next_msg);
        }
    }
    mForwarderQueueStage->finished();

    mNoiseStage->started();
    if (GetOption(NOISE)->as<bool>()) {
        for(ServerMessageQueue::KnownServerIterator it = mServerMessageQueue->knownServersBegin(); it != mServerMessageQueue->knownServersEnd(); it++) {
            ServerID sid = *it;
            if (sid == mContext->id()) continue;
            while(true) {
                std::string randnoise;
                randnoise.resize((size_t)(50 + 200*randFloat()));
                Message* noise_msg = new Message(mContext->id(),SERVER_PORT_NOISE,sid, SERVER_PORT_NOISE, randnoise); // FIXME control size from options?

                bool sent_success = mServerMessageQueue->addMessage(noise_msg);
                if (sent_success) {
                        mContext->trace()->serverDatagramQueued(mContext->time, sid, noise_msg->id(), 0);
                }else {
                    delete noise_msg;
                }
                if (!sent_success) break;
            }
        }
    }
    mNoiseStage->finished();
    // Try to push things from the server message queues down to the network
    mServerMessageQueue->service();

    mReceiveStage->started();
    Message* next_msg = NULL;
    while(mServerMessageQueue->receive(&next_msg)) {
        processChunk(next_msg, false);
    }
    mReceiveStage->finished();
  }



  // Routing interface for servers.  This is used to route messages that originate from
  // a server provided service, and thus don't have a source object.  Messages may be destined
  // for either servers or objects.  The second form will simply automatically do the destination
  // server lookup.
  // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
  bool Forwarder::route(MessageRouter::SERVICES svc, Message* msg, bool is_forward)
  {
      assert(msg->source_server() == mContext->id());
      assert(msg->dest_server() != NullServerID);

      ServerID dest_server = msg->dest_server();

      bool success = false;

    if (dest_server==mContext->id())
    {
        processChunk(msg, is_forward);
        success = true;
    }
    else
    {
        QueueEnum::PushResult push_result = mOutgoingMessages->getFairQueue(dest_server).push(svc, msg);
        success = (push_result == QueueEnum::PushSucceeded);
    }
    return success;
  }

  bool Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward, ServerID forwardFrom)
  {
      UUID dest_obj = msg->dest_object();

      TIMESTAMP_START(tstamp, msg);

      TIMESTAMP_END(tstamp, Trace::OSEG_LOOKUP_STARTED);

      bool accepted = mOSegLookups->lookup(
          msg,
          boost::bind(&Forwarder::routeObjectMessageToServer, this, _1, _2, _3, is_forward, forwardFrom)
      );


      if (!accepted) {
          TIMESTAMP_END(tstamp, Trace::DROPPED);
      }

      return accepted;
  }

//end what i think it should be replaced with

void Forwarder::dispatchMessage(Message*msg) const {
    MessageDispatcher::dispatchMessage(msg);
}
void Forwarder::dispatchMessage(const CBR::Protocol::Object::ObjectMessage&msg) const {
    MessageDispatcher::dispatchMessage(msg);
}
bool Forwarder::routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg) {
    // Messages destined for the space skip the object message queue and just get dispatched
    if (obj_msg->dest_object() == UUID::null()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return true;
    }

    return route(obj_msg, false, NullServerID);
}


void Forwarder::processChunk(Message* msg, bool forwarded_self_msg) {
    if (!forwarded_self_msg)
        mContext->trace()->serverDatagramReceived(mContext->time, mContext->time, msg->source_server(), msg->id(), msg->serializedSize());

    dispatchMessage(msg);
}

void Forwarder::receiveMessage(Message* msg) {
    if (msg->dest_port() == SERVER_PORT_NOISE) {
        delete msg;
        return;
    }

    // Forwarder only subscribes as a recipient for object messages
    // so it can easily check whether it can deliver directly
    // or needs to forward them.
    assert(msg->dest_port() == SERVER_PORT_OBJECT_MESSAGE_ROUTING);

    CBR::Protocol::Object::ObjectMessage* obj_msg = new CBR::Protocol::Object::ObjectMessage();
    bool parsed = parsePBJMessage(obj_msg, msg->payload());
    assert(parsed);

    UUID dest = obj_msg->dest_object();

    // Special case: Object 0 is the space itself
    if (dest == UUID::null()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return;
    }

    // Otherwise, either deliver or forward it, depending on whether the destination object is attached to this server
    ObjectConnection* conn = getObjectConnection(dest);
    if (conn != NULL) {
        TIMESTAMP_START(tstamp, obj_msg);

        bool send_success = true;

        if (!conn->enabled())
            send_success = false;
        else
            send_success = conn->send(obj_msg);

        if (!send_success) {
            TIMESTAMP_END(tstamp, Trace::DROPPED);
        }

        if (!send_success)
            delete obj_msg;
    }
    else
    {
        // FIXME we need to check the result here. There needs to be a way to push back, which is probably trickiest here
        // since it would have to filter all the way back to where the original server to server message was decoded
        route(obj_msg, true, msg->source_server());// bftm changed to allow for forwarding back messages.
    }

    delete msg;
}


bool Forwarder::routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* obj_msg, ServerID dest_serv, OSegLookupQueue::ResolvedFrom resolved_from, bool is_forward, ServerID forwardFrom)
{
    Trace::MessagePath mp = (resolved_from == OSegLookupQueue::ResolvedFromCache)
        ? Trace::OSEG_CACHE_LOOKUP_FINISHED
        : Trace::OSEG_SERVER_LOOKUP_FINISHED;

    // Timestamp the message as having finished an OSeg Lookup
    TIMESTAMP(obj_msg, mp);

  //send out all server updates associated with an object with this message:
  UUID obj_id =  obj_msg->dest_object();

  Message* svr_obj_msg = new Message(
      mContext->id(),
      SERVER_PORT_OBJECT_MESSAGE_ROUTING,
      dest_serv,
      SERVER_PORT_OBJECT_MESSAGE_ROUTING,
      serializePBJMessage(*obj_msg)
  );
  bool send_success=route(OBJECT_MESSAGESS, svr_obj_msg, is_forward);
  if (!send_success) {
      delete svr_obj_msg;
      TIMESTAMP(obj_msg, Trace::DROPPED);
  }else {
      delete obj_msg;
  }

  // Note that this is done *after* the real message is sent since it is an optimization and
  // we don't want it blocking useful traffic
  if (forwardFrom != NullServerID) {
      // FIXME we used to kind of keep track of sending the same OSeg cache fix to a server multiple
      // times, but it really only applied for the rate of lookups/migrations.  We should a) determine
      // if this is actually a problem and b) if it is, take a more principled approach to solving it.
#ifdef CRAQ_DEBUG
      std::cout<<"\n\n bftm debug Sending an oseg cache update message at time:  "<<mContext->time.raw()<<"\n";
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

      // FIXME what if this fails to get sent out?
      route(MessageRouter::OSEG_CACHE_UPDATE, up_os, false);
  }

  return send_success;
}


void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
  UniqueObjConn uoc;
  uoc.id = mUniqueConnIDs;
  uoc.conn = conn;
  ++mUniqueConnIDs;


  //    mObjectConnections[dest_obj] = conn;
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



} //end namespace
