


#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include <map>
#include <vector>
#include "Statistics.hpp"
#include "Timer.hpp"
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
CraqObjectSegmentation::CraqObjectSegmentation (SpaceContext* ctx, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> initArgs, char prefixID)
 : ObjectSegmentation(ctx),
   mCSeg (cseg)
{
    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->dispatcher->registerMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher->registerMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE,this);

    craqDht.initialize(initArgs);

    std::vector<CraqOperationResult> getResults;
    std::vector<CraqOperationResult> trackedSetResults;

    myUniquePrefixKey = prefixID;

    numTicks = 0;
    
    //1000 ticks put here to allow lots of connections to be made
    for (int s=0; s < 500; ++s)
    {
      craqDht.tick(getResults,trackedSetResults);
      processCraqTrackedSetResults(trackedSetResults);
    }


    CraqDataKey obj_id_as_dht_key;
    //start loading the objects that are in vectorOfObjectsInitializedOnThisServer into the dht.
    for (int s=0;s < (int)vectorOfObjectsInitializedOnThisServer.size(); ++s)
    {
      convert_obj_id_to_dht_key(vectorOfObjectsInitializedOnThisServer[s],obj_id_as_dht_key);
      
      //      CraqDataSetGet cdSetGet(vectorOfObjectsInitializedOnThisServer[s].rawHexData(), mContext->id,false,CraqDataSetGet::SET);
      CraqDataSetGet cdSetGet(obj_id_as_dht_key, mContext->id,false,CraqDataSetGet::SET);
      craqDht.set(cdSetGet);
      printf("\nObject %i of %i\n",s+1,(int)(vectorOfObjectsInitializedOnThisServer.size()) );

      mObjects.push_back(vectorOfObjectsInitializedOnThisServer[s]);        //also need to load those objects into local object storage.
      
    }

    //50 ticks to update
    for (int s=0; s < 250; ++s)
    {
      craqDht.tick(getResults,trackedSetResults);
      processCraqTrackedSetResults(trackedSetResults);
    }

    mTimer.start();
  }

  /*
    Destructor
  */
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
    mContext->dispatcher->unregisterMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher->unregisterMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE,this);
  }


  /*
    This function checks to see whether the obj_id is hosted on this space server
  */
  bool CraqObjectSegmentation::checkOwn(const UUID& obj_id)
  {
    if (std::find(mObjects.begin(),mObjects.end(), obj_id) == mObjects.end())
    {
      //means that the object isn't hosted on this space server
      return false;
    }

    //means that the object *is* hosted on this space server
    //need to queue the result as a response.
    mFinishedMoveOrLookup[obj_id]= mContext->id;
    
    return true;  
  }


  
  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
  */
  void CraqObjectSegmentation::lookup(const UUID& obj_id)
  {
    
     if (checkOwn(obj_id))  //this call just checks through
     {
       return;
     }
    
    UUID tmper = obj_id;
    std::map<UUID,TransLookup>::const_iterator iter = mInTransitOrLookup.find(tmper);

    if (iter == mInTransitOrLookup.end()) //means that the object isn't already being looked up and the object isn't already in transit
    {
      //if object is not in transit, lookup its location in the dht.  returns -1 if object doesn't exist.
      //add the mapping of a craqData Key to a uuid.

      std::string indexer = "";
      indexer.append(1,myUniquePrefixKey);
      indexer.append(tmper.rawHexData());

      CraqDataSetGet cdSetGet (indexer,0,false,CraqDataSetGet::GET); //bftm modified
      //      CraqDataSetGet cdSetGet (tmper.rawHexData(),0,false,CraqDataSetGet::GET);
      //      mapDataKeyToUUID[tmper.rawHexData()] = tmper;

      mapDataKeyToUUID[indexer] = tmper; //changed here.

      craqDht.get(cdSetGet); //calling the craqDht to do a get.

      
      mContext->trace->objectSegmentationLookupRequest(mContext->time, obj_id, mContext->id);
      //      mContext->trace->objectSegmentationLookupRequest(timerDur.toMilliseconds(), obj_id, mContext->id);

      //also need to say that the object is in transit or lookup.

      Duration timerDur = mTimer.elapsed();
      TransLookup tmpTransLookup;
      tmpTransLookup.sID = 0;      
      //      tmpTransLookup.timeAdmitted = mContext->time.raw();
      tmpTransLookup.timeAdmitted = timerDur.toMilliseconds();

      mInTransitOrLookup[tmper] = tmpTransLookup; //just says that we are performing a lookup on the object
    }
  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  OSegMigrateMessageAcknowledge* CraqObjectSegmentation::generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to)
  {
    OSegMigrateMessageAcknowledge* oseg_ack_msg = new OSegMigrateMessageAcknowledge(mContext->id,  mContext->id,  sID_to, sID_to,    mContext->id,obj_id);

    return oseg_ack_msg;
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
  void CraqObjectSegmentation::addObject(const UUID& obj_id, const ServerID idServerAckTo, bool generateAck)
  {
    if (generateAck)
    {
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      //      CraqDataSetGet cdSetGet(obj_id.rawHexData(), mContext->id ,true,CraqDataSetGet::SET);
      CraqDataSetGet cdSetGet(cdk, mContext->id ,true,CraqDataSetGet::SET);

      
      int trackID = craqDht.set(cdSetGet);
      trackingMessages[trackID] = generateAcknowledgeMessage(obj_id, idServerAckTo);
      //      std::cout<<"\n\nbftm: debug inside of add object for obj_id.  will generateAck:  "<<obj_id.toString()<<"   to   "<< idServerAckTo<< "  from  "<< mContext->id << "   this is trackID     "  << trackID << "\n\n";
    }
    else
    {

      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      //      CraqDataSetGet cdSetGet(obj_id.rawHexData(), mContext->id ,false,CraqDataSetGet::SET);
      CraqDataSetGet cdSetGet(cdk, mContext->id ,false,CraqDataSetGet::SET);
      int trackID = craqDht.set(cdSetGet);
    }
  }


  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
    Whatever calls this must verify that the object is on this server.
    I can do the check in the function by querrying bambooDht as well
  */
  void CraqObjectSegmentation::migrateObject(const UUID& obj_id, const ServerID new_server_id)
  {

    //log the message.
    mContext->trace->objectBeginMigrate(mContext->time,obj_id,mContext->id,new_server_id); //log it.

    std::map<UUID,TransLookup>::const_iterator transIter = mInTransitOrLookup.find(obj_id);

    if (transIter != mInTransitOrLookup.end())
    {
      std::cout<<"\n\n The object was already in lookup or transit \n\n  ";
    }


    //if we do, then say that the object is in transit.
    //    mInTransitOrLookup[obj_id] = new_server_id;

    TransLookup tmpTransLookup;
    tmpTransLookup.sID = new_server_id;
    tmpTransLookup.timeAdmitted = mContext->time.raw();

    mInTransitOrLookup[obj_id] = tmpTransLookup;
    
    //erases the local copy of obj_id
    UUID tmp_id = obj_id; //note: can probably delete this line

    //erase the local copy of the object.
    std::vector<UUID>::iterator iter = std::find(mObjects.begin(),mObjects.end(),obj_id);
    if (iter != mObjects.end())
    {
      mObjects.erase (iter);//erase the local copy of the object.
    }
    else
    {
      std::cout<<"\n\nThe object clearly wasn't registered on this server.  Oh no.\n\n";
    }
    
  }

  
  //this function takes all the tracked set results 
  void CraqObjectSegmentation::processCraqTrackedSetResults(std::vector<CraqOperationResult> &trackedSetResults)
  {
    for (unsigned int s=0; s < trackedSetResults.size();  ++s)
    {
      //genrateAcknowledgeMessage(uuid,sidto);  ...we may as well hold onto the object pointer then.
      if (trackedSetResults[s].trackedMessage != 0)
      {
        if (trackingMessages.find(trackedSetResults[s].trackedMessage) !=  trackingMessages.end())
        {
          //means that we were tracking this message.
          //means that we populate the
          //          printf("\n\nbftm: debug inside of tick of craqObjectSeg: adding messages to push back to forwarder.  \n\n");

               //add to mObjects the uuid associated with trackedMessage.
          mObjects.push_back(trackingMessages[trackedSetResults[s].trackedMessage]->getObjID());

//           std::cout<<"\n\nbftm debug: sending an acknowledge from "<<mContext->id<<"  to ";
//           std::cout<< trackingMessages[trackedSetResults[s].trackedMessage]->getMessageDestination()  <<"   this is tracking message number  ";
//           std::cout<<trackedSetResults[s].trackedMessage  << "\n\n";
//           std::cout<<"  for object:  ";
//           std::cout<< trackingMessages[trackedSetResults[s].trackedMessage]->getObjID().toString()<<"    at time   "<< mContext->time.raw()  <<  " \n\n";
          

          trackingMessages.erase(trackedSetResults[s].trackedMessage);//stop tracking this message.
        }
        else
        {
          std::cout<<"\n\nbftm debug:  received a tracked set that I don't have a record of an acknowledge to send.\n\n";
        }
      }
    }
  }




void CraqObjectSegmentation::iteratedWait(int numWaits,std::vector<CraqOperationResult> &allGetResults,std::vector<CraqOperationResult>&allTrackedResults)
{
  for (int s=0; s < numWaits; ++s)
  {
    basicWait(allGetResults,allTrackedResults);
  }
}

void CraqObjectSegmentation::basicWait(std::vector<CraqOperationResult> &allGetResults,std::vector<CraqOperationResult>&allTrackedResults)
{
  std::vector<CraqOperationResult> getResults;
  std::vector<CraqOperationResult> trackedSetResults;
  
  craqDht.tick(getResults,trackedSetResults);
  
  for (int s=0; s < (int)getResults.size(); ++s)
  {
    allGetResults.push_back(getResults[s]);
  }

  for (int s= 0; s < (int)trackedSetResults.size(); ++s)
  {
    allTrackedResults.push_back(trackedSetResults[s]);
  }
}


  /*
    Fill the messageQueueToPushTo deque by pushing outgoing messages from destServers and osegMessages to it
    Then clear osegMessages and destServers.

    may also need to add a message to send queue
  */
  void CraqObjectSegmentation::service(std::map<UUID,ServerID>& updated)
  {
    Timer osegServiceDurTimer;
    osegServiceDurTimer.start();

    
    

//     static Timer serviceTimer;
//     static uint64 serviceCount = 0;
//     static uint64 previousTime = 0;
        
//     if (serviceCount == 0)
//     {
//       serviceTimer.start();
//     }
//     else
//     {
//       if ((serviceCount %1000) == 0)
//       {
//         Duration serviceTimerDur = serviceTimer.elapsed();
//         previousTime = serviceTimerDur.toMilliseconds();
//       }
//       if (((serviceCount-1) %1000) == 0)
//       {
//         Duration serviceTimerDur = serviceTimer.elapsed();
//         std::cout<<"\n\nbftm debug.  Time between craqoseg service calls:   "<< ((int64)serviceTimerDur.toMilliseconds()) - ((int64)previousTime)  << "\n\n";
//       }
//     }
//     ++serviceCount;
    

    
    std::vector<CraqOperationResult> getResults;
    std::vector<CraqOperationResult> trackedSetResults;

    //    iteratedWait(5, getResults,trackedSetResults);
    craqDht.tick(getResults,trackedSetResults);

    Duration tickDur = osegServiceDurTimer.elapsed();

    updated = mFinishedMoveOrLookup;


    //run through all the get results first.
    for (unsigned int s=0; s < getResults.size(); ++s)
    {
      updated[mapDataKeyToUUID[getResults[s].idToString()]]  = getResults[s].servID;
      
      UUID tmper = mapDataKeyToUUID[getResults[s].idToString()];
      std::map<UUID,TransLookup>::iterator iter = mInTransitOrLookup.find(tmper);

      if (iter != mInTransitOrLookup.end()) //means that the object isn't already being looked up and the object isn't already in transit
      {
        //        Duration timerDur = mTimer.elapsed();
        //        mContext->trace->objectSegmentationProcessedRequest(mContext->time, mapDataKeyToUUID[getResults[s].idToString()],getResults[s].servID, mContext->id, (uint32) (((int) timerDur.toMilliseconds()) - (int)(iter->second.timeAdmitted)));
        mContext->trace->objectSegmentationProcessedRequest(mContext->time, mapDataKeyToUUID[getResults[s].idToString()],getResults[s].servID, mContext->id, 0);

        //        mContext->trace->objectSegmentationProcessedRequest(mContext->time, mapDataKeyToUUID[getResults[s].idToString()],getResults[s].servID, mContext->id, (uint32) (((int)mContext->time.raw()) -  (int)(iter->second.timeAdmitted)) );
        mInTransitOrLookup.erase(iter);
      }
      else
      {
        std::cout<<"\n\nbftm debug:  getting results for objects that we were not looking for.  \n\n";
      }
    }
    
    processCraqTrackedSetResults(trackedSetResults);
    mFinishedMoveOrLookup.clear();

    

    Duration osegServiceDuration = osegServiceDurTimer.elapsed();
    if (osegServiceDuration.toMilliseconds() > 1)
    {
      std::cout<<"\n\n CraqObjectSegmentation.service took more than 1 ms. "  <<  osegServiceDuration.toMilliseconds() <<  ".  \n";
      std::cout<<"Tick dur:                   "<< tickDur.toMilliseconds() << "\n";
      std::cout<<"Num get results:            "<< getResults.size() << " \n";
      std::cout<<"Num tracked set results:    "<< trackedSetResults.size() <<"  \n\n\n";
    }
    
  }


  void CraqObjectSegmentation::convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const
  {
    returner[0] = myUniquePrefixKey;
    strncpy(returner+1,obj_id.rawHexData().c_str(),obj_id.rawHexData().size() + 1);
  }


  //This receives messages oseg migrate and acknowledge messages
  void CraqObjectSegmentation::receiveMessage(Message* msg)
  {
    bool correctMessage = false;
    OSegMigrateMessageMove* oseg_move_msg = dynamic_cast<OSegMigrateMessageMove*>(msg);
    if(oseg_move_msg != NULL)
    {
      //received a message to move the object.
      processMigrateMessageMove(oseg_move_msg);
      correctMessage = !correctMessage;
    }

    OSegMigrateMessageAcknowledge* oseg_ack_msg = dynamic_cast<OSegMigrateMessageAcknowledge*>(msg);
    if(oseg_ack_msg != NULL)
    {
      processMigrateMessageAcknowledge(oseg_ack_msg);
      correctMessage = !correctMessage;
    }

    if (! correctMessage)
    {
      printf("\n\nReceived the wrong type of message in receiveMessage of craqobjectsegmentation.cpp.  Or dynamic casting doesn't work.  Shutting down.\n\n ");
      assert(correctMessage);
    }
    delete msg; //delete message here.
  }


  void CraqObjectSegmentation::processMigrateMessageMove(OSegMigrateMessageMove* msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;

    serv_from = msg->getServFrom();
    serv_to   = msg->getServTo();
    obj_id    = msg->getObjID();

    printf("\n\nGot a message that I should not have\n\n");
    assert(false);
  }


  void CraqObjectSegmentation::processMigrateMessageAcknowledge(OSegMigrateMessageAcknowledge* msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;


    serv_from = msg->getServFrom();
    serv_to   = msg->getServTo();
    obj_id    = msg->getObjID();

    //    std::cout<<"\n\n bftm debug: in CraqObjectSegmentation.cpp.  Got an acknowledge.  Acked from " << serv_from << "   acked to:    " << serv_to<< "   obj_id: " << obj_id.toString()<<"\n\n";

    std::map<UUID,TransLookup>::iterator inTransIt;

    inTransIt = mInTransitOrLookup.find(obj_id);
    if (inTransIt != mInTransitOrLookup.end())
    {
      //means that we now remove the obj_id from mInTransit
      mInTransitOrLookup.erase(inTransIt);
      //add it to mFinishedMove.  serv_from
      mFinishedMoveOrLookup[obj_id] = serv_from;

      //log reception of acknowled message
      mContext->trace->objectAcknowledgeMigrate(mContext->time, obj_id,serv_from,mContext->id);
    }

  }


}//namespace CBR
