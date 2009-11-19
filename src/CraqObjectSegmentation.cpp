


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
#include "CraqCacheGood.hpp"
#include <boost/thread/mutex.hpp>

namespace CBR
{

  /*
    Basic constructor
  */
  CraqObjectSegmentation::CraqObjectSegmentation (SpaceContext* ctx, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> getInitArgs, std::vector<CraqInitializeArgs> setInitArgs, char prefixID, IOStrand* o_strand, IOStrand* strand_to_post_to)
 : ObjectSegmentation(ctx, o_strand),
   mCSeg (cseg),
   craqDhtGet(ctx, o_strand),
   craqDhtSet(ctx, o_strand),
   postingStrand(strand_to_post_to)
{

    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->dispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->dispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);


    craqDhtGet.initialize(getInitArgs);
    craqDhtSet.initialize(setInitArgs);

    std::vector<CraqOperationResult*> getResults1;
    std::vector<CraqOperationResult*> trackedSetResults1;

    std::vector<CraqOperationResult*> getResults2;
    std::vector<CraqOperationResult*> trackedSetResults2;


    myUniquePrefixKey = prefixID;


    std::map<UUID,ServerID> dummy;

    iteratedWait(CRAQ_INIT_WAIT_TIME, getResults1, trackedSetResults1);
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
    numServices                 = 0;
    numLookingUpDebug           = 0;
    mServiceTimer.start();
    mTimer.start();
  }

  /*
    Destructor
  */
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
    mContext->dispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->dispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->dispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

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
    //set a mutex lock.

    inTransOrLookup_m.lock();
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
    inTransOrLookup_m.unlock();
    
    receivingObjects_m.lock();
    bool migratingToHere = std::find(mReceivingObjects.begin(), mReceivingObjects.end(), obj_id) != mReceivingObjects.end();
    //means that the object migrating to here has not yet received an acknowledge, and therefore shouldn't begin migrating again.
    receivingObjects_m.unlock();
    
    //clear to migrate only if migrating from here and migrating to here are false.
    return (!migratingFromHere) && (!migratingToHere);
  }



  //this call returns true if the object is migrating from this server to another server, but hasn't yet received an ack message (which will disconnect the object connection.)
  bool CraqObjectSegmentation::checkMigratingFromNotCompleteYet(const UUID& obj_id)
  {
    inTransOrLookup_m.lock();
    std::map<UUID,TransLookup>::const_iterator iterInTransOrLookup = mInTransitOrLookup.find(obj_id);

    if (iterInTransOrLookup == mInTransitOrLookup.end())
    {
      inTransOrLookup_m.unlock();
      return false;
    }
    if (iterInTransOrLookup->second.sID != CRAQ_OSEG_LOOKUP_SERVER_ID)
    {
      inTransOrLookup_m.unlock();
      return true;
    }

    inTransOrLookup_m.unlock();
    return false;
  }


  //checks value against cache.
  //should only be called from the "postingStrand"
  ServerID CraqObjectSegmentation::satisfiesCache(const UUID& obj_id)
  {

#ifndef CRAQ_CACHE
    return NullServerID;
#endif

    return mCraqCache.get(obj_id);
  }




  void CraqObjectSegmentation::newObjectAdd(const UUID& obj_id)
  {
    CraqDataKey cdk;
    convert_obj_id_to_dht_key(obj_id,cdk);

    CraqDataSetGet cdSetGet(cdk, mContext->id() ,true,CraqDataSetGet::SET);

    TrackedSetResultsDataAdded tsrda;
    tsrda.msgAdded = generateAddedMessage(obj_id);
    tsrda.dur       = mTimer.elapsed();

    int trackID = craqDhtSet.set(cdSetGet);
    trackedAddMessages[trackID] = tsrda;

    return;
  }


  CBR::Protocol::OSeg::AddedObjectMessage* CraqObjectSegmentation::generateAddedMessage(const UUID& obj_id)
  {
      CBR::Protocol::OSeg::AddedObjectMessage* oadd = new CBR::Protocol::OSeg::AddedObjectMessage();
      oadd->set_m_objid(obj_id);
    return oadd;
  }


  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
    Only called from postingStrand
  */
  ServerID CraqObjectSegmentation::lookup(const UUID& obj_id)
  {
    ++numLookups;

    if (checkOwn(obj_id))  //this call just checks through to see whether the object is on this space server.
    {
      ++numOnThisServer;
      return mContext->id();
    }


    mContext->trace()->objectSegmentationLookupNotOnServerRequest(mContext->time, obj_id, mContext->id()); //log the request.


    if (checkMigratingFromNotCompleteYet(obj_id))//this call just checks to see whether the object is migrating from this server to another server.  If it is, but hasn't yet received an ack message to disconnect the object connection.
    {
      ++numMigrationNotCompleteYet;
      return mContext->id();
    }


    ServerID cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn != NullServerID) && (cacheReturn != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {
      mContext->trace()->osegCacheResponse(mContext->time,cacheReturn,obj_id);
      ++numCacheHits;
      return cacheReturn;
    }

    oStrand->post(boost::bind(&CraqObjectSegmentation::beginCraqLookup,this,obj_id));

    return NullServerID;
  }

  void CraqObjectSegmentation::beginCraqLookup(const UUID& obj_id)
  {
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


      craqDhtGet.get(cdSetGet); //calling the craqDht to do a get.

      //      std::cout<<"\nCraq lookup for "<<obj_id.toString()<<"\n";

      mContext->trace()->objectSegmentationCraqLookupRequest(mContext->time, obj_id, mContext->id());

      ++numLookingUpDebug;

      //also need to say that the object is in transit or lookup.

      Duration timerDur = mTimer.elapsed();
      TransLookup tmpTransLookup;
      tmpTransLookup.sID = CRAQ_OSEG_LOOKUP_SERVER_ID;  //means that we're performing a lookup, rather than a migrate.
      tmpTransLookup.timeAdmitted = (int)timerDur.toMilliseconds();
      //tmpTransLookup.timeAdmitted = numServices;
      
      mInTransitOrLookup[tmper] = tmpTransLookup; //just says that we are performing a lookup on the object
    }
    else
    {
      ++numAlreadyLookingUp;
    }

  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  CBR::Protocol::OSeg::MigrateMessageAcknowledge* CraqObjectSegmentation::generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to)
  {
      CBR::Protocol::OSeg::MigrateMessageAcknowledge* oseg_ack_msg = new CBR::Protocol::OSeg::MigrateMessageAcknowledge();
      oseg_ack_msg->set_m_servid_from(mContext->id());
      oseg_ack_msg->set_m_servid_to(sID_to);
      oseg_ack_msg->set_m_message_destination(sID_to);
      oseg_ack_msg->set_m_message_from(mContext->id());
      oseg_ack_msg->set_m_objid(obj_id);

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

      receivingObjects_m.lock();
      if (std::find(mReceivingObjects.begin(),mReceivingObjects.end(),obj_id) == mReceivingObjects.end())//shouldn't need to check if it already exists, but may as well.
      {
        mReceivingObjects.push_back(obj_id);  //means that this object has been pushed to this server, but its migration isn't complete yet.
      }
      receivingObjects_m.unlock();

      TrackedSetResultsData tsrd;
      tsrd.migAckMsg = generateAcknowledgeMessage(obj_id,idServerAckTo);
      tsrd.dur       = mTimer.elapsed();

      int trackID = craqDhtSet.set(cdSetGet);
      trackingMessages[trackID] = tsrd;

    }
    else
    {
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, mContext->id() ,false,CraqDataSetGet::SET);
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
    //        tmpTransLookup.timeAdmitted = numServices;

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
        std::cout<<"  Likely a problem with clear to migrate\n\n";
      else
        std::cout<<"\n\n clear to migrate is fine migration is being called from somewhere besides server.cpp\n\n";

