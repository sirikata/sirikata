
#include "Network.hpp"
#include "Server.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "ServerMessageQueue.hpp"
#include "Statistics.hpp"
#include "Options.hpp"

#include "ForwarderUtilityClasses.hpp"
#include "Forwarder.hpp"
#include "ObjectSegmentation.hpp"

#include "ObjectConnection.hpp"

#include "Random.hpp"

namespace CBR
{

// Helper method which timestamps a message if it is and object-to-object message. Note that this is potentially expensive since it requires parsing the inner message
void tryTimestampObjectMessage(Trace* trace, const Time& t, Message* next_msg, Trace::MessagePath path) {
    if (next_msg->source_port() == SERVER_PORT_OBJECT_MESSAGE_ROUTING && next_msg->dest_port() == SERVER_PORT_OBJECT_MESSAGE_ROUTING) {
        CBR::Protocol::Object::ObjectMessage om;
        bool got_om = parsePBJMessage(&om, next_msg->payload());
        if (got_om)
            trace->timestampMessage(t, om.unique(), path, om.source_port(), om.dest_port(), SERVER_PORT_OBJECT_MESSAGE_ROUTING);
    }
}

void OSegLookupQueue::ObjMessQBeginSendList::push_back(CBR::Protocol::Object::ObjectMessage* msg) {
    mTotalSize += msg->ByteSize();
    ObjectMessageVector::push_back(msg);
}

void OSegLookupQueue::push(UUID objid, CBR::Protocol::Object::ObjectMessage* dat) {
    size_t cursize = dat->ByteSize();
    if (mPredicate(objid,cursize,mTotalSize)) {
        mTotalSize+=cursize;
        mObjects[objid].push_back(dat);
    }
}


bool ForwarderQueue::CanSendPredicate::operator() (MessageRouter::SERVICES svc, const Message*msg) {
    return mServerMessageQueue->canAddMessage(msg);
}


bool AlwaysPush(const UUID&, size_t cursize , size_t totsize) {return true;}
  /*
    Constructor for Forwarder
  */
Forwarder::Forwarder(SpaceContext* ctx)
 :
   mOutgoingMessages(NULL),
   mContext(ctx),
   mCSeg(NULL),
   mOSeg(NULL),
   mServerMessageQueue(NULL),
   mUniqueConnIDs(0),
   queueMap(&AlwaysPush),
   mLastSampleTime(Time::null()),
   mSampleRate( GetOption(STATS_SAMPLE_RATE)->as<Duration>() ),
   mProfiler("Forwarder Loop")
{
    //no need to initialize mSelfMessages and mOutgoingMessages.

    // Fill in the rest of the context
    mContext->mRouter = this;
    mContext->mDispatcher = this;

    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
    this->registerMessageRecipient(SERVER_PORT_NOISE, this);

    mProfiler.addStage("OSeg");
    mProfiler.addStage("Self Messages");
    mProfiler.addStage("Noise");
    mProfiler.addStage("Outgoing Messages");
    mProfiler.addStage("Forwarder Queue => Server Message Queue");
    mProfiler.addStage("Server Message Queue");
    mProfiler.addStage("OSegII");
    mProfiler.addStage("Server Message Queue Receive");


}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
    if (GetOption(PROFILE)->as<bool>())
        mProfiler.report();

