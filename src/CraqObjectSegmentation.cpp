


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
#include <algorithm>



namespace CBR
{

  
  /*
    Basic constructor
  */
  CraqObjectSegmentation::CraqObjectSegmentation (SpaceContext* ctx, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> getInitArgs, std::vector<CraqInitializeArgs> setInitArgs, char prefixID)
 : ObjectSegmentation(ctx),
   mCSeg (cseg)
{
  
    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->dispatcher()->registerMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher()->registerMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->dispatcher()->registerMessageRecipient(MESSAGE_TYPE_UPDATE_OSEG, this);


    //    craqDht.initialize(initArgs);
    
    craqDhtGet.initialize(getInitArgs);
    craqDhtSet.initialize(setInitArgs);
    
    std::vector<CraqOperationResult*> getResults1;
    std::vector<CraqOperationResult*> trackedSetResults1;

    std::vector<CraqOperationResult*> getResults2;
    std::vector<CraqOperationResult*> trackedSetResults2;

    
    myUniquePrefixKey = prefixID;


    std::map<UUID,ServerID> dummy;
    //500 ticks put here to allow lots of connections to be made
    //    for (int s=0; s < 500; ++s)
//     for (int s=0; s < 2000; ++s)
//     {
//       craqDhtGet.tick(getResults1,trackedSetResults1);
//       processCraqTrackedSetResults(trackedSetResults1, dummy);

//       for (int t=0; t < (int)getResults1.size(); ++t)
//       {
//         delete getResults1[t];
//       }

      
//       craqDhtSet.tick(getResults2,trackedSetResults2);
//       processCraqTrackedSetResults(trackedSetResults2, dummy);

//       for (int t=0; t < (int)getResults2.size(); ++t)
//       {
//         delete getResults2[t];
//       }
      
//     }

    iteratedWait(2000, getResults1, trackedSetResults1);
    processCraqTrackedSetResults(trackedSetResults2, dummy);
    
    for (int t=0; t < (int)getResults1.size(); ++t)
    {
      delete getResults1[t];
    }

    numCacheHits     = 0;
    numOnThisServer  = 0;
    numLookups       = 0;
    numCraqLookups   = 0;
    numTimeElapsedCacheEviction = 0;
    numMigrationNotCompleteYet  = 0;
    numAlreadyLookingUp         = 0;
    

//     CraqDataKey obj_id_as_dht_key;
//     //start loading the objects that are in vectorOfObjectsInitializedOnThisServer into the dht.
//     for (int s=0;s < (int)vectorOfObjectsInitializedOnThisServer.size(); ++s)
//     {
//       convert_obj_id_to_dht_key(vectorOfObjectsInitializedOnThisServer[s],obj_id_as_dht_key);

//       CraqDataSetGet cdSetGet(obj_id_as_dht_key, mContext->id(),false,CraqDataSetGet::SET);
//       craqDht.set(cdSetGet);
//       printf("\nObject %i of %i\n",s+1,(int)(vectorOfObjectsInitializedOnThisServer.size()) );

//       mObjects.push_back(vectorOfObjectsInitializedOnThisServer[s]);        //also need to load those objects into local object storage.
//     }

    
    
    //50 ticks to update
//     for (int s=0; s < 2000; ++s)
//     {
//       craqDhtGet.tick(getResults1,trackedSetResults1);
//       processCraqTrackedSetResults(trackedSetResults1, dummy);

//       for (int t=0; t < (int)getResults1.size(); ++t)
//       {
//         delete getResults1[t];
//       }

//       craqDhtSet.tick(getResults2,trackedSetResults2);
//       processCraqTrackedSetResults(trackedSetResults2, dummy);

//       for (int t=0; t < (int)getResults2.size(); ++t)
//       {
//         delete getResults2[t];
//       }
      
//     }

    mTimer.start();
  }