#endif
    }
  }



  void CraqObjectSegmentation::processCraqTrackedSetResults(std::vector<CraqOperationResult*> &trackedSetResults, std::map<UUID,ServerID>& updated)
  {
    for (unsigned int s=0; s < trackedSetResults.size();  ++s)
    {
      //genrateAcknowledgeMessage(uuid,sidto);  ...we may as well hold onto the object pointer then.
      if (trackedSetResults[s]->trackedMessage != 0) //if equals zero, meant that we weren't supposed to be tracking this message.
      {
        if (trackingMessages.find(trackedSetResults[s]->trackedMessage) !=  trackingMessages.end())
        {
          //means that we were tracking this message. and that we'll have to send an acknowledge

          //add the value to the cache
          //          mCraqCache.insert(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid(), mContext->id());
          
          postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid(), mContext->id()));

          
          //add to mObjects the uuid associated with trackedMessage.
          mObjects.push_back(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid());

          //will also need to make sure that updated has the most recent copy of the location of the message.
          if(updated.find(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid()) != updated.end())
          {
            //The serverid in update may be from a stale return from craq.  Therefore, just overwrite the updated function with our serverid (because the object should now be hosted on this space server).
            updated[trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid()] = mContext->id();
          }

          inTransOrLookup_m.lock();
          //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
          std::map<UUID,TransLookup>::iterator inTransLookIter = mInTransitOrLookup.find(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid());

          if (inTransLookIter != mInTransitOrLookup.end())
          {
            //means that object can now be removed from mInTransitOrLookup
#ifdef CRAQ_DEBUG
            std::cout<<"\n\nbftm debug: short-circuiting for object:  " << trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid().toString() <<"\n\n";
#endif
            mInTransitOrLookup.erase(inTransLookIter);
          }
          //finished deleting from mInTransitOrLookup
          inTransOrLookup_m.unlock();
          

          //remove this object from mReceivingObjects, indicating that this object can now be safely migrated from this server to another if need be.
          receivingObjects_m.lock();
          std::vector<UUID>::iterator recObjIter = std::find(mReceivingObjects.begin(), mReceivingObjects.end(), trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid());
          if (recObjIter != mReceivingObjects.end())
          {
            //the object should be removed from receiving objects
            mReceivingObjects.erase(recObjIter);
          }
          else
          {
#ifdef CRAQ_DEBUG
            std::cout<<"\n\nbftm debug:  Didn't realize that we were setting this object.\n\n";
#endif
          }
          //done removing from receivingObjects.

          receivingObjects_m.unlock();
          

#ifdef CRAQ_DEBUG
          std::cout<<"\n\nbftm debug:  sending an acknowledge out from  "<<  mContext->id();
          std::cout<<"  to  "<<          trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getMessageDestination();
          std::cout<<"  for object  " << trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid().toString();
          std::cout<< "   at time:  " << mContext->time.raw() <<"\n\n";
#endif

          Duration procTrackedSetRes = mTimer.elapsed();
          int durMs = procTrackedSetRes.toMilliseconds() - trackingMessages[trackedSetResults[s]->trackedMessage].dur.toMilliseconds();

          mContext->trace()->processOSegTrackedSetResults(mContext->time, trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid(), mContext->id(), durMs);


          Message* to_send = new Message(
              mContext->id(),
              SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
              trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_message_destination(),
              SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
              serializePBJMessage( *(trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg) )
          );
           bool sent= mContext->router()->route(MessageRouter::MIGRATES,to_send,false);//send an acknowledge message to space server that formerly hosted object.

           if (!sent)
           {
             //             printf("\n\nbftm debug: ERROR had problems sending migrate ack msg for object %s to %i \n\n", trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->m_objid().toString().c_str(), trackingMessages[trackedSetResults[s]->trackedMessage].migAckMsg->getMessageDestination() );
             //             fflush(stdout);
             reTryMigAckMessage.push_back(to_send); //will try to re-send the tracking message
           }

          trackingMessages.erase(trackedSetResults[s]->trackedMessage);//stop tracking this message.
        }
        else if (trackedAddMessages.find(trackedSetResults[s]->trackedMessage) != trackedAddMessages.end())
        {
          //means that we just finished adding first object
          //bftm single line-change
          mObjects.push_back(trackedAddMessages[trackedSetResults[s]->trackedMessage].msgAdded->m_objid() );//need to add obj_id

          Message* to_send = new Message(
              mContext->id(),
              SERVER_PORT_OSEG_ADDED_OBJECT,
              mContext->id(),
              SERVER_PORT_OSEG_ADDED_OBJECT,
              serializePBJMessage( *(trackedAddMessages[trackedSetResults[s]->trackedMessage].msgAdded) )
          );

          bool sent = mContext->router()->route(MessageRouter::MIGRATES, to_send,false);
          if (!sent)
          {
            //            printf("\n\nbftm debug: error here: obj: %s", trackedAddMessages[trackedSetResults[s]->trackedMessage].msgAdded->contents.m_objid().toString().c_str());
            //            fflush(stdout);
            reTryAddedMessage.push_back(to_send);  //will try to re-send the add message
          }
        }
        else
        {
#ifdef CRAQ_DEBUG
          std::cout<<"\n\nbftm debug:  received a tracked set that I don't have a record of an acknowledge to send.\n\n";
#endif
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

  allGetResults.insert(allGetResults.end(), getResults.begin(), getResults.end());
  allTrackedResults.insert(allTrackedResults.end(), trackedSetResults.begin(), trackedSetResults.end());

  std::vector<CraqOperationResult*> getResults2;
  std::vector<CraqOperationResult*> trackedSetResults2;

  craqDhtSet.tick(getResults2,trackedSetResults2);

  allGetResults.insert(allGetResults.end(), getResults2.begin(), getResults2.end());
  allTrackedResults.insert(allTrackedResults.end(),trackedSetResults2.begin(), trackedSetResults2.end());
}

  /*
    Fill the messageQueueToPushTo deque by pushing outgoing messages from destServers and osegMessages to it
    Then clear osegMessages and destServers.

  */
  void CraqObjectSegmentation::poll()
  {
    mServiceStage->started();

    std::map<UUID,ServerID> updated;
    ++numServices;

    //    checkNotFoundData(); //determines what to do with all the lookups that returned that the object wasn't found
    checkReSends(); //tries to re-send all the messages that failed

    std::vector<CraqOperationResult*> getResults;
    std::vector<CraqOperationResult*> trackedSetResults;

    iteratedWait(5, getResults,trackedSetResults);

    int numGetResults = (int) getResults.size();
    int numSetResults = (int) trackedSetResults.size();

    numLookingUpDebug = numLookingUpDebug - ((int)getResults.size());

    if (mFinishedMoveOrLookup.size() !=0)
      updated.swap(mFinishedMoveOrLookup);


    //run through all the get results first.
    for (unsigned int s=0; s < getResults.size(); ++s)
    {
      if (getResults[s]->servID == NullServerID)
      {
        notFoundFunction(getResults[s]);
        delete getResults[s];

        std::cout<<"\n\nWe have an object that does not exist in craq system.  This shouldn't have really been called.\n\n";
        assert(false);
        continue;
      }

      updated[mapDataKeyToUUID[getResults[s]->idToString()]]  = getResults[s]->servID;

      UUID tmper = mapDataKeyToUUID[getResults[s]->idToString()];

      inTransOrLookup_m.lock();
      std::map<UUID,TransLookup>::iterator iter = mInTransitOrLookup.find(tmper);

      //put the value in the cache
      //      mCraqCache.insert(tmper, getResults[s]->servID);
      postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, tmper, getResults[s]->servID));

      
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
        //by short-circuited we mean that we did a set on this object and maybe received set ack before we got a get response
#ifdef CRAQ_DEBUG
        std::cout<<"\n\nbftm debug:  getting results for objects that we were not looking for.  Or object lookups that have been short-circuited by a trackedSet for obj id:  "  << getResults[s]->idToString()<<"\n\n";
#endif
      }
      inTransOrLookup_m.unlock();
      delete getResults[s];
    }

    processCraqTrackedSetResults(trackedSetResults, updated);

    if( mFinishedMoveOrLookup.size() != 0)
      mFinishedMoveOrLookup.clear();


    if (mListener)
    {
      for(std::map<UUID,ServerID>::iterator it = updated.begin(); it != updated.end(); it++)
        postingStrand->post(boost::bind(&CraqObjectSegmentation::callOsegLookupCompleted, this, it->first, it->second));
    }

    mServiceStage->finished();
  }

