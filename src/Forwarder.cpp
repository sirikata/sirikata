
#include "Network.hpp"
#include "Server.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "ServerMessageQueue.hpp"
#include "Statistics.hpp"
#include "Options.hpp"
#include "ObjectMessageQueue.hpp"

#include "ForwarderUtilityClasses.hpp"
#include "Forwarder.hpp"
#include "ObjectSegmentation.hpp"

#include "ObjectConnection.hpp"

#include "Random.hpp"

namespace CBR
{

bool ForwarderQueue::CanSendPredicate::operator() (MessageRouter::SERVICES svc,const OutgoingMessage*msg) {
    return mServerMessageQueue->canAddMessage(msg->dest,msg->data);
}

  /*
    Constructor for Forwarder
  */
Forwarder::Forwarder(SpaceContext* ctx)
 :
   mOutgoingMessages(NULL),
   mContext(ctx),
   mCSeg(NULL),
   mOSeg(NULL),
   mObjectMessageQueue(NULL),
   mServerMessageQueue(NULL),
   mUniqueConnIDs(0),
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
    this->registerMessageRecipient(MESSAGE_TYPE_OBJECT, this);

    mProfiler.addStage("OSeg");
    mProfiler.addStage("Self Messages");
    mProfiler.addStage("Outgoing Messages");
    mProfiler.addStage("Noise");
    mProfiler.addStage("Object Message Queue");
    mProfiler.addStage("Server Message Queue");
    mProfiler.addStage("OSegII");
    mProfiler.addStage("Server Message Queue Receive");


}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
    if (GetOption(PROFILE)->as<bool>())
        mProfiler.report();

    this->unregisterMessageRecipient(MESSAGE_TYPE_OBJECT, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
void Forwarder::initialize(CoordinateSegmentation* cseg, ObjectSegmentation* oseg, ObjectMessageQueue* omq, ServerMessageQueue* smq)
{
  mCSeg = cseg;
  mOSeg = oseg;
  mObjectMessageQueue = omq;
  mServerMessageQueue =smq;
  mOutgoingMessages=new ForwarderQueue(smq,omq,16384);
}

  /*
    Sends a tick to OSeg.  Receives messages from oseg.  Adds them to mOutgoingQueue.
  */
  void Forwarder::tickOSeg(const Time&t)
  {

    std::map<UUID,ServerID> updatedObjectLocations;
    std::map<UUID,ServerID>::iterator iter;
    std::map<UUID,ObjectMessageList>::iterator iterObjectsInTransit;

    std::map<UUID,ObjMessQBeginSendList>::iterator iterQueueMap;

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
        //means that we have to call endSend on the message.
        for (int s=0; s < (signed) ((iterQueueMap->second).size()); ++ s)
        {
          mObjectMessageQueue->endSend(iterQueueMap->second[s],iter->second); //iter->second is the dest_server_id
        }
        queueMap.erase(iterQueueMap);
      }
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
        processChunk(self_messages.front().data, mContext->id(), self_messages.front().forwarded);
      self_messages.pop_front();
    }
    mProfiler.finishedStage();
    (*mOutgoingMessages)->service(); mProfiler.finishedStage();
    while(true)
    {
        uint64 size=1<<30;
        MessageRouter::SERVICES svc;
        OutgoingMessage* next_msg = (*mOutgoingMessages)->front(&size,&svc);
        if (!next_msg)
            break;
        if (!mContext->trace()->timestampMessage(t,Trace::SPACE_OUTGOING_MESSAGE,next_msg->data)) {
            static bool warned=false;
            if (!warned) {
                fprintf (stderr, "shouldn't reach here: trace problem\nPacket has no id\n");
                warned=true;
            }
        }

        bool send_success = mServerMessageQueue->addMessage(next_msg->dest, next_msg->data);
        if (!send_success){
            fprintf(stderr,"shouldnt reach here %s\n",__FILE__);
            break;
        }
        uint64 b=1<<30;
        delete (*mOutgoingMessages)->pop(&b);
    }
    mProfiler.finishedStage();


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
                noise_msg->set_unique(test+rand()%1000000000);
                
                ObjMessQBeginSend beginMess;
                send_success=mObjectMessageQueue->beginSend(noise_msg,beginMess);
                if(send_success) {
                    mContext->trace()->serverDatagramQueued(mContext->time, sid, noise_msg->unique(), 0);
                    mObjectMessageQueue->endSend(beginMess,sid);
                }
            }while (send_success);
        }
    }
    mProfiler.finishedStage();
    // XXXXXXXXXXXXXXXXXXXXXXXX


    mServerMessageQueue->service(); mProfiler.finishedStage();

    tickOSeg(t);  mProfiler.finishedStage();

    Sirikata::Network::Chunk *c=NULL;
    ServerID source_server;
    while(mServerMessageQueue->receive(&c, &source_server))
    {
        processChunk(*c, source_server, false);
        delete c;
    }
    mProfiler.finishedStage();
  }



  // Routing interface for servers.  This is used to route messages that originate from
  // a server provided service, and thus don't have a source object.  Messages may be destined
  // for either servers or objects.  The second form will simply automatically do the destination
  // server lookup.
  // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
  bool Forwarder::route(MessageRouter::SERVICES svc, Message* msg, const ServerID& dest_server, bool is_forward)
  {

      bool success = false;

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset = msg->serialize(msg_serialized, offset);


    if (dest_server==mContext->id())
    {
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
      // FIXME this should be limited in size so we can actually provide pushback to other servers when we can't route to ourselves/
      // connected objects fast enough
      mSelfMessages.push_back( SelfMessage(msg_serialized, is_forward) );
      success = true;
    }
    else
    {

        mContext->trace()->serverDatagramQueued(mContext->time, dest_server, msg->id(), offset);
        QueueEnum::PushResult push_result = (*mOutgoingMessages)->push(svc, new OutgoingMessage(msg_serialized, dest_server) );
        success = (push_result == QueueEnum::PushSucceeded);
    }
    if (success)
        delete msg;
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
    ObjectMessage *om=dynamic_cast<ObjectMessage*>(msg);
    if (om) {
        mContext->trace()->timestampMessage(mContext->time,om->contents.unique(),Trace::DISPATCHED,om->contents.source_port(),om->contents.dest_port(),msg->type());
    }
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

    ObjMessQBeginSend beginMess;
    bool send_success = mObjectMessageQueue->beginSend(obj_msg, beginMess);

    if (send_success)
    {
      //      mOSeg->lookup(beginMess.dest_uuid);
      ServerID lookupID = mOSeg->lookup(beginMess.dest_uuid);

      if (lookupID == NullServerID)
      {
        if (queueMap.find(beginMess.dest_uuid) == queueMap.end())
        {
            //nothing is in queueMap
            ObjMessQBeginSendList tmpList;
            tmpList.push_back(beginMess);
            queueMap[beginMess.dest_uuid] = tmpList;
        }
        else
        {
            queueMap[beginMess.dest_uuid].push_back(beginMess);
        }
      }
      else
      {
          mObjectMessageQueue->endSend(beginMess,lookupID);
      }
    }

    return send_success;
}


  void Forwarder::processChunk(const Network::Chunk&chunk, const ServerID& source_server, bool forwarded_self_msg)
  {
    Message* result;
    uint32 offset=0;

    do
    {
      offset = Message::deserialize(chunk,offset,&result);
      ObjectMessage *om=dynamic_cast<ObjectMessage*>(result);
      if (om) {
          mContext->trace()->timestampMessage(mContext->time,om->contents.unique(),Trace::FORWARDED,om->contents.source_port(),om->contents.dest_port(),result->type());
      }
      if (!forwarded_self_msg)
          mContext->trace()->serverDatagramReceived(mContext->time, mContext->time, source_server, result->id(), offset);

      deliver(result);

      //if (delivered)
      //  mContext->trace()->serverDatagramReceived(mContext->time, mContext->time, source_server, result->id(), offset);
    }while (offset<chunk.size());
  }



  // Delivery interface.  This should be used to deliver received messages to the correct location -
  // the server or object it is addressed to.
  void Forwarder::deliver(Message* msg)
  {
    switch(msg->type()) {
      case MESSAGE_TYPE_OBJECT:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_NOISE:
          {

              NoiseMessage* noise_msg = dynamic_cast<NoiseMessage*>(msg);
              assert(noise_msg != NULL);
              delete noise_msg;
          }
        break;
      case MESSAGE_TYPE_MIGRATE:
          {
            dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_CSEG_CHANGE:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_LOAD_STATUS:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_OSEG_MIGRATE_MOVE:
        {
          dispatchMessage(msg);
        }
        break;
      case MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE:
        {
          dispatchMessage(msg);
        }
        break;
      case MESSAGE_TYPE_KILL_OBJ_CONN:
        {
          dispatchMessage(msg);
        }
        break;
      case MESSAGE_TYPE_UPDATE_OSEG:
        {
          dispatchMessage(msg);
        }
        break;
      case MESSAGE_TYPE_OSEG_ADDED_OBJECT:
        {
          dispatchMessage(msg);
        }
        break;
      case MESSAGE_TYPE_SERVER_PROX_QUERY:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_SERVER_PROX_RESULT:
          {
              dispatchMessage(msg);
          }
          break;
      case MESSAGE_TYPE_BULK_LOCATION:
          {
              dispatchMessage(msg);
          }
          break;
      default:
        assert(false);
        break;
    }
  }


void Forwarder::receiveMessage(Message* msg) {
    // Forwarder only subscribes as a recipient for object messages
    // so it can easily check whether it can deliver directly
    // or needs to forward them.
    assert(msg->type() == MESSAGE_TYPE_OBJECT);
    ObjectMessage* obj_msg = dynamic_cast<ObjectMessage*>(msg);
    assert(obj_msg != NULL);

    UUID dest = obj_msg->contents.dest_object();

    // Special case: Object 0 is the space itself
    if (dest == UUID::null()) {
        dispatchMessage(obj_msg->contents);
        return;
    }

    // Otherwise, either deliver or forward it, depending on whether the destination object is attached to this server
    // FIXME having to make this copy is nasty
    CBR::Protocol::Object::ObjectMessage* obj_msg_cpy = new CBR::Protocol::Object::ObjectMessage(obj_msg->contents);
    ObjectConnection* conn = getObjectConnection(dest);
    if (conn != NULL) {
        // This should probably have control via push back as well, i.e. should return bool and we should care what
        // happens
        conn->send(obj_msg_cpy);
    }
    else
    {
        // FIXME we need to check the result here. There needs to be a way to push back, which is probably trickiest here
        // since it would have to filter all the way back to where the original server to server message was decoded
      route(obj_msg_cpy, true,GetUniqueIDServerID(obj_msg->id()) );// bftm changed to allow for forwarding back messages.
    }

    //    else
    //        route(obj_msg_cpy, true);

    delete msg;
}



bool Forwarder::routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, bool is_forward, MessageRouter::SERVICES svc)
{
  //send out all server updates associated with an object with this message:
  UUID obj_id =  msg->dest_object();


  //#ifdef CRAQ_CACHE
  if (mServersToUpdate.find(obj_id) != mServersToUpdate.end())
  {
    for (int s=0; s < (int) mServersToUpdate[obj_id].size(); ++s)
    {
      UpdateOSegMessage* up_os = new UpdateOSegMessage(mContext->id(),dest_serv,obj_id);
      //      route(MessageRouter::OSEG_CACHE_UPDATE, up_os, mServersToUpdate[obj_id][s],false);

#ifdef CRAQ_DEBUG
      std::cout<<"\n\n bftm debug Sending an oseg cache update message at time:  "<<mContext->time.raw()<<"\n";
      std::cout<<"\t for object:  "<<obj_id.toString()<<"\n";
      std::cout<<"\t to server:   "<<mServersToUpdate[obj_id][s]<<"\n";
      std::cout<<"\t obj on:      "<<dest_serv<<"\n\n";
#endif

      // FIXME what if this fails to get sent out?
      route(MessageRouter::OSEG_CACHE_UPDATE, up_os, mServersToUpdate[obj_id][s],false);

    }
  }
  //#endif

  // Wrap it up in one of our ObjectMessages and ship it.
  ObjectMessage* obj_msg = new ObjectMessage(mContext->id(), *msg);  //turns the cbr::prot::message into just an object message.
  return route(svc,obj_msg, dest_serv, is_forward);
}


void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
  UniqueObjConn uoc;
  uoc.id = mUniqueConnIDs;
  uoc.conn = conn;
  ++mUniqueConnIDs;


  //    mObjectConnections[dest_obj] = conn;
  mObjectConnections[dest_obj] = uoc;
  mObjectMessageQueue->registerClient(dest_obj, 1); // FIXME weight?
}

ObjectConnection* Forwarder::removeObjectConnection(const UUID& dest_obj) {
    mObjectMessageQueue->unregisterClient(dest_obj);
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
