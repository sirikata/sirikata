
#include "Network.hpp"
#include "Server.hpp"
#include "Proximity.hpp"
#include "CoordinateSegmentation.hpp"
#include "Message.hpp"
#include "ServerIDMap.hpp"
#include "ServerMessageQueue.hpp"
#include "Statistics.hpp"
#include "Options.hpp"
#include "ObjectMessageQueue.hpp"
#include "LoadMonitor.hpp"

#include "ForwarderUtilityClasses.hpp"
#include "Forwarder.hpp"
#include "ObjectSegmentation.hpp"

#include "Proximity.hpp"

#include "ObjectConnection.hpp"

#include "Random.hpp"

namespace CBR
{


  /*
    Constructor for Forwarder

  */
Forwarder::Forwarder(ServerID id)
 : mTrace(NULL),
   mCSeg(NULL),
   mOSeg(NULL),
   mLocationService(NULL),
   mObjectMessageQueue(NULL),
   mServerMessageQueue(NULL),
   mLoadMonitor(NULL),
   mProximity(NULL),
   m_serv_ID(id),
   mCurrentTime(NULL),
   mLastSampleTime(0),
   mSampleRate( GetOption(STATS_SAMPLE_RATE)->as<Duration>() )
{
    //no need to initialize mSelfMessages and mOutgoingMessages.

    // Messages destined for objects are subscribed to here so we can easily pick them
    // out and decide whether they can be delivered directly or need forwarding
    this->registerMessageRecipient(MESSAGE_TYPE_PROXIMITY, this);
    this->registerMessageRecipient(MESSAGE_TYPE_LOCATION, this);
    this->registerMessageRecipient(MESSAGE_TYPE_SUBSCRIPTION, this);
}

  //Don't need to do anything special for destructor
  Forwarder::~Forwarder()
  {
    this->unregisterMessageRecipient(MESSAGE_TYPE_PROXIMITY, this);
    this->unregisterMessageRecipient(MESSAGE_TYPE_LOCATION, this);
    this->unregisterMessageRecipient(MESSAGE_TYPE_SUBSCRIPTION, this);
  }

  /*
    Assigning time and mObjects, which should have been constructed in Server's constructor.
  */
void Forwarder::initialize(Trace* trace, CoordinateSegmentation* cseg, ObjectSegmentation* oseg, LocationService* locService, ObjectMessageQueue* omq, ServerMessageQueue* smq, LoadMonitor* lm, Time* currTime, Proximity* prox)
  {
      mTrace = trace;
      mCSeg = cseg;
      mOSeg = oseg;
      mLocationService = locService;
      mObjectMessageQueue = omq;
      mServerMessageQueue =smq;
      mLoadMonitor = lm;
    mCurrentTime = currTime;
    mProximity = prox;
  }

  //used to access server id, which was given in initialization
  const ServerID& Forwarder::serv_id() const
  {
    return m_serv_ID;
  }



  void Forwarder::getOSegMessages()
  {
    std::vector<Message*> messToSendFromOSegToForwarder;
    std::vector<ServerID> destServers;


    mOSeg -> getMessages (messToSendFromOSegToForwarder,destServers);

    for (int s=0; s < (signed)messToSendFromOSegToForwarder.size(); ++s)
    {
      //route
      route(messToSendFromOSegToForwarder[s],destServers[s],false);
    }

  }

  /*
    Sends a tick to OSeg.  Receives messages from oseg.  Adds them to mOutgoingQueue.
  */
  void Forwarder::tickOSeg(const Time&t)
  {
    getOSegMessages();


    //bftm tmp, nothing
    //    mOSeg -> tick(t,mOutgoingMessages);
    //objects from
    std::map<UUID,ServerID> updatedObjectLocations;
    std::map<UUID,ServerID>::iterator iter;
    std::map<UUID, std::vector<Message*> >::iterator iterObjectsInTransit;

    mOSeg -> tick(t,updatedObjectLocations);

    //    cross-check updates against held messages.
    for (iter = updatedObjectLocations.begin();  iter != updatedObjectLocations.end(); ++iter)
    {
      iterObjectsInTransit = mObjectsInTransit.find(iter->first);

      if (iterObjectsInTransit != mObjectsInTransit.end())
      {
        //means that we can route messages being held in mObjectsInTransit

        for (int s=0; s < (signed)((iterObjectsInTransit->second).size()); ++s)
        {
          route((iterObjectsInTransit->second)[s],iter->second,false);
          //          void Forwarder::route(Message* msg, const ServerID& dest_server, bool is_forward)
        }

        //remove the messages from the objects in transit
        mObjectsInTransit.erase(iterObjectsInTransit);

      }
    }
  }