//should be called from inside of mainStrand->post.
void CraqObjectSegmentation::callOsegLookupCompleted(const UUID& obj_id, const ServerID& sID)
{
  mListener->osegLookupCompleted( obj_id,sID);
}

  void CraqObjectSegmentation::convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const
  {
    returner[0] = myUniquePrefixKey;
    strncpy(returner+1,obj_id.rawHexData().c_str(),obj_id.rawHexData().size() + 1);
  }


  //This receives messages oseg migrate and acknowledge messages
  void CraqObjectSegmentation::receiveMessage(Message* msg)
  {
      if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_MOVE) {
          CBR::Protocol::OSeg::MigrateMessageMove oseg_move_msg;
          bool parsed = parsePBJMessage(&oseg_move_msg, msg->payload());
          if (parsed)
              processMigrateMessageMove(oseg_move_msg);
      }
      else if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE) {
          CBR::Protocol::OSeg::MigrateMessageAcknowledge oseg_ack_msg;
          bool parsed = parsePBJMessage(&oseg_ack_msg, msg->payload());
          if (parsed)
              processMigrateMessageAcknowledge(oseg_ack_msg);
      }
      else if (msg->dest_port() == SERVER_PORT_OSEG_UPDATE) {
          CBR::Protocol::OSeg::UpdateOSegMessage update_oseg_msg;
          bool parsed = parsePBJMessage(&update_oseg_msg, msg->payload());
          if (parsed)
              processUpdateOSegMessage(update_oseg_msg);
      }
      else {
          printf("\n\nReceived the wrong type of message in receiveMessage of craqobjectsegmentation.cpp.\n\n ");
          delete msg;
      }

    delete msg; //delete message here.
  }



