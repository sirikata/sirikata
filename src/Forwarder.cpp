
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

void OSegLookupQueue::ObjMessQBeginSendList::push_back(const OSegLookup& lu) {
    mTotalSize += lu.msg->ByteSize();
    OSegLookupVector::push_back(lu);
}

void OSegLookupQueue::push(UUID objid, const OSegLookup& lu) {
    size_t cursize = lu.msg->ByteSize();
    if (mPredicate(objid,cursize,mTotalSize)) {
        mTotalSize+=cursize;
        mObjects[objid].push_back(lu);
    }
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
   mOSegLookups(&AlwaysPush),
   mLastSampleTime(Time::null()),
   mSampleRate( GetOption(STATS_SAMPLE_RATE)->as<Duration>() ),
   mProfiler("Forwarder Loop")
{
    //no need to initialize mOutgoingMessages.

    // Fill in the rest of the context
    mContext->mRouter = this;
    mContext->mDispatcher = this;

    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(SERVER_PORT_OBJECT_MESSAGE_ROUTING, this);
    this->registerMessageRecipient(SERVER_PORT_NOISE, this);

    mProfiler.addStage("OSeg");
    mProfiler.addStage("Noise");
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

    OSegLookupQueue::ObjectMap::iterator iterQueueMap;

    mOSeg->service(updatedObjectLocations);

    //    cross-check updates against held messages.
    for (iter = updatedObjectLocations.begin();  iter != updatedObjectLocations.end(); ++iter)
    {
        ServerID dest_server = iter->second;

      //Now sending messages that we had saved up from oseg lookup calls.
      iterQueueMap = mOSegLookups.find(iter->first);
      if (iterQueueMap != mOSegLookups.end())
      {
        for (int s=0; s < (signed) ((iterQueueMap->second).size()); ++ s)
        {
            const OSegLookup& lu = (iterQueueMap->second[s]);
            if (!routeObjectMessageToServer(lu.msg, dest_server, lu.forward)) {
                mContext->trace()->timestampMessage(mContext->time,
                    lu.msg->unique(),
                    Trace::DROPPED,
                    lu.msg->source_port(),
                    lu.msg->dest_port(),
                    SERVER_PORT_OBJECT_MESSAGE_ROUTING
                );
                // FIXME causing a crash
                //delete lu.msg;
            }
            // FIXME should we be deleting in_msg here?
        }
        mOSegLookups.erase(iterQueueMap);
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

    if (GetOption(NOISE)->as<bool>()) {
        for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
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
    mProfiler.finishedStage();

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

            tryTimestampObjectMessage(mContext->trace(), mContext->time, next_msg, Trace::SPACE_OUTGOING_MESSAGE);


            mContext->trace()->serverDatagramQueued(mContext->time, next_msg->dest_server(), next_msg->id(), next_msg->serializedSize());
            bool send_success = mServerMessageQueue->addMessage(next_msg);
            if (!send_success)
                break;

            Message* pop_msg = mOutgoingMessages->getFairQueue(sid).pop(&size);
            assert(pop_msg == next_msg);
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


    ServerID destServer = mOSeg->lookup(msg->dest_object());
    if (destServer == NullServerID)
    {
        OSegLookup lu;
        lu.msg = msg;
        lu.forward = is_forward;
        mOSegLookups[msg->dest_object()].push_back(lu);
        //if mOSegLookups.full() send_success=false;
        return true;
        // FIXME what if the mOSegLookups is full? Do we need to timestamp?
    }
    else
    {
        return routeObjectMessageToServer(msg, destServer, is_forward);
    }
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

    return route(obj_msg, false, NullServerID);
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



bool Forwarder::routeObjectMessageToServer(CBR::Protocol::Object::ObjectMessage* obj_msg, ServerID dest_serv, bool is_forward)
{
  //send out all server updates associated with an object with this message:
  UUID obj_id =  obj_msg->dest_object();

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
