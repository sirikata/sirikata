


#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include <map>
#include <vector>
#include "Statistics.hpp"
#include "Timer.hpp"

//#include "oseg_dht/Bamboo.hpp"
//#include "oseg_dht/gateway_prot.h"
//#include "DhtObjectSegmentation.hpp"
#include "CraqObjectSegmentation.hpp"
#include "craq_oseg/asyncCraq.hpp"
#include "craq_oseg/asyncUtil.hpp"
#include "craq_oseg/asyncConnection.hpp"


#include "CoordinateSegmentation.hpp"
#include <sstream>
#include <string.h>
#include <stdlib.h>


namespace CBR
{

  /*
    Basic constructor
  */
  //  CraqObjectSegmentation::CraqObjectSegmentation (CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, ServerID servID,  Trace* tracer, char* dht_host, char* dht_port)
  CraqObjectSegmentation::CraqObjectSegmentation (CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, ServerID servID,  Trace* tracer, std::vector<CraqInitializeArgs> initArgs)
    : mCSeg (cseg),
      mCurrentTime(Time::null())
  {
    
    mID = servID;
    mTrace = tracer;
    
    craqDht.initialize(initArgs);
    
    std::vector<CraqOperationResult> getResults;
    std::vector<CraqOperationResult> trackedSetResults;

    //1000 ticks put here to allow lots of connections to be made
    for (int s=0; s < 1000; ++s)
    {
      craqDht.tick(getResults,trackedSetResults);
    }
    

    CraqDataKey obj_id_as_dht_key;
    //start loading the objects that are in vectorOfObjectsInitializedOnThisServer into the dht.
    for (int s=0;s < (int)vectorOfObjectsInitializedOnThisServer.size(); ++s)
    {
      
      convert_obj_id_to_dht_key(vectorOfObjectsInitializedOnThisServer[s],obj_id_as_dht_key);

      CraqDataSetGet cdSetGet(vectorOfObjectsInitializedOnThisServer[s].rawHexData(), mID,false,CraqDataSetGet::SET);
      craqDht.set(cdSetGet);
      
      printf("\nObject %i of %i\n",s+1,(int)(vectorOfObjectsInitializedOnThisServer.size()) );
    }

    //50 ticks to update
    for (int s=0; s < 150; ++s)
    {
      craqDht.tick(getResults,trackedSetResults);
      
      //      craqDht.tick(serverIds,objectIds,trackedMessages);
    }
  }
    