void CraqObjectSegmentation::processUpdateOSegMessage(const CBR::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg)
{
#ifdef CRAQ_DEBUG
  std::cout<<"\n\n got a processUpdateOSegMessage time received  "<< mContext->time.raw()<<"\n";
#endif


  //  mCraqCache.insert(update_oseg_msg.m_objid(), update_oseg_msg.servid_obj_on());
  postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, update_oseg_msg.m_objid(), update_oseg_msg.servid_obj_on()));

}


  void CraqObjectSegmentation::processMigrateMessageMove(const CBR::Protocol::OSeg::MigrateMessageMove& msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;

    serv_from = msg.m_servid_from();
    serv_to   = msg.m_servid_to();
    obj_id    = msg.m_objid();

    printf("\n\nGot a message that I should not have\n\n");
    assert(false);
  }


  void CraqObjectSegmentation::processMigrateMessageAcknowledge(const CBR::Protocol::OSeg::MigrateMessageAcknowledge& msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;

    serv_from = msg.m_servid_from();
    serv_to   = msg.m_servid_to();
    obj_id    = msg.m_objid();


    std::map<UUID,TransLookup>::iterator inTransIt;


    //    mCraqCache.insert(obj_id, serv_from); //note: make sure that this is the right insert order.
    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, obj_id, serv_from));
    
    inTransOrLookup_m.lock();
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
    inTransOrLookup_m.unlock();
    
    //send a message to the server that object should now disconnect
    // FIXME is this really supposed to go to the same server? If so, there *must* be a better way to accomplish this
    CBR::Protocol::ObjConnKill::ObjConnKill kill_msg_contents;
    kill_msg_contents.set_m_objid(obj_id);
    Message* kill_msg = new Message(
        mContext->id(),
        SERVER_PORT_KILL_OBJ_CONN,
        mContext->id(),
        SERVER_PORT_KILL_OBJ_CONN,
        serializePBJMessage(kill_msg_contents)
    );
    bool sent = mContext->router()->route(MessageRouter::MIGRATES, kill_msg);//this will route the message to the server so that this computer can disconnect from the object connection.

    if (!sent)
    {
      //      printf("\n\nbftm debug: ERROR sending killconnection message for migrate for obj:  %s  to self\n\n",obj_id.toString().c_str());
      reTryKillConnMessage.push_back(kill_msg);
    }
  }