  /*
    Destructor
  */
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
    mContext->dispatcher()->unregisterMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher()->unregisterMessageRecipient(MESSAGE_TYPE_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->dispatcher()->unregisterMessageRecipient(MESSAGE_TYPE_UPDATE_OSEG, this);

    std::cout<<"\n\n\nIN DESTRUCTOR \n\n";
    mContext->trace()->processOSegShutdownEvents(mContext->time,mContext->id(), numLookups,numOnThisServer,numCacheHits, numCraqLookups, numTimeElapsedCacheEviction,    numMigrationNotCompleteYet);

    printf("\n\nREMAINING IN QUEUE GET:  %i     SET:  %i\n\n", (int) craqDhtGet.queueSize(),(int) craqDhtSet.queueSize());
    fflush(stdout);
    printf("\n\nNUM ALREADY LOOKING UP:  %i\n\n",numAlreadyLookingUp);
    fflush(stdout);
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
    //    mFinishedMoveOrLookup[obj_id]= mContext->id;

    return true;
  }


  /*
    If returns true, means that the object isn't already undergoing a migration, and is therefore clear to migrate.
  */
  bool CraqObjectSegmentation::clearToMigrate(const UUID& obj_id)
  {
    std::map<UUID,TransLookup>::iterator mInTransIter = mInTransitOrLookup.find(obj_id);

    bool migratingFromHere = false;
    if (mInTransIter != mInTransitOrLookup.end())
    {
      //means that the object is either being looked up or in transit
      if (mInTransIter->second.sID != CRAQ_OSEG_LOOKUP_SERVER_ID)
      {
        //means that the object is migrating from here.
        migratingFromHere = true;
      }
    }

    bool migratingToHere = std::find(mReceivingObjects.begin(), mReceivingObjects.end(), obj_id) != mReceivingObjects.end();
    //means that the object migrating to here has not yet received an acknowledge, and therefore shouldn't begin migrating again.

    //clear to migrate only if migrating from here and migrating to here are false.
    return (!migratingFromHere) && (!migratingToHere);

    //
    //    return (std::find(mReceivingObjects.begin(), mReceivingObjects.end(), obj_id) == mReceivingObjects.end());
  }



  //this call returns true if the object is migrating from this server to another server, but hasn't yet received an ack message (which will disconnect the object connection.)
  bool CraqObjectSegmentation::checkMigratingFromNotCompleteYet(const UUID& obj_id)
  {
    std::map<UUID,TransLookup>::const_iterator iterInTransOrLookup = mInTransitOrLookup.find(obj_id);

    if (iterInTransOrLookup == mInTransitOrLookup.end())
      return false;

    if (iterInTransOrLookup->second.sID != CRAQ_OSEG_LOOKUP_SERVER_ID)
      return true;

    return false;
  }