    this->unregisterMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
    this->unregisterMessageRecipient(SERVER_PORT_NOISE, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
void Forwarder::initialize(CoordinateSegmentation* cseg, ObjectSegmentation* oseg, ServerMessageQueue* smq)
{
  mCSeg = cseg;
  mOSeg = oseg;
  mServerMessageQueue =smq;
  mOutgoingMessages=new ForwarderQueue(smq,16384);
}

  /*
    Sends a tick to OSeg.  Receives messages from oseg.  Adds them to mOutgoingQueue.
  */
  void Forwarder::tickOSeg(const Time&t)
  {

    std::map<UUID,ServerID> updatedObjectLocations;
    std::map<UUID,ServerID>::iterator iter;
    std::map<UUID,ObjectMessageList>::iterator iterObjectsInTransit;

    OSegLookupQueue::ObjectMap::iterator iterQueueMap;

    mOSeg->service(updatedObjectLocations);

    //    cross-check updates against held messages.
    for (iter = updatedObjectLocations.begin();  iter != updatedObjectLocations.end(); ++iter)
    {
      iterObjectsInTransit = mObjectsInTransit.find(iter->first);
      if (iterObjectsInTransit != mObjectsInTransit.end())
      {
        //means that we can route messages being held in mObjectsInTransit
        for (int s=0; s < (signed)((iterObjectsInTransit->second).size()); ++s)
        {
            // FIXME what if this fails?
            routeObjectMessageToServer((iterObjectsInTransit->second)[s].mMessage,iter->second,(iterObjectsInTransit->second)[s].mIsForward,MessageRouter::OSEG_TO_OBJECT_MESSAGESS);
        }

        //remove the messages from the objects in transit
        mObjectsInTransit.erase(iterObjectsInTransit);
      }

      //Now sending messages that we had saved up from oseg lookup calls.
      iterQueueMap = queueMap.find(iter->first);
      if (iterQueueMap != queueMap.end())
      {
          ServerID dest_server = iter->second;

        for (int s=0; s < (signed) ((iterQueueMap->second).size()); ++ s)
        {
            CBR::Protocol::Object::ObjectMessage* in_msg = (iterQueueMap->second[s]);
            Message* obj_msg = new Message(
                mContext->id(),
                SERVER_PORT_OBJECT_MESSAGE_ROUTING,
                dest_server,
                SERVER_PORT_OBJECT_MESSAGE_ROUTING,
                serializePBJMessage(*in_msg)
            );
            if (!route(MessageRouter::OBJECT_MESSAGESS, obj_msg)) {
                mContext->trace()->timestampMessage(mContext->time,
                    in_msg->unique(),
                    Trace::DROPPED,
                    in_msg->source_port(),
                    in_msg->dest_port(),
                    SERVER_PORT_OBJECT_MESSAGE_ROUTING
                );
                delete obj_msg;
            }
            // FIXME should we be deleting in_msg here?
        }
        queueMap.erase(iterQueueMap);
      }
    }
  }


/** This is a helper method which is used as a callback when popping an element off the ForwarderQueue, to
 *  guarantee that we don't screw up the fair queue by mucking with the ServerMessageQueue after it has computed
 *  a new front value.
 */
static void push_to_smq(ServerMessageQueue* smq, SpaceContext* ctx, Message* expected, Message* result) {
    assert(expected == result);
    ctx->trace()->serverDatagramQueued(ctx->time, result->dest_server(), result->id(), result->serializedSize());
    bool send_success = smq->addMessage(result);
    if (!send_success) {
        SILOG(cbr,fatal,"Push to ServerMessageQueueFailed.  Probably indicates that the predicate is incorrect.");
        delete result;
    }
}

void Forwarder::service()
{
    Time t = mContext->time;

    if (t - mLastSampleTime > mSampleRate)
    {
          mServerMessageQueue->reportQueueInfo(t);
          mLastSampleTime = t;
    }

    mProfiler.startIteration();

    tickOSeg(t);  mProfiler.finishedStage();

    std::deque<SelfMessage> self_messages;
    self_messages.swap( mSelfMessages );
    while (!self_messages.empty())
    {
        processChunk(self_messages.front().msg, self_messages.front().forwarded);
        self_messages.pop_front();
    }
    mProfiler.finishedStage();
/*

    // XXXXXXXXXXXXXXXXXXXXXX Generate noise
    std::string noiseChunk;
    if (GetOption(NOISE)->as<bool>()) {
        static UUID key=UUID::random();
        static UUID zerodkey=key;
        ServerID nilo=0;
        memcpy(&zerodkey,&nilo,sizeof(nilo));
        for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
            if (sid == mContext->id()) continue;
            memcpy(&key,&sid,sizeof(sid));
            if (!mObjectMessageQueue->hasClient(key)) {
                mObjectMessageQueue->registerClient(key,1.0e-6);
            }
            bool send_success;
            do {
                Protocol::Object::ObjectMessage* noise_msg = new Protocol::Object::ObjectMessage;
                noiseChunk.resize(50 + 200*randFloat());
                noise_msg->set_payload(noiseChunk);
                noise_msg->set_source_port(OBJECT_PORT_NOISE);
                noise_msg->set_dest_port(OBJECT_PORT_NOISE);

                noise_msg->set_source_object(key);
                noise_msg->set_dest_object(UUID::null());///bizzzzogus
                uint64 test=(1<<30);
                test<<=30;
                noise_msg->set_unique(GenerateUniqueID(mContext->id()));

                ObjMessQBeginSend beginMess;
                send_success=mObjectMessageQueue->beginSend(noise_msg,beginMess);
                if(send_success) {
                    mObjectMessageQueue->endSend(beginMess,sid);
                }
            }while (send_success);
        }
    }
    mProfiler.finishedStage();
    // XXXXXXXXXXXXXXXXXXXXXXXX
    */
    //FIXME do we really need to call service? 
    for (int sid=0;sid<mOutgoingMessages->numServerQueues();++sid) {
        mOutgoingMessages->getFairQueue(sid).service();
        while(true)
        {
            
            uint64 size=1<<30;
            MessageRouter::SERVICES svc;
            Message* next_msg = mOutgoingMessages->getFairQueue(sid).front(&size,&svc);
            if (!next_msg)
                break;
            
            if (!mServerMessageQueue->canSend(next_msg))
                break;

            tryTimestampObjectMessage(mContext->trace(), mContext->time, next_msg, Trace::SPACE_OUTGOING_MESSAGE);
            
            Message* pop_msg = mOutgoingMessages->getFairQueue(sid).pop(
                &size,
                std::tr1::bind(push_to_smq,
                               mServerMessageQueue,
                               mContext,
                               next_msg,
                               std::tr1::placeholders::_1)
                );
        }
    }
    mProfiler.finishedStage();

    // Try to push things from the server message queues down to the network
    mServerMessageQueue->service(); mProfiler.finishedStage();

    tickOSeg(t);  mProfiler.finishedStage();

    Message* next_msg = NULL;
    while(mServerMessageQueue->receive(&next_msg)) {
        processChunk(next_msg, false);
    }
    mProfiler.finishedStage();
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
/* Because these are just routed out to the object host or to the appropriate service, we don't record datagram queued and sent -- they effectively don't happen since they bypass
      any server-to-server queues.
      if (!is_forward)
      {
          mContext->trace()->serverDatagramQueued(mContext->time, dest_server, msg->id(), offset);
          mContext->trace()->serverDatagramSent(mContext->time, mContext->time, 0 , dest_server, msg->id(), offset); // self rate is infinite => start and end times are identical
      }
      else
      {
          // The more times the message goes through a forward to self loop, the more times this will record, causing an explosion
          // in trace size.  It's probably not worth recording this information...
          //mContext->trace()->serverDatagramQueued(mContext->time, dest_server, msg->id(), offset);
      }
*/
      // FIXME this should be limited in size so we can actually provide pushback to other servers when we can't route to ourselves/
      // connected objects fast enough
      mSelfMessages.push_back( SelfMessage(msg, is_forward) );
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
    ServerID dest_server_id = mOSeg->lookup(dest_obj);


    //#ifdef  CRAQ_CACHE
    if (is_forward && (forwardFrom != NullServerID))
    {
      //means that when we figure out which server this object is located on, we should also send an oseg update message.
      //we need to be careful not to send multiple update messages to the same server:
      //

      if (mServersToUpdate.find(dest_obj) != mServersToUpdate.end()) //if the object already exists in mServersToUpdate
      {
        if (mServersToUpdate[dest_obj].end() == std::find(mServersToUpdate[dest_obj].begin(), mServersToUpdate[dest_obj].end(), forwardFrom))  //if the server that we want to update doesn't already exist for that entry of dest_obj
        {
          //don't already know that forwardFrom needs to be updated.  means that we should append the forwardFrom ServerID to a list of servers that we need to update when we know which server to send the message to.
          mServersToUpdate[dest_obj].push_back(forwardFrom);

#ifdef CRAQ_DEBUG
          std::cout<<"\n\n bftm debug: got a server to update at time  "<<mContext->time.raw()<< " obj_id:   "<<dest_obj.toString()<<"    server to send update to:  "<<forwardFrom<<"\n\n";
#endif

        }
      }
      else
      {
        //the object doesn't already exist in mServersToUpdate: put it in mServersToUpdate
        mServersToUpdate[dest_obj].push_back(forwardFrom);
      }
    }
    //#endif



    if (dest_server_id != NullServerID)
    {
      //means that we instantly knew what the location of the object is, and we can route immediately!
        return routeObjectMessageToServer(msg, dest_server_id,is_forward,MessageRouter::SERVER_TO_OBJECT_MESSAGESS);
    }

    // FIXME we need to handle this delay somewhere else. We can't just queue up an arbitrarily large number of messages
    // waiting for oseg lookups...
    //add message to objects in transit.
    MessageAndForward mAndF;
    mAndF.mMessage = msg;
    mAndF.mIsForward = is_forward;


    mObjectsInTransit[dest_obj].push_back(mAndF);
    return true;
  }

//end what i think it should be replaced with

void Forwarder::dispatchMessage(Message*msg) const {
    tryTimestampObjectMessage(mContext->trace(), mContext->time, msg, Trace::DISPATCHED);
    MessageDispatcher::dispatchMessage(msg);
}
void Forwarder::dispatchMessage(const CBR::Protocol::Object::ObjectMessage&msg) const {
    mContext->trace()->timestampMessage(mContext->time,msg.unique(),Trace::DISPATCHED,0,0);
    MessageDispatcher::dispatchMessage(msg);
}
bool Forwarder::routeObjectHostMessage(CBR::Protocol::Object::ObjectMessage* obj_msg) {
    // Messages destined for the space skip the object message queue and just get dispatched
    if (obj_msg->dest_object() == UUID::null()) {
        dispatchMessage(*obj_msg);
        delete obj_msg;
        return true;
    }

    ServerID lookupID = mOSeg->lookup(obj_msg->dest_object());
    if (lookupID == NullServerID)
    {
        queueMap[obj_msg->dest_object()].push_back(obj_msg);
        //if queueMap.full() send_success=false;
        return true;
        // FIXME what if the queueMap is full? Do we need to timestamp as below?
    }
    else
    {
        Message* svr_obj_msg = new Message(
            mContext->id(),
            SERVER_PORT_OBJECT_MESSAGE_ROUTING,
            lookupID,
            SERVER_PORT_OBJECT_MESSAGE_ROUTING,
            serializePBJMessage(*obj_msg)
        );
        bool send_success=route(OBJECT_MESSAGESS, svr_obj_msg, lookupID);
        if (!send_success) {
            delete svr_obj_msg;
            mContext->trace()->timestampMessage(mContext->time,
                obj_msg->unique(),
                Trace::DROPPED,
                obj_msg->source_port(),
                obj_msg->dest_port(),
                SERVER_PORT_OBJECT_MESSAGE_ROUTING);
        }
        delete obj_msg;
        return send_success;
    }
}


void Forwarder::processChunk(Message* msg, bool forwarded_self_msg) {
    tryTimestampObjectMessage(mContext->trace(), mContext->time, msg, Trace::FORWARDED);

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
        // This should probably have control via push back as well, i.e. should return bool and we should care what
        // happens
        conn->send(obj_msg);
    }
    else
    {
        // FIXME we need to check the result here. There needs to be a way to push back, which is probably trickiest here
        // since it would have to filter all the way back to where the original server to server message was decoded
        route(obj_msg, true, msg->source_server());// bftm changed to allow for forwarding back messages.
    }

    delete msg;
}



bool Forwarder::routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, bool is_forward, MessageRouter::SERVICES svc)
{
  //send out all server updates associated with an object with this message:
  UUID obj_id =  msg->dest_object();


  //#ifdef CRAQ_CACHE
  CBR::Protocol::OSeg::UpdateOSegMessage contents;
  contents.set_servid_sending_update(mContext->id());
  contents.set_servid_obj_on(dest_serv);
  contents.set_m_objid(obj_id);

  if (mServersToUpdate.find(obj_id) != mServersToUpdate.end())
  {
    for (int s=0; s < (int) mServersToUpdate[obj_id].size(); ++s)
    {
#ifdef CRAQ_DEBUG
      std::cout<<"\n\n bftm debug Sending an oseg cache update message at time:  "<<mContext->time.raw()<<"\n";
      std::cout<<"\t for object:  "<<obj_id.toString()<<"\n";
      std::cout<<"\t to server:   "<<mServersToUpdate[obj_id][s]<<"\n";
      std::cout<<"\t obj on:      "<<dest_serv<<"\n\n";
#endif

        Message* up_os = new Message(
            mContext->id(),
            SERVER_PORT_OSEG_UPDATE,
            mServersToUpdate[obj_id][s],
            SERVER_PORT_OSEG_UPDATE,
            serializePBJMessage(contents)
        );

      // FIXME what if this fails to get sent out?
      route(MessageRouter::OSEG_CACHE_UPDATE, up_os, false);

    }
  }
  //#endif

  // Wrap it up in one of our ObjectMessages and ship it.
  Message* obj_msg = new Message(
      mContext->id(),
      SERVER_PORT_OBJECT_MESSAGE_ROUTING,
      dest_serv,
      SERVER_PORT_OBJECT_MESSAGE_ROUTING,
      serializePBJMessage(*msg)
  );
  bool route_success = route(svc, obj_msg, is_forward);
  // FIXME record drop? ignore? queue for later?
  if (!route_success)
      delete obj_msg;
  return route_success;
}


void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
  UniqueObjConn uoc;
  uoc.id = mUniqueConnIDs;
  uoc.conn = conn;
  ++mUniqueConnIDs;


  //    mObjectConnections[dest_obj] = conn;
  mObjectConnections[dest_obj] = uoc;
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
    //    return (it == mObjectConnections.end()) ? NULL : it->second;
    return (it == mObjectConnections.end()) ? NULL : it->second.conn;
}


ObjectConnection* Forwarder::getObjectConnection(const UUID& dest_obj, uint64& ider )
{
    ObjectConnectionMap::iterator it = mObjectConnections.find(dest_obj);
    //    return (it == mObjectConnections.end()) ? NULL : it->second;
    if (it == mObjectConnections.end())
    {
      ider = 0;
      return NULL;
    }
    ider = it->second.id;
    return it->second.conn;
}



} //end namespace