//This function tries to re-send all the messages that failed to be delivered
void CraqObjectSegmentation::checkReSends()
{
  Timer checkSendTimer;
  checkSendTimer.start();


    std::vector<Message*> reReTryAddedMessage;
    std::vector<Message*> reReTryMigAckMessage;
    std::vector<Message*> reReTryKillConnMessage;


    //trying to resend faild added messages
    for (int s=0;s < (int)reTryAddedMessage.size(); ++s)
    {
      bool sent = mContext->router()->route(MessageRouter::MIGRATES,reTryAddedMessage[s],false);

      if (!sent)
      {
        //        printf("\n\nbftm debug: ERROR here trying to resend add message: obj: %s", reTryAddedMessage[s]->contents.m_objid().toString().c_str());
        //        fflush(stdout);
        reReTryAddedMessage.push_back(reTryAddedMessage[s]);
      }
    }


    //trying to re-send failed mig ack messages
    for (int s=0; s< (int)reTryMigAckMessage.size(); ++s)
    {
      bool sent= mContext->router()->route(MessageRouter::MIGRATES,reTryMigAckMessage[s],false);
      if (!sent)
      {
        //        printf("\n\nbftm debug: ERROR here trying to resend migrate ack msg for object %s to %i.  Size of outstanding:  %i \n\n", reTryMigAckMessage[s]->m_objid().toString().c_str(),reTryMigAckMessage[s]->getMessageDestination(), (int)reTryMigAckMessage.size() );
        //        fflush(stdout);
        reReTryMigAckMessage.push_back(reTryMigAckMessage[s]);
      }
    }

    //trying to re-send failed kill conn messages
    for (int s=0; s <(int) reTryKillConnMessage.size(); ++s)
    {
      bool sent = mContext->router()->route(MessageRouter::MIGRATES,reTryKillConnMessage[s]);//this will route the message to the server so that this computer can disconnect from the object connection.

      if (!sent)
      {
        //        printf("\n\nbftm debug: ERROR here trying to resend killconnection message for migrate for obj:  %s  to self\n\n", reTryKillConnMessage[s]->contents.m_objid().toString().c_str());
        reTryKillConnMessage.push_back(reTryKillConnMessage[s]);
      }
    }


    //load the messages that failed from this set of operations back so that can try to resend them next time this function is called.
    reTryAddedMessage.clear();
    reReTryAddedMessage.swap(reTryAddedMessage);

    reTryMigAckMessage.clear();
    reReTryMigAckMessage.swap(reTryMigAckMessage);

    reTryKillConnMessage.clear();
    reReTryKillConnMessage.swap(reTryKillConnMessage);

}


