
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
          route((iterObjectsInTransit->second)[s].mMessage,iter->second,(iterObjectsInTransit->second)[s].mIsForward);
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

    while(true)
    {
        uint64 size=1<<30;
        MessageRouter::SERVICES svc;
        OutgoingMessage* next_msg = (*mOutgoingMessages)->front(&size,&svc);
        if (!next_msg)
            break;
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

    if (GetOption(NOISE)->as<bool>()) {
        if (GetOption(SERVER_QUEUE)->as<String>() == "fair") {
            for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
                if (sid == mContext->id()) continue;
                while(true) {
                    NoiseMessage* noise_msg = new NoiseMessage(mContext->id(), (uint32)(50 + 200*randFloat())); // FIXME control size from options?

                    uint32 offset = 0;
                    Network::Chunk msg_serialized;
                    offset = noise_msg->serialize(msg_serialized, offset);

                    bool sent_success = mServerMessageQueue->addMessage(sid, msg_serialized);
                    if (sent_success)
                        mContext->trace()->serverDatagramQueued(mContext->time, sid, noise_msg->id(), offset);
                    delete noise_msg;
                    if (!sent_success) break;
                }
            }
        }
        else {
            // For FIFO we generate for random servers because they all
            // share a single internal queue.
            uint32 nfail = 0;
            uint32 nservers = mCSeg->numServers();
            while(nfail < nservers) {
                ServerID sid = randInt<uint32>(1, nservers);
                if (sid == mContext->id()) continue;

                NoiseMessage* noise_msg = new NoiseMessage(mContext->id(), (uint32)(50 + 200*randFloat())); // FIXME control size from options?

                uint32 offset = 0;
                Network::Chunk msg_serialized;
                offset = noise_msg->serialize(msg_serialized, offset);

                bool sent_success = mServerMessageQueue->addMessage(sid, msg_serialized);
                if (sent_success)
                    mContext->trace()->serverDatagramQueued(mContext->time, sid, noise_msg->id(), offset);
                delete noise_msg;
                if (!sent_success) nfail++;
            }
        }
    }
    mProfiler.finishedStage();
    // XXXXXXXXXXXXXXXXXXXXXXXX

    mObjectMessageQueue->service(); mProfiler.finishedStage();
    mServerMessageQueue->service(); mProfiler.finishedStage();


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
  void Forwarder::route(MessageRouter::SERVICES svc, Message* msg, const ServerID& dest_server, bool is_forward)
  {
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
      mSelfMessages.push_back( SelfMessage(msg_serialized, is_forward) );
    }
    else
    {
      
        mContext->trace()->serverDatagramQueued(mContext->time, dest_server, msg->id(), offset);
      (*mOutgoingMessages)->push(svc, new OutgoingMessage(msg_serialized, dest_server) );
    }
    delete msg;
  }


  void Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg, bool is_forward, ServerID forwardFrom)
  {
    UUID dest_obj = msg->dest_object();
    ServerID dest_server_id = mOSeg->lookup(dest_obj);

    
#ifdef  CRAQ_CACHE
    if (is_forward && (forwardFrom != NullServerID))
    {
      //means that when we figure out which server this object is located on, we should also send an oseg update message.
      //we need to be careful not to send multiple update messages to the same server:
      //

      std::cout<<"\n\nbftm debug: first\n\n";
      
      if (mServersToUpdate.find(dest_obj) != mServersToUpdate.end())
      {
        if (mServersToUpdate[dest_obj].end() == std::find(mServersToUpdate[dest_obj].begin(), mServersToUpdate[dest_obj].end(), forwardFrom))
        {
          //don't already know that forwardFrom needs to be updated.  means that we should append the forwardFrom ServerID to a list of servers that we need to update when we know which server to send the message to.
          mServersToUpdate[dest_obj].push_back(forwardFrom);

#ifdef CRAQ_DEBUG
          std::cout<<"\n\n bftm debug: got a server to update at time  "<<mContext->time.raw()<< " obj_id:   "<<dest_obj.toString()<<"    server to send update to:  "<<forwardFrom<<"\n\n";
#endif
          
        }
      }
    }
#endif
    

    if (dest_server_id != NullServerID)
    {
      //means that we instantly knew what the location of the object is, and we can route immediately!
      route(msg, dest_server_id,is_forward);

      return;
    }

    //add message to objects in transit.
    MessageAndForward mAndF;
    mAndF.mMessage = msg;
    mAndF.mIsForward = is_forward;
    mObjectsInTransit[dest_obj].push_back(mAndF);

  }

//end what i think it should be replaced with


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
    if (conn != NULL)
        conn->send(obj_msg_cpy);
    else 
      route(obj_msg_cpy, true,GetUniqueIDServerID(obj_msg->id()) );// bftm changed to allow for forwarding back messages.

    //    else
    //        route(obj_msg_cpy, true);

    delete msg;
}



void Forwarder::route(CBR::Protocol::Object::ObjectMessage* msg, ServerID dest_serv, bool is_forward)
{
  //send out all server updates associated with an object with this message:
  UUID obj_id =  msg->dest_object();

#ifdef CRAQ_CACHE
  if (mServersToUpdate.find(obj_id) != mServersToUpdate.end())
  {
    for (int s=0; s < (int) mServersToUpdate[obj_id].size(); ++s)
    {
      UpdateOSegMessage* up_os = new UpdateOSegMessage(mContext->id(),dest_serv,obj_id);
      route(MessageRouter::OSEG_CACHE_UPDATE, up_os, mServersToUpdate[obj_id][s],false);

#ifdef CRAQ_DEBUG
      std::cout<<"\n\n Sending an oseg cache update message at time:  "<<mContext->time.raw()<<"\n";
      std::cout<<"\t for object:  "<<obj_id.toString()<<"\n";
      std::cout<<"\t to server:   "<<mServersToUpdate[obj_id][s]<<"\n";
      std::cout<<"\t obj on:      "<<dest_serv<<"\n\n";
#endif
    }
  }
#endif

  // Wrap it up in one of our ObjectMessages and ship it.
  ObjectMessage* obj_msg = new ObjectMessage(mContext->id(), *msg);  //turns the cbr::prot::message into just an object message.
  route(MessageRouter::OBJECT_MESSAGESS,obj_msg, dest_serv, is_forward);
  delete msg;
}


void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
    mObjectConnections[dest_obj] = conn;
    mObjectMessageQueue->registerClient(dest_obj, 1); // FIXME weight?
}

ObjectConnection* Forwarder::removeObjectConnection(const UUID& dest_obj) {
    mObjectMessageQueue->unregisterClient(dest_obj);
    ObjectConnection* conn = mObjectConnections[dest_obj];
    mObjectConnections.erase(dest_obj);
    return conn;
}

ObjectConnection* Forwarder::getObjectConnection(const UUID& dest_obj) {
    ObjectConnectionMap::iterator it = mObjectConnections.find(dest_obj);
    return (it == mObjectConnections.end()) ? NULL : it->second;
}

} //end namespace