  void Forwarder::tick(const Time&t)
  {

      if (t - mLastSampleTime > mSampleRate) {
          mServerMessageQueue->reportQueueInfo(t);
          mLastSampleTime = t;
      }

    mLoadMonitor->tick(t);

    tickOSeg(t);//updates oseg


    std::deque<SelfMessage> self_messages;
    self_messages.swap( mSelfMessages );
    while (!self_messages.empty())
    {
      processChunk(self_messages.front().data, this->serv_id(), self_messages.front().forwarded);
      self_messages.pop_front();
    }

    while(!mOutgoingMessages.empty())
    {
        OutgoingMessage& next_msg = mOutgoingMessages.front();
        bool send_success = mServerMessageQueue->addMessage(next_msg.dest, next_msg.data);
        if (!send_success) break;
        mOutgoingMessages.pop_front();
    }

    // XXXXXXXXXXXXXXXXXXXXXX Generate noise

    if (GetOption(NOISE)->as<bool>()) {
        if (GetOption(SERVER_QUEUE)->as<String>() == "fair") {
            for(ServerID sid = 1; sid <= mCSeg->numServers(); sid++) {
                if (sid == m_serv_ID) continue;
                while(true) {
                    OriginID origin;
                    origin.id = m_serv_ID;

                    NoiseMessage* noise_msg = new NoiseMessage(origin, (uint32)(50 + 200*randFloat())); // FIXME control size from options?

                    uint32 offset = 0;
                    Network::Chunk msg_serialized;
                    offset = noise_msg->serialize(msg_serialized, offset);

                    bool sent_success = mServerMessageQueue->addMessage(sid, msg_serialized);
                    if (sent_success)
                        mTrace->serverDatagramQueued((*mCurrentTime), sid, noise_msg->id(), offset);
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
                if (sid == m_serv_ID) continue;

                OriginID origin;
                origin.id = m_serv_ID;

                NoiseMessage* noise_msg = new NoiseMessage(origin, (uint32)(50 + 200*randFloat())); // FIXME control size from options?

                uint32 offset = 0;
                Network::Chunk msg_serialized;
                offset = noise_msg->serialize(msg_serialized, offset);

                bool sent_success = mServerMessageQueue->addMessage(sid, msg_serialized);
                if (sent_success)
                    mTrace->serverDatagramQueued((*mCurrentTime), sid, noise_msg->id(), offset);
                delete noise_msg;
                if (!sent_success) nfail++;
            }
        }
    }
    // XXXXXXXXXXXXXXXXXXXXXXXX

    mObjectMessageQueue->service(t);
    mServerMessageQueue->service(t);


    Sirikata::Network::Chunk *c=NULL;
    ServerID source_server;
    while(mServerMessageQueue->receive(&c, &source_server))
    {
        processChunk(*c, source_server, false);
        delete c;
    }
  }




  // Routing interface for servers.  This is used to route messages that originate from
  // a server provided service, and thus don't have a source object.  Messages may be destined
  // for either servers or objects.  The second form will simply automatically do the destination
  // server lookup.
  // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
  /*  void Forwarder::route(Message* msg, const ServerID& dest_server, bool is_forward)
  {
    assert(msg != NULL);

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset = msg->serialize(msg_serialized, offset);

    if (dest_server==serv_id())
    {
      if (!is_forward)
      {
        mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
        mTrace->serverDatagramSent((*mCurrentTime), (*mCurrentTime), 0 , dest_server, msg->id(), offset); // self rate is infinite => start and end times are identical
      }

      mSelfMessages.push_back( SelfMessage(msg_serialized, is_forward) );
    }
    else
    {
      mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
      mOutgoingMessages.push_back( OutgoingMessage(msg_serialized, dest_server) );
    }
    delete msg;
  }


  void Forwarder::route(Message* msg, const UUID& dest_obj, bool is_forward)
  {
    ServerID dest_server_id = lookup(dest_obj);
    route(msg, dest_server_id, is_forward);
  }

  */



  // Routing interface for servers.  This is used to route messages that originate from
  // a server provided service, and thus don't have a source object.  Messages may be destined
  // for either servers or objects.  The second form will simply automatically do the destination
  // server lookup.
  // if forwarding is true the message will be stuck onto a queue no matter what, otherwise it may be delivered directly
  void Forwarder::route(Message* msg, const ServerID& dest_server, bool is_forward)
  {

    uint32 offset = 0;
    Network::Chunk msg_serialized;
    offset = msg->serialize(msg_serialized, offset);

    if (dest_server==serv_id())
    {
      if (!is_forward)
      {
        mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
        mTrace->serverDatagramSent((*mCurrentTime), (*mCurrentTime), 0 , dest_server, msg->id(), offset); // self rate is infinite => start and end times are identical
      }else {
          // The more times the message goes through a forward to self loop, the more times this will record, causing an explosion
          // in trace size.  It's probably not worth recording this information...
          //mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
      }
      mSelfMessages.push_back( SelfMessage(msg_serialized, is_forward) );
    }
    else
    {
      mTrace->serverDatagramQueued((*mCurrentTime), dest_server, msg->id(), offset);
      mOutgoingMessages.push_back( OutgoingMessage(msg_serialized, dest_server) );
    }
    delete msg;

  }

  void Forwarder::route(Message* msg, const UUID& dest_obj, bool is_forward)
  {
    //    ServerID dest_server_id = lookup(dest_obj);
    ServerID dest_server_id = mOSeg->lookup(dest_obj);

    if (dest_server_id == OBJECT_IN_TRANSIT)
    {
      //add message to objects in transit.
      //      if (mObjectsInTransit[dest_obj] == null)
      if (mObjectsInTransit.find(dest_obj) == mObjectsInTransit.end())
      {
        //no messages have been queued for mObjectsInTransit
        //add this as the beginning of a vector of message pointers.
        std::vector<Message*> tmp;
        tmp.push_back(msg);
        mObjectsInTransit[dest_obj] = tmp;

      }
      else
      {

        //add message to queue of objects in transit.
        mObjectsInTransit[dest_obj].push_back(msg);
      }
    }
    else
    {
      route(msg, dest_server_id, is_forward);
    }
  }




  void Forwarder::processChunk(const Network::Chunk&chunk, const ServerID& source_server, bool forwarded_self_msg)
  {
    Message* result;
    unsigned int offset=0;
    do
    {
      offset = Message::deserialize(chunk,offset,&result);

      if (!forwarded_self_msg)
        mTrace->serverDatagramReceived((*mCurrentTime), (*mCurrentTime), source_server, result->id(), offset);

      deliver(result);

      //if (delivered)
      //  mTrace->serverDatagramReceived((*mCurrentTime), (*mCurrentTime), source_server, result->id(), offset);
    }while (offset<chunk.size());
  }



  // Delivery interface.  This should be used to deliver received messages to the correct location -
  // the server or object it is addressed to.
  void Forwarder::deliver(Message* msg)
  {
    switch(msg->type()) {
      case MESSAGE_TYPE_PROXIMITY: // fall-through
      case MESSAGE_TYPE_LOCATION:  // fall-through
      case MESSAGE_TYPE_SUBSCRIPTION:
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
      case MESSAGE_TYPE_OSEG_MIGRATE:
        {
          OSegMigrateMessage* oseg_change_msg = dynamic_cast<OSegMigrateMessage*> (msg);
          assert(oseg_change_msg != NULL);

          if (oseg_change_msg->getMessageDestination() != this->serv_id())
          {
            //route it on to another server

              route(oseg_change_msg, oseg_change_msg->getMessageDestination(),true);
          }
          else
          {
            //otherwise, deliver to local oseg.

            //bftm debug
            mOSeg->osegMigrateMessage(oseg_change_msg);
            delete oseg_change_msg;
          }
        }
        break;
      case MESSAGE_TYPE_OSEG_LOOKUP:
        {
          OSegLookupMessage* oseg_lookup_msg = dynamic_cast<OSegLookupMessage*> (msg);
          assert(oseg_lookup_msg != NULL);

          mOSeg->processLookupMessage(oseg_lookup_msg);  //responsible for deleting the message if it has a record of it.
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


  void Forwarder::forward(Message* msg, const UUID& dest_obj)
  {

    ServerID dest_server_id = mOSeg->lookup(dest_obj);

    if (dest_server_id == OBJECT_IN_TRANSIT)
    {


      //add message to objects in transit.
      //      if (mObjectsInTransit[dest_obj] == null)
      if (mObjectsInTransit.find(dest_obj) == mObjectsInTransit.end())
      {
        //no messages have been queued for mObjectsInTransit
        //add this as the beginning of a vector of message pointers.
        std::vector<Message*> tmp;
        tmp.push_back(msg);
        mObjectsInTransit[dest_obj] = tmp;
      }
      else
      {
        //add message to queue of objects in transit.
        mObjectsInTransit[dest_obj].push_back(msg);
      }
    }
    else
    {
      route(msg, dest_server_id, true);
    }

  }


static UUID getObjectMessageDest(Message* msg) {
    switch(msg->type()) {
      case MESSAGE_TYPE_PROXIMITY:
          {
              ProximityMessage* prox_msg = dynamic_cast<ProximityMessage*>(msg);
              assert(prox_msg != NULL);

              return prox_msg->object_header.dest_object();
          }
          break;
      case MESSAGE_TYPE_LOCATION:
          {
              ObjectToObjectMessage* obj_msg = dynamic_cast<ObjectToObjectMessage*>(msg);
              assert(obj_msg != NULL);

              return obj_msg->destObject();
          }
      case MESSAGE_TYPE_SUBSCRIPTION:
          {
              SubscriptionMessage* subs_msg = dynamic_cast<SubscriptionMessage*>(msg);
              assert(subs_msg != NULL);

              return subs_msg->object_header.dest_object();
          }
        break;
      default:
        assert(false);
    }
}

void Forwarder::receiveMessage(Message* msg) {
    // Forwarder only subscribes as a recipient for object messages
    // so it can easily check whether it can deliver directly
    // or needs to forward them.

    UUID dest = getObjectMessageDest(msg);
    ObjectConnection* conn = getObjectConnection(dest);
    if (conn != NULL)
        conn->deliver(msg, *mCurrentTime);
    else
        forward(msg, dest);
}


  ServerID Forwarder::lookup(const Vector3f& pos)
  {
    ServerID sid = mCSeg->lookup(pos);
    return sid;
  }

  /*
    May return that it's in transit.
  */
  ServerID Forwarder::lookup(const UUID& obj_id)
  {

    ServerID sid = mOSeg->lookup(obj_id);
    //need to change this lookup function to query oseg instead.
    /*
    Vector3f pos = mLocationService->currentPosition(obj_id);
    ServerID sid = mCSeg->lookup(pos);
    */

    return sid;
  }

void Forwarder::addObjectConnection(const UUID& dest_obj, ObjectConnection* conn) {
    mObjectConnections[dest_obj] = conn;
}

ObjectConnection* Forwarder::removeObjectConnection(const UUID& dest_obj) {
    ObjectConnection* conn = mObjectConnections[dest_obj];
    mObjectConnections.erase(dest_obj);
    return conn;
}

ObjectConnection* Forwarder::getObjectConnection(const UUID& dest_obj) {
    ObjectConnectionMap::iterator it = mObjectConnections.find(dest_obj);
    return (it == mObjectConnections.end()) ? NULL : it->second;
}

} //end namespace