//this function tells us what to do with all the ids that just weren't found in craq.
void CraqObjectSegmentation::notFoundFunction(CraqOperationResult* nf)
{
  //turn the id into a uuid and then push it onto the end of  mNfData queue.

#ifdef CRAQ_DEBUG
  std::cout<<"\n\nI'm pushing into notFoundFunction\n\n";
#endif

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

  std::cout<<"\n\nGOT NOT FOUND\n\n";


  Duration tmpDur = mTimer.elapsed();

  bool queueDataOldEnough = true;
  NotFoundData* nfd;

  while (queueDataOldEnough)
  {
    nfd = mNfData.front();
    queueDataOldEnough = false;

    if ((tmpDur.toMilliseconds() - nfd->dur.toMilliseconds()) > CRAQ_NOT_FOUND_SIT_OUT) //we wait 500 ms.
    {
      queueDataOldEnough = true;

      //perform a direct craq get call
      std::string indexer = "";
      indexer.append(1,myUniquePrefixKey);
      indexer.append(nfd->obj_id.rawHexData());

      CraqDataSetGet cdSetGet (indexer,0,false,CraqDataSetGet::GET); //bftm modified
      craqDhtGet.get(cdSetGet); //calling the craqDht to do a get.

      mNfData.pop(); //remove the item from the queue.

#ifdef CRAQ_DEBUG
      std::cout<<"\n\nbftm debug.  I'm releasing an object that wasn't found with obj_id  "<<nfd->obj_id.toString()<<"\n";
#endif
      delete nfd; //memory managment
    }

    queueDataOldEnough = queueDataOldEnough && (mNfData.size() > 0);  //don't keep going if you're out of objects.
  }
}


}//namespace CBR