  /*
    Destructor
  */
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
  }


  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
  */
  void CraqObjectSegmentation::lookup(const UUID& obj_id)
  {
    
    UUID tmper = obj_id;
    std::map<UUID,ServerID>::const_iterator iter = mInTransitOrLookup.find(tmper);
    
    if (iter == mInTransitOrLookup.end()) //means that the object isn't already being looked up and the object isn't already in transit
    {
      //if object is not in transit, lookup its location in the dht.  returns -1 if object doesn't exist.

      //add the mapping of a craqData Key to a uuid.
      
      CraqDataSetGet cdSetGet (tmper.rawHexData(),0,false,CraqDataSetGet::GET);
      mapDataKeyToUUID[tmper.rawHexData()] = tmper;
      std::cout<<"\n\nIn craq lookup: this is tmperId:   "<<tmper.toString()<<"\n\n";

      craqDht.get(cdSetGet); //calling the craqDht to do a get.
    }
  }

  /*
    Returns the id of the server that this oseg is hosted on.
  */
  ServerID CraqObjectSegmentation::getHostServerID()
  {
    return ObjectSegmentation::mID ;
  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  Message* CraqObjectSegmentation::generateAcknowledgeMessage(Object* obj,ServerID sID_to)
  {
    const UUID& obj_id = obj->uuid();
    OriginID origin;
    origin.id= (uint32)(this->getHostServerID());

    Message* oseg_change_msg = new OSegMigrateMessage(origin,  this->getHostServerID(),  sID_to, sID_to,    this->getHostServerID(),      obj_id, OSegMigrateMessage::ACKNOWLEDGE);
                                                    //origin,id_from, id_to,   messDest  messFrom   obj_id   osegaction
    return  oseg_change_msg;
  }

  
  /*
    Means that the server that this oseg is on now is in charge of the object with obj_id.
    Add
    1. pushes this server id onto the dht associated with the object_id (as key)  (even if it doesn't already exist on the dht).
        -may need to wait for an acknowledgment before tell the other servers that we have taken control of the object.
        -potentially run a series of gets against.
        -potentially send a special token.  Can know that it's been done w
    2. Designates that the craq dht should update the entry corresponding to obj_id.  The craq dht is also instructed to track the call associated with this call (so that we know when it's occured).
        -We store the tracking ID.  The tick call to craqDht will give us a list of trackingIds that have been serviced correctly.
        -When we receive an id in the tick function, we push the message in tracking messages to the forwarder to forward an acknowledge message to the 
    
    Generates an acknowledge message.  Only sends the acknowledge message when the trackId has been returned by 
       --really need to think about this.
  */
  void CraqObjectSegmentation::addObject(const UUID& obj_id, Object* obj, const ServerID ourID)
  {
    CraqDataSetGet cdSetGet(obj_id.rawHexData(),ourID,true,CraqDataSetGet::SET);
    int trackID = craqDht.set(cdSetGet);
    
    
    Message* oseg_ack_msg;
    trackingMessages[trackID] = generateAcknowledgeMessage(obj, (dynamic_cast<OSegMigrateMessage*>(oseg_ack_msg))->getMessageDestination());
    std::cout<<"\n\nbftm: debug inside of add object for obj_id:  "<<obj_id.toString()<<"\n\n";
  }

  
  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
    Whatever calls this must verify that the object is on this server.
    I can do the check in the function by querrying bambooDht as well
  */
  void CraqObjectSegmentation::migrateObject(const UUID& obj_id, const ServerID new_server_id)
  {
    printf("\n\n bftm got a migrateObject call in DhtObjecSegmentation.cpp \n\n");

    //log the message.
    mTrace->objectBeginMigrate(mCurrentTime,obj_id,this->getHostServerID(),new_server_id); //log it.
      
    //if we do, then say that the object is in transit.
    mInTransitOrLookup[obj_id] = new_server_id;
  }


  
  /*
    Behavior:

    If receives a create message:
      1) Checks to make sure that object does not already exist in dht
      2) If it doesn't, adds the object and my server name to dht.
      3) If it does, throws an error and kills program.
      
    If receives a kill message:
      1) Gets all records of object from dht.
      2) Systematically deletes every object/server pair from dht.
      3) ????Also deletes object from inTransit (not implemented yet)???
      4) Note: if object doesn't exist, does nothing.
      
    Should never receive a move message.
      Kills program if I do.
    
    If receives an acknowledged message, it means that we can start forwarding messages that we received:
      1) Checks to see if object id was stored in inTransit.
      2) If it is, then we remove it from inTransit.
      3) Put it in mFinishedMove.
      4) Log the acknowledged message.
    
  */
  void CraqObjectSegmentation::osegMigrateMessage(OSegMigrateMessage* msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;
    OSegMigrateMessage::OSegMigrateAction oaction;

    serv_from = msg->getServFrom();
    serv_to   = msg->getServTo();
    obj_id    = msg->getObjID();
    oaction   = msg->getAction();
    
    switch (oaction)
    {
      case OSegMigrateMessage::CREATE:      //msg says something has been added
        {
          std::cout<<"\n\nReceived a create osegmigratemessage.  Should not have.  Killing now\n\n";
          assert(false);
        }
        break;

      case OSegMigrateMessage::KILL:
        {
          std::cout<<"\n\nReceived a kill osegmigratemessage.  Should not have.  Killing now\n\n";
          assert(false);
        }
        
        break;

      case OSegMigrateMessage::MOVE:     //means that an object moved from one server to another.
        {
          //should never receive this message.
          printf("\n\nIn CraqObjectSegmentation.cpp under osegMigrateMessage.  Got a move message that I should not have.\n\n");
          assert(false);
        }
        break;

      case OSegMigrateMessage::ACKNOWLEDGE:
        {
          //When receive an acknowledge, it means that we can free the messages that we were holding for the node that's being moved.
          //remove object id from in transit.
          //place it in mFinishedMove

          printf("\n\n bftm debug: in DhtObjectSegmentation.cpp.  Got an acknowledge. \n\n");
          
          std::map<UUID,ServerID>::iterator inTransIt;
        
          inTransIt = mInTransitOrLookup.find(obj_id);
          if (inTransIt != mInTransitOrLookup.end())
          {
            //means that we now remove the obj_id from mInTransit
            mInTransitOrLookup.erase(inTransIt);
            //add it to mFinishedMove.  serv_from
            mFinishedMoveOrLookup[obj_id] = serv_from;

            //log reception of acknowled message
            mTrace->objectAcknowledgeMigrate(mCurrentTime, obj_id,serv_from,this->getHostServerID());
          }
        }        
        break;
    }
  }

  /*
    Fill the messageQueueToPushTo deque by pushing outgoing messages from destServers and osegMessages to it
    Then clear osegMessages and destServers.

    may also need to add a message to send queue
    
  */
  void CraqObjectSegmentation::tick(const Time& t, std::map<UUID,ServerID>& updated,std::vector<Message*> &messagesToSend)
  {
    std::cout<<"\n\nGot into a tick for craqobjectseg\n\n";
    std::vector<CraqOperationResult> getResults;
    std::vector<CraqOperationResult> trackedSetResults;
    craqDht.tick(getResults,trackedSetResults);

    //bftm note: I don't really know that this should go here.
    updated = mFinishedMoveOrLookup;

    //run through all the get results first.
    for (unsigned int s=0; s < getResults.size(); ++s)
    {
      updated[mapDataKeyToUUID[getResults[s].idToString()]]  = getResults[s].servID;
    }

    for (unsigned int s=0; s < trackedSetResults.size();  ++s)
    {
      if (trackedSetResults[s].trackedMessage != 0)
      {
        if (trackingMessages.find(trackedSetResults[s].trackedMessage) !=  trackingMessages.end())
        {
          //means that we were tracking this message.
          //means that we populate the
          printf("\n\nbftm: debug inside of tick of craqObjectSeg: adding messages to push back to forwarder.  \n\n");
          
          messagesToSend.push_back(trackingMessages[trackedSetResults[s].trackedMessage]);
          trackingMessages.erase(trackedSetResults[s].trackedMessage);//stop tracking this message.
        }
      }
    }
    
    mCurrentTime = t;
    //    updated =  mFinishedMoveOrLookup;
    mFinishedMoveOrLookup.clear();
  }


  /*
  */
  //  void DhtObjectSegmentation::convert_obj_id_to_dht_key(const UUID& obj_id, Bamboo::Bamboo_key& returner) const
  void CraqObjectSegmentation::convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const
  {
    std::cout<<"\n\ngot into convert_obj_id_to_dht_key\n\n";
    strncpy(returner,obj_id.rawHexData().c_str(),obj_id.rawHexData().size() + 1);
  }


}//namespace CBR