  /*
    Checks to see whether have a reasonably up-to-date cache value to return to
    
   */
  ServerID CraqObjectSegmentation::satisfiesCache(const UUID& obj_id)
  {

#ifndef CRAQ_CACHE
    return NullServerID;
#endif
    
    ObjectCacheMap::iterator objCacheIter = mServerObjectCache.find(obj_id);

    if (objCacheIter == mServerObjectCache.end()) //object not in cache map
      return NullServerID;

    uint64 timeElapsed =  (uint64) (((long int) mContext->time.raw())/1000.0)  - ((long int)objCacheIter->second.timeStamp);

    ++numTimeElapsedCacheEviction;

    
    //    if ((timeElapsed < 1000)&&(objCacheIter->second.lastLookup))
    if ((timeElapsed < 8000)&&(objCacheIter->second.lastLookup))
    {
      //if the time since absorbed a cache value is less than one second
      //and if the last cache value came from one of our personal lookups.
      //then return the cached server id.
      return objCacheIter->second.sID;
    }

    if (timeElapsed < 5000)
    {
      //if received an update message less than half a second ago, then also
      //return the cached server id.
      return objCacheIter->second.sID;
    }

    //otherwise, just tell them that I don't really know.
    return NullServerID;
  }

  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
  */
  ServerID CraqObjectSegmentation::lookup(const UUID& obj_id)
  {

    ++numLookups;
    
    if (checkOwn(obj_id))  //this call just checks through to see whether the object is on this space server.
    {
      ++numOnThisServer;
      return mContext->id();

    }

    if (checkMigratingFromNotCompleteYet(obj_id))//this call just checks to see whether the object is migrating from this server to another server.  If it is, but hasn't yet received an ack message to disconnect the object connection.
    {
      ++numMigrationNotCompleteYet;
      return mContext->id();
    }

    ServerID cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn != NullServerID) && (cacheReturn != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {
#ifdef CRAQ_DEBUG
      std::cout<<"\n\nbftm debug: cached value object id:"<< obj_id.toString()  <<"   cached value returned:  " << cacheReturn<<" \n\n";
#endif
      
      ++numCacheHits;
      return cacheReturn;

    }
    
    UUID tmper = obj_id;
    std::map<UUID,TransLookup>::const_iterator iter = mInTransitOrLookup.find(tmper);

    if (iter == mInTransitOrLookup.end()) //means that the object isn't already being looked up and the object isn't already in transit
    {
      //if object is not in transit, lookup its location in the dht.  returns -1 if object doesn't exist.
      //add the mapping of a craqData Key to a uuid.

      ++numCraqLookups;
      
      std::string indexer = "";
      indexer.append(1,myUniquePrefixKey);
      indexer.append(tmper.rawHexData());

      CraqDataSetGet cdSetGet (indexer,0,false,CraqDataSetGet::GET); //bftm modified

      mapDataKeyToUUID[indexer] = tmper; //changed here.

      //      craqDht.get(cdSetGet); //calling the craqDht to do a get.
      craqDhtGet.get(cdSetGet); //calling the craqDht to do a get.

      
      mContext->trace()->objectSegmentationLookupRequest(mContext->time, obj_id, mContext->id());
      //      mContext->trace->objectSegmentationLookupRequest(timerDur.toMilliseconds(), obj_id, mContext->id);

      //also need to say that the object is in transit or lookup.

      Duration timerDur = mTimer.elapsed();
      TransLookup tmpTransLookup;
      tmpTransLookup.sID = CRAQ_OSEG_LOOKUP_SERVER_ID;  //means that we're performing a lookup, rather than a migrate.
      tmpTransLookup.timeAdmitted = (int)timerDur.toMilliseconds();

      mInTransitOrLookup[tmper] = tmpTransLookup; //just says that we are performing a lookup on the object
    }
    else
    {
      ++numAlreadyLookingUp;
    }
    
    
    //means that we have to perform an external craq lookup
    return NullServerID;

  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  OSegMigrateMessageAcknowledge* CraqObjectSegmentation::generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to)
  {
    OSegMigrateMessageAcknowledge* oseg_ack_msg = new OSegMigrateMessageAcknowledge(mContext->id(),  mContext->id(),  sID_to, sID_to,    mContext->id(),obj_id);

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

       Note: will need to re-write the above.  It's significantly outdated
  */
  void CraqObjectSegmentation::addObject(const UUID& obj_id, const ServerID idServerAckTo, bool generateAck)
  {
    if (generateAck)
    {
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, mContext->id() ,true,CraqDataSetGet::SET);

#ifdef CRAQ_DEBUG
      std::cout<<"\n\n Received an addObject\n\n";
#endif

      if (std::find(mReceivingObjects.begin(),mReceivingObjects.end(),obj_id) == mReceivingObjects.end())//shouldn't need to check if it already exists, but may as well.
      {
        mReceivingObjects.push_back(obj_id);  //means that this object has been pushed to this server, but its migration isn't complete yte.
      }

      TrackedSetResultsData tsrd;
      tsrd.migAckMsg = generateAcknowledgeMessage(obj_id,idServerAckTo);
      tsrd.dur       = mTimer.elapsed();
      
      //      int trackID = craqDht.set(cdSetGet);
      int trackID = craqDhtSet.set(cdSetGet);
      //      trackingMessages[trackID] = generateAcknowledgeMessage(obj_id, idServerAckTo);
      trackingMessages[trackID] = tsrd;
      
    }
    else
    {
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, mContext->id() ,false,CraqDataSetGet::SET);
      //      int trackID = craqDht.set(cdSetGet);
      int trackID = craqDhtSet.set(cdSetGet);

      
      if (std::find(mObjects.begin(), mObjects.end(), obj_id) == mObjects.end())
      {
        mObjects.push_back(obj_id);
        std::cout<<"\n\nAdding object:  "<<obj_id.toString()<<"\n";
      }

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
    mContext->trace()->objectBeginMigrate(mContext->time,obj_id,mContext->id(),new_server_id); //log it.

    std::map<UUID,TransLookup>::const_iterator transIter = mInTransitOrLookup.find(obj_id);

#ifdef CRAQ_DEBUG
    if (transIter != mInTransitOrLookup.end())
    {
      std::cout<<"\n\n The object was already in lookup or transit \n\n  ";
    }
#endif


    //if we do, then say that the object is in transit.
    TransLookup tmpTransLookup;
    tmpTransLookup.sID = new_server_id;

    Duration tmpDurer= mTimer.elapsed();
    //    tmpTransLookup.timeAdmitted = mContext->time.raw();
    tmpTransLookup.timeAdmitted = (int)tmpDurer.toMilliseconds();

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

#ifdef CRAQ_DEBUG
      std::cout<<"\n\nThe object clearly wasn't registered on this server.  This is obj id:  " <<  obj_id.toString() <<  ".  This is time:   " <<mContext->time.raw() << " Oh no.   ";

      if (clearToMigrate(obj_id))
      {
        std::cout<<"  Likely a problem with clear to migrate\n\n";
      }
      else
      {
        std::cout<<"\n\n clear to migrate is fine migration is being called from somewhere besides server.cpp\n\n";
      }
#endif
    }
  }



  void CraqObjectSegmentation::processCraqTrackedSetResults(std::vector<CraqOperationResult*> &trackedSetResults, std::map<UUID,ServerID>& updated)
  {

#ifdef CRAQ_DEBUG
    if (trackedSetResults.size() !=0)
      std::cout<<"\n\nbftm debug:  got this many trackedsetresults:   "<<trackedSetResults.size()<<"\n\n";
#endif

    for (unsigned int s=0; s < trackedSetResults.size();  ++s)
    {
      
      //genrateAcknowledgeMessage(uuid,sidto);  ...we may as well hold onto the object pointer then.
      if (trackedSetResults[s]->trackedMessage != 0) //if equals zero, meant that we weren't supposed to be tracking this message.
      {
        if (trackingMessages.find(trackedSetResults[s]->trackedMessage) !=  trackingMessages.end())
        {
          //means that we were tracking this message. and that we'll have to send an acknowledge

          OSegCacheVal cacheVal;
          cacheVal.sID          = mContext->id();
          cacheVal.timeStamp    = (mContext->time.raw())/1000.0;
          cacheVal.lastLookup   = true;
          mServerObjectCache[trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID()] = cacheVal;
          
          //add to mObjects the uuid associated with trackedMessage.
          mObjects.push_back(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID());

          //will also need to make sure that updated has the most recent copy of the location of the message.
          if(updated.find(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID()) != updated.end())
          {
            //The serverid in update may be from a stale return from craq.  Therefore, just overwrite the updated function with our serverid (because the object should now be hosted on this space server).
            updated[trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID()] = mContext->id();
          }

          //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
          std::map<UUID,TransLookup>::iterator inTransLookIter = mInTransitOrLookup.find(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID());

          if (inTransLookIter != mInTransitOrLookup.end())
          {
            //means that object can now be removed from mInTransitOrLookup
            mInTransitOrLookup.erase(inTransLookIter);
          }
          //finished deleting from mInTransitOrLookup


          //remove this object from mReceivingObjects, indicating that this object can now be safely migrated from this server to another if need be.
          std::vector<UUID>::iterator recObjIter = std::find(mReceivingObjects.begin(), mReceivingObjects.end(), trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID());
          if (recObjIter != mReceivingObjects.end())
          {
            //the object should be removed from receiving objects
            mReceivingObjects.erase(recObjIter);
          }
          else
          {
            std::cout<<"\n\nbftm debug:  Didn't realize that we were setting this object.\n\n";
          }
          //done removing from receivingObjects.

          
#ifdef CRAQ_DEBUG
          std::cout<<"\n\nbftm debug:  sending an acknowledge out from  "<<  mContext->id();
          std::cout<<"  to  "<<          trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getMessageDestination();
          std::cout<<"  for object  " << trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID().toString();
          std::cout<< "   at time:  " << mContext->time.raw() <<"\n\n";
#endif

          Duration procTrackedSetRes = mTimer.elapsed();
          int durMs = procTrackedSetRes.toMilliseconds() - trackingMessages[trackedSetResults[s]->trackedMessage].dur.toMilliseconds();
          
          mContext->trace()->processOSegTrackedSetResults(mContext->time, trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getObjID(), mContext->id(), durMs);
          
          mContext->router()->route(MessageRouter::MIGRATES,trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg,trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getMessageDestination(),false);//send an acknowledge message to space server that formerly hosted object.

            

          trackingMessages.erase(trackedSetResults[s]->trackedMessage);//stop tracking this message.
        }
        else
        {
          std::cout<<"\n\nbftm debug:  received a tracked set that I don't have a record of an acknowledge to send.\n\n";
        }
      }


      //delete the tracked set result
      delete trackedSetResults[s];

    }
  }





void CraqObjectSegmentation::iteratedWait(int numWaits,std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
{
  for (int s=0; s < numWaits; ++s)
  {
    basicWait(allGetResults,allTrackedResults);
  }
}

void CraqObjectSegmentation::basicWait(std::vector<CraqOperationResult*> &allGetResults,std::vector<CraqOperationResult*>&allTrackedResults)
{
  std::vector<CraqOperationResult*> getResults;
  std::vector<CraqOperationResult*> trackedSetResults;

  craqDhtGet.tick(getResults,trackedSetResults);

  for (int s=0; s < (int)getResults.size(); ++s)
  {
    allGetResults.push_back(getResults[s]);
  }

  for (int s= 0; s < (int)trackedSetResults.size(); ++s)
  {
    allTrackedResults.push_back(trackedSetResults[s]);
  }


  std::vector<CraqOperationResult*> getResults2;
  std::vector<CraqOperationResult*> trackedSetResults2;

  craqDhtSet.tick(getResults2,trackedSetResults2);

  for (int s=0; s < (int)getResults2.size(); ++s)
  {
    allGetResults.push_back(getResults2[s]);
  }

  for (int s= 0; s < (int)trackedSetResults2.size(); ++s)
  {
    allTrackedResults.push_back(trackedSetResults2[s]);
  }

  
}


  /*
    Fill the messageQueueToPushTo deque by pushing outgoing messages from destServers and osegMessages to it
    Then clear osegMessages and destServers.

    may also need to add a message to send queue
  */
  void CraqObjectSegmentation::service(std::map<UUID,ServerID>& updated)
  {

    checkNotFoundData();
    
    static Timer servTimer;
    static uint numTicks = 0;
    static uint numTicksGreaterThanMs = 0;
    static uint numBetweenTicksGreaterThan3 = 0;
    static uint prevTime =0;


    if (numTicks != 0)
    {
      servTimer.start();
    }
    else
    {
      Duration thisDur = servTimer.elapsed();

      if(thisDur.toMilliseconds() - prevTime > 3)
      {
        ++numBetweenTicksGreaterThan3;
      }

      prevTime = thisDur.toMilliseconds();
    }

    ++numTicks;

    Timer osegServiceDurTimer;
    osegServiceDurTimer.start();


    std::vector<CraqOperationResult*> getResults;
    std::vector<CraqOperationResult*> trackedSetResults;

    iteratedWait(6, getResults,trackedSetResults);
    //    craqDht.tick(getResults,trackedSetResults);

    Duration tickDur = osegServiceDurTimer.elapsed();

    if (mFinishedMoveOrLookup.size() !=0)
    {
      updated.swap(mFinishedMoveOrLookup);
      //updated = mFinishedMoveOrLookup;
    }

    int sizeGetRes  = (int) getResults.size();
    int sizeTracked = (int) trackedSetResults.size();

    if (sizeGetRes != 0)
    {
      std::cout<<"\n\nGot a bunch of responses" <<sizeGetRes <<"\n\n";
      std::cout<<"\n\nNum remaining in queue get:  "<<craqDhtGet.queueSize()<<"     set:  " << craqDhtSet.queueSize()<<"    time:  "<< mContext->time.raw() <<"\n\n";
    }

    
    //run through all the get results first.
    for (unsigned int s=0; s < getResults.size(); ++s)
    {
      if (getResults[s]->servID == NullServerID)
      {
        notFoundFunction(getResults[s]);
        delete getResults[s];
        continue;
      }

      
      updated[mapDataKeyToUUID[getResults[s]->idToString()]]  = getResults[s]->servID;

      UUID tmper = mapDataKeyToUUID[getResults[s]->idToString()];
      std::map<UUID,TransLookup>::iterator iter = mInTransitOrLookup.find(tmper);

      //put this value in the cache
      OSegCacheVal cacheVal;
      cacheVal.sID = getResults[s]->servID;
      cacheVal.timeStamp = (mContext->time.raw())/1000.0;
      cacheVal.lastLookup = true;
      
      mServerObjectCache[tmper] = cacheVal;


      if (iter != mInTransitOrLookup.end()) //means that the object was already being looked up or in transit
      {
        //log message stating that object was processed.
        Duration timerDur = mTimer.elapsed();
        mContext->trace()->objectSegmentationProcessedRequest(mContext->time, mapDataKeyToUUID[getResults[s]->idToString()],getResults[s]->servID, mContext->id(), (uint32) (((int) timerDur.toMilliseconds()) - (int)(iter->second.timeAdmitted)), (uint32) craqDhtGet.queueSize()  );

        if(iter->second.sID ==  CRAQ_OSEG_LOOKUP_SERVER_ID)
        {
          //means that after receiving a lookup request, we did not intermediarilly receive a migrate request.
          //if above predicate is not true, means that we did receive a migrate request, and therefore should not delete that migrate request from mInTransitOrLookup (ie do nothing).
          mInTransitOrLookup.erase(iter);
        }
      }
      else
      {
        std::cout<<"\n\nbftm debug:  getting results for objects that we were not looking for.  Or object lookups that have been short-circuited by a trackedSet\n\n";
      }

      delete getResults[s];
    }

    processCraqTrackedSetResults(trackedSetResults, updated);

    if( mFinishedMoveOrLookup.size() != 0)
      mFinishedMoveOrLookup.clear();


#ifdef CRAQ_DEBUG
    Duration osegServiceDuration = osegServiceDurTimer.elapsed();
    if (osegServiceDuration.toMilliseconds() > 1)
    {
      std::cout<<"\n\n CraqObjectSegmentation.service took more than 1 ms. "  <<  osegServiceDuration.toMilliseconds() <<  ".  \n";
      std::cout<<"Tick dur:                   "<< tickDur.toMilliseconds() << "\n";
      std::cout<<"Num get results:            "<< sizeGetRes << " \n";
      std::cout<<"Num tracked set results:    "<<  sizeTracked <<"  \n";
      std::cout<<"Num ticks here:             "<< numTicks<<"\n\n\n";
      ++numTicksGreaterThanMs;
    }

    if ((numTicks %1000) == 0)
    {
      std::cout<<"\n\nHere are the numbers:   \n";
      std::cout<<"\t num greater than ms:       "<< numTicksGreaterThanMs<<"\n";
      std::cout<<"\t num between ticks:         " << numBetweenTicksGreaterThan3 << "\n";
      std::cout<<"\t num total:                 "<< numTicks<<"\n\n\n";
    }
#endif
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

    UpdateOSegMessage* update_oseg_msg = dynamic_cast <UpdateOSegMessage*>(msg);
    if (update_oseg_msg != NULL)
    {
      processUpdateOSegMessage(update_oseg_msg);
      correctMessage = !correctMessage;
    }

    
    if (! correctMessage)
    {
      printf("\n\nReceived the wrong type of message in receiveMessage of craqobjectsegmentation.cpp.  Or dynamic casting doesn't work.  Shutting down.\n\n ");
      assert(correctMessage);
    }
    delete msg; //delete message here.
  }


//
void CraqObjectSegmentation::processUpdateOSegMessage(UpdateOSegMessage* update_oseg_msg)
{
  OSegCacheVal cacheVal;
  cacheVal.timeStamp    =  (mContext->time.raw())/1000.0;
  cacheVal.sID          =  update_oseg_msg->contents.servid_obj_on();
  cacheVal.lastLookup   =  false;

#ifdef CRAQ_DEBUG
  std::cout<<"\n\n got a processUpdateOSegMessage time received  "<< mContext->time.raw()<<"\n";
  std::cout<<"\ttime stamped:  "<<cacheVal.timeStamp <<" \n ";
  std::cout<<"\tsID "  <<cacheVal.sID   <<"\n\n";
#endif
  
  mServerObjectCache[update_oseg_msg->contents.m_objid()] = cacheVal;
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


    std::map<UUID,TransLookup>::iterator inTransIt;

    inTransIt = mInTransitOrLookup.find(obj_id);
    if (inTransIt != mInTransitOrLookup.end())
    {
      //means that we now remove the obj_id from mInTransit
      mInTransitOrLookup.erase(inTransIt);
      //add it to mFinishedMove.  serv_from
      mFinishedMoveOrLookup[obj_id] = serv_from;

      //log reception of acknowled message
      mContext->trace()->objectAcknowledgeMigrate(mContext->time, obj_id,serv_from,mContext->id());
    }

    //send a message to the server that object should now disconnect

    KillObjConnMessage* killConnMsg = new KillObjConnMessage(mContext->id());
    killConnMsg->contents.set_m_objid(obj_id);
    mContext->router()->route(MessageRouter::MIGRATES,killConnMsg,mContext->id());//this will route the message to the server so that this computer can disconnect from the object connection.

  }

int CraqObjectSegmentation::getOSegType()
{
  return CRAQ_OSEG;
}


//this function tells us what to do with all the ids that just weren't found in craq.
void CraqObjectSegmentation::notFoundFunction(CraqOperationResult* nf)
{
  //turn the id into a uuid and then push it onto the end of  mNfData queue.

  if (mapDataKeyToUUID.find(nf->idToString()) == mapDataKeyToUUID.end())
    return; //means that we really have no record of this objects' even having been requested.


  NotFoundData* nfd = new NotFoundData();
  nfd->obj_id =  mapDataKeyToUUID[nf->idToString()];
  nfd->dur    =  mTimer.elapsed();
  mNfData.push(nfd);
}


//this function tells us if there are any ids that have waited long enough for it to be safe to query for the again.
void CraqObjectSegmentation::checkNotFoundData()
{
  //look at the first element of the queue.  if the time hasn't been sufficiently long, return.  if the time has been sufficiently long, then go ahead and directly request asyncCraq to perform another lookup.

  if (mNfData.size() == 0)
    return;
  
  Duration tmpDur = mTimer.elapsed();


  bool queueDataOldEnough = true;
  NotFoundData* nfd;
  
  while (queueDataOldEnough)
  {
    nfd = mNfData.front();
    if ((tmpDur.toMilliseconds() - nfd->dur.toMilliseconds()) > 500) //we wait 500 ms.
    {
      queueDataOldEnough = true;

      //perform a direct craq get call
      std::string indexer = "";
      indexer.append(1,myUniquePrefixKey);
      indexer.append(nfd->obj_id.rawHexData());

      CraqDataSetGet cdSetGet (indexer,0,false,CraqDataSetGet::GET); //bftm modified
      craqDhtGet.get(cdSetGet); //calling the craqDht to do a get.    

      mNfData.pop(); //remove the item from the queue.


      std::cout<<"\n\nbftm debug  I'm releasing one; obj_id:  "<<nfd->obj_id.toString()<<"\n";
      
      delete nfd; //memory managment
    }

    queueDataOldEnough = queueDataOldEnough && (mNfData.size() > 0);  //don't keep going if you're out of objects.
  }
}


}//namespace CBR
