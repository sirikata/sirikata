/*  Sirikata
 *  CraqObjectSegmentation.cpp
 *
 *  Copyright (c) 2010, Behram Mistree
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */




#include <sirikata/space/ObjectSegmentation.hpp>
#include <sirikata/space/ServerMessage.hpp>
#include <map>
#include <vector>
#include <sirikata/core/trace/Trace.hpp>
#include "CraqObjectSegmentation.hpp"
#include "craq_oseg/asyncCraq.hpp"
#include "craq_oseg/asyncUtil.hpp"
#include "craq_oseg/asyncConnection.hpp"

#include <sirikata/space/CoordinateSegmentation.hpp>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <boost/thread/mutex.hpp>

#include <sirikata/core/network/IOStrandImpl.hpp>

namespace Sirikata
{

  /*
    Basic constructor
  */
CraqObjectSegmentation::CraqObjectSegmentation (SpaceContext* con, Network::IOStrand* o_strand, CoordinateSegmentation* cseg, OSegCache* cache, char unique)
   : ObjectSegmentation(con, o_strand),
     mCSeg (cseg),
     craqDhtGet1(con, o_strand, this),
     craqDhtGet2(con, o_strand, this),
     craqDhtSet(con, o_strand, this),
     postingStrand(con->mainStrand),
     mStrand(o_strand),
     mMigAckMessages( con->mainStrand->wrap(std::tr1::bind(&CraqObjectSegmentation::handleNewMigAckMessages, this)) ),
     mCraqCache(cache),
     mFrontMigAck(NULL),
     ctx(con),
     mReceivedStopRequest(false),
     mAtomicTrackID(10),
     mOSegQueueLen(0)
  {

      std::vector<CraqInitializeArgs> getInitArgs;
      CraqInitializeArgs cInitArgs1;

      cInitArgs1.ipAdd = "localhost";
      cInitArgs1.port  =     "10498"; //craq version 2
      getInitArgs.push_back(cInitArgs1);

      std::vector<CraqInitializeArgs> setInitArgs;
      CraqInitializeArgs cInitArgs2;
      cInitArgs2.ipAdd = "localhost";
      cInitArgs2.port  =     "10499";
      setInitArgs.push_back(cInitArgs2);

    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    // Register a ServerMessage service for oseg
    mOSegServerMessageService = mContext->serverRouter()->createServerMessageService("craq-oseg");

    craqDhtGet1.initialize(getInitArgs);
    craqDhtGet2.initialize(getInitArgs);
    craqDhtSet.initialize(setInitArgs);

    myUniquePrefixKey = unique;

    numCacheHits     = 0;
    numOnThisServer  = 0;
    numLookups       = 0;
    numCraqLookups   = 0;
    numTimeElapsedCacheEviction = 0;
    numMigrationNotCompleteYet  = 0;
    numAlreadyLookingUp         = 0;
    numServices                 = 0;
    numLookingUpDebug           = 0;

    checkOwnTimeDur   = 0;
    checkOwnTimeCount = 0;

//    mAtomicTrackID    = 10;

  }


  void CraqObjectSegmentation::stop()
  {
    craqDhtSet.stop();
    craqDhtGet1.stop();
    craqDhtGet2.stop();
    
    mReceivedStopRequest = true;
  }

  /*
    Destructor
  */
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    delete mOSegServerMessageService;

    //    should delete not found queue;
    while (mNfData.size() != 0)
    {
      NotFoundData* nfd = mNfData.front();
      mNfData.pop();
      delete nfd;
    }


    for (TrackedMessageMapAdded::iterator tmessmapit  = trackedAddMessages.begin(); tmessmapit != trackedAddMessages.end(); ++tmessmapit)
      delete tmessmapit->second.msgAdded;

    trackedAddMessages.clear();


    //delete retries
    while( !mMigAckMessages.empty() ) {
        Message* msg = NULL;
        mMigAckMessages.pop(msg);
        delete msg;
    }




    CONTEXT_SPACETRACE(processOSegShutdownEvents,
        mContext->id(),
        numLookups,
        numOnThisServer,
        numCacheHits,
        numCraqLookups,
        numTimeElapsedCacheEviction,
        numMigrationNotCompleteYet);

  }


  /*
    This function checks to see whether the obj_id is hosted on this space server
  */
  bool CraqObjectSegmentation::checkOwn(const UUID& obj_id, float*radius)
  {
    ObjectSet::iterator where=mObjects.find(obj_id);
    if (where == mObjects.end())
    {
      //means that the object isn't hosted on this space server
      return false;
    }
    *radius=where->second.radius();
    //means that the object *is* hosted on this space server
    //    Duration endingDur = mTimer.elapsed();

    return true;
  }


  /*
    If returns true, means that the object isn't already undergoing a migration, and is therefore clear to migrate.
  */
  bool CraqObjectSegmentation::clearToMigrate(const UUID& obj_id)
  {
    if (mReceivedStopRequest)
      return false;

    //set a mutex lock.
    inTransOrLookup_m.lock();
    InTransitMap::iterator mInTransIter = mInTransitOrLookup.find(obj_id);

    bool migratingFromHere = false;
    if (mInTransIter != mInTransitOrLookup.end())
    {
      //means that the object is either being looked up or in transit
      if (mInTransIter->second.sID.notNull())
      {
        //means that the object is migrating from here.
        migratingFromHere = true;
      }
    }
    inTransOrLookup_m.unlock();

    receivingObjects_m.lock();
    bool migratingToHere = (mReceivingObjects.find(obj_id) != mReceivingObjects.end());
    //means that the object migrating to here has not yet received an acknowledge, and therefore shouldn't begin migrating again.
    receivingObjects_m.unlock();

    //clear to migrate only if migrating from here and migrating to here are false.
    return (!migratingFromHere) && (!migratingToHere);
  }



  //this call returns true if the object is migrating from this server to another server, but hasn't yet received an ack message (which will disconnect the object connection.)
bool CraqObjectSegmentation::checkMigratingFromNotCompleteYet(const UUID& obj_id, float*radius)
  {
    if (mReceivedStopRequest)
      return false;

    inTransOrLookup_m.lock();
    InTransitMap::const_iterator iterInTransOrLookup = mInTransitOrLookup.find(obj_id);

    if (iterInTransOrLookup == mInTransitOrLookup.end())
    {
      inTransOrLookup_m.unlock();
      return false;
    }
    if (iterInTransOrLookup->second.sID.notNull())
    {
      *radius=iterInTransOrLookup->second.sID.radius();
      inTransOrLookup_m.unlock();
      return true;
    }

    inTransOrLookup_m.unlock();
    return false;
  }


  //checks value against cache.
  //should only be called from the "postingStrand"
  CraqEntry CraqObjectSegmentation::satisfiesCache(const UUID& obj_id)
  {

#ifndef CRAQ_CACHE
    return CraqEntry::null();
#endif

    return mCraqCache->get(obj_id);
  }


  int CraqObjectSegmentation::getUniqueTrackID()
  {
      int returner = mAtomicTrackID;
      ++mAtomicTrackID;
      if (returner == 0) //0 has a special meaning.
      {
          returner = mAtomicTrackID;
          ++mAtomicTrackID;
      }
      
      return returner;
  }


  void CraqObjectSegmentation::addNewObject(const UUID& obj_id, float radius)
  {
    if (mReceivedStopRequest)
      return ;

    CraqDataKey cdk;
    convert_obj_id_to_dht_key(obj_id,cdk);
    CraqDataSetGet cdSetGet(cdk, CraqEntry(mContext->id(),radius) ,true,CraqDataSetGet::SET);

    TrackedSetResultsDataAdded tsrda;
    tsrda.msgAdded  = generateAddedMessage(obj_id,radius);
    tsrda.dur = Time::local() - Time::epoch();


    int trackID = getUniqueTrackID();
    craqDhtSet.set(cdSetGet, trackID);
    trackedAddMessages[trackID] = tsrda;
  }

void CraqObjectSegmentation::removeObject(const UUID& obj_id) {
    // SILOG(craqoseg, error, "CraqObjectSegmentation::removeObject not implemented.");
}

Sirikata::Protocol::OSeg::AddedObjectMessage* CraqObjectSegmentation::generateAddedMessage(const UUID& obj_id, float radius)
  {
    Sirikata::Protocol::OSeg::AddedObjectMessage* oadd = new Sirikata::Protocol::OSeg::AddedObjectMessage();
    oadd->set_m_objid(obj_id);
    oadd->set_m_objradius(radius);
    return oadd;
  }

  OSegEntry CraqObjectSegmentation::cacheLookup(const UUID& obj_id)
  {
      // NOTE: This must be thread safe, so don't access most state.  Don't
      // bother with local/migration checks.  Just check the cache and move on.
    CraqEntry cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn.notNull()) && (cacheReturn.server() != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {
      ++numCacheHits;
      return cacheReturn;
    }

    return CraqEntry::null();
  }

bool CraqObjectSegmentation::shouldLog()
{
    static uint64 counter = 0;

    ++counter;
    return ((counter % 51) ==0);
}

int CraqObjectSegmentation::getPushback()
{
    return mOSegQueueLen;
}

  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
    Only called from postingStrand
  */
  OSegEntry CraqObjectSegmentation::lookup(const UUID& obj_id)
  {

    if (mReceivedStopRequest)
      return CraqEntry::null();


    OSegLookupTraceToken* traceToken = new OSegLookupTraceToken(obj_id,shouldLog());
    traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_INITIAL_LOOKUP_TIME);


    ++numLookups;
    float radius=0;
    if (checkOwn(obj_id,&radius))  //this call just checks through to see whether the object is on this space server.
    {
      ++numOnThisServer;
      delete traceToken;
      return CraqEntry(mContext->id(),radius);
    }

    //log the request.
    CONTEXT_SPACETRACE(objectSegmentationLookupNotOnServerRequest,
        obj_id,
        mContext->id());


    if (checkMigratingFromNotCompleteYet(obj_id,&radius))//this call just checks to see whether the object is migrating from this server to another server.  If it is, but hasn't yet received an ack message to disconnect the object connection.
    {
      ++numMigrationNotCompleteYet;
      delete traceToken;
      return CraqEntry(mContext->id(),radius);
    }


    traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CHECK_CACHE_LOCAL_BEGIN);
    
    CraqEntry cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn.notNull()) && (cacheReturn.server() != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {
        CONTEXT_SPACETRACE(osegCacheResponse,
            cacheReturn.server(),
            obj_id);

      ++numCacheHits;


      traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CHECK_CACHE_LOCAL_END);

      delete traceToken;
      return cacheReturn;
    }


    traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CHECK_CACHE_LOCAL_END);

    ++mOSegQueueLen;
    traceToken->osegQLenPostQuery = mOSegQueueLen;
    oStrand->post(boost::bind(&CraqObjectSegmentation::beginCraqLookup,this,obj_id, traceToken));

    return CraqEntry::null();
  }




  void CraqObjectSegmentation::beginCraqLookup(const UUID& obj_id, OSegLookupTraceToken* traceToken)
  {
      --mOSegQueueLen;
      if (mReceivedStopRequest)
      {
          delete traceToken;
          return;
      }

    traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CRAQ_LOOKUP_BEGIN);

    UUID tmper = obj_id;
    InTransitMap::const_iterator iter = mInTransitOrLookup.find(tmper);

    if (iter == mInTransitOrLookup.end()) //means that the object isn't already being looked up and the object isn't already in transit
    {
        //Duration beginCraqLookupNotAlreadyLookingUpDur = Time::local() - Time::epoch();
        //traceToken->craqLookupNotAlreadyLookingUpBegin  = beginCraqLookupNotAlreadyLookingUpDur.toMicroseconds();
        traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CRAQ_LOOKUP_NOT_ALREADY_LOOKING_UP_BEGIN);
      
      //if object is not in transit, lookup its location in the dht.  returns -1 if object doesn't exist.
      //add the mapping of a craqData Key to a uuid.

      ++numCraqLookups;

      std::string indexer = "";
      indexer.append(1,myUniquePrefixKey);
      indexer.append(tmper.rawHexData());

      CraqDataSetGet cdSetGet (indexer,CraqEntry::null(),false,CraqDataSetGet::GET); //bftm modified

      mapDataKeyToUUID[indexer] = tmper; //changed here.


      CONTEXT_SPACETRACE(objectSegmentationCraqLookupRequest,
          obj_id,
          mContext->id());

      ++numLookingUpDebug;

      //puts object in transit or lookup.
      //Duration timerDur =  Time::local() - Time::epoch();

      TransLookup tmpTransLookup;
      tmpTransLookup.sID = CraqEntry::null();  //means that we're performing a lookup, rather than a migrate.
      //tmpTransLookup.timeAdmitted = (int)timerDur.toMilliseconds();
      tmpTransLookup.timeAdmitted = 0;

      mInTransitOrLookup[tmper] = tmpTransLookup; //just says that we are performing a lookup on the object

      traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CRAQ_LOOKUP_END);
      traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CRAQ_LOOKUP_NOT_ALREADY_LOOKING_UP_END);

      if ((numCraqLookups %2) == 0)
          craqDhtGet1.get(cdSetGet,traceToken); //calling the craqDht to do a get
      else
          craqDhtGet2.get(cdSetGet,traceToken); //calling the craqDht to do a get.
    }
    else
    {
      ++numAlreadyLookingUp;
      traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_CRAQ_LOOKUP_END);
      CONTEXT_SPACETRACE(osegCumulativeResponse, traceToken);
      delete traceToken;
    }
  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  Sirikata::Protocol::OSeg::MigrateMessageAcknowledge* CraqObjectSegmentation::generateAcknowledgeMessage(const UUID &obj_id,float radius, ServerID sID_to)
  {
    Sirikata::Protocol::OSeg::MigrateMessageAcknowledge* oseg_ack_msg = new Sirikata::Protocol::OSeg::MigrateMessageAcknowledge();
    oseg_ack_msg->set_m_servid_from(mContext->id());
    oseg_ack_msg->set_m_servid_to(sID_to);
    oseg_ack_msg->set_m_message_destination(sID_to);
    oseg_ack_msg->set_m_message_from(mContext->id());
    oseg_ack_msg->set_m_objid(obj_id);
    oseg_ack_msg->set_m_objradius(radius);
    return oseg_ack_msg;
  }


  /*
    This is the function that we use to add an object.  generateAck is true if you want to generate an ack message to server idServerAckTo

    If you're initially adding an object to the world, you should use the newObjectAdd function instead.
  */
void CraqObjectSegmentation::addMigratedObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool generateAck)
  {
    if (mReceivedStopRequest)
      return;


    if (generateAck)
    {
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, CraqEntry(mContext->id(),radius) ,true,CraqDataSetGet::SET);

      receivingObjects_m.lock();
      mReceivingObjects.insert(ObjectSet::value_type(obj_id,CraqEntry(mContext->id(),radius)));
      receivingObjects_m.unlock();

      TrackedSetResultsData tsrd;
      tsrd.migAckMsg = generateAcknowledgeMessage(obj_id, radius, idServerAckTo);
      tsrd.dur =  Time::local() - Time::epoch();

      int trackID = getUniqueTrackID();
      craqDhtSet.set(cdSetGet,trackID);
      trackingMessages[trackID] = tsrd;
    }
    else
    {
      //potentially clean up: we should really only be adding objects without generating acks using the newObjectAdd interface
      CraqDataKey cdk;
      convert_obj_id_to_dht_key(obj_id,cdk);

      CraqDataSetGet cdSetGet(cdk, CraqEntry(mContext->id(),radius) ,false,CraqDataSetGet::SET);
      int trackID = getUniqueTrackID();
      craqDhtSet.set(cdSetGet, trackID);


      std::pair<ObjectSet::iterator, bool> inserted = mObjects.insert(ObjectSet::value_type(obj_id,CraqEntry(mContext->id(),radius)));
      if (inserted.second)
      {
        std::cout<<"\n\nAdding object:  "<<obj_id.toString()<<"\n";
      }
    }
  }


  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
    Whatever calls this must verify that the object is on this server.
    I can do the check in the function by querrying bambooDht as well
  */
  void CraqObjectSegmentation::migrateObject(const UUID& obj_id, const OSegEntry& new_server_id)
  {
    if (mReceivedStopRequest)
      return;

    //log the message.
    CONTEXT_SPACETRACE(objectBeginMigrate,
        obj_id,mContext->id(),
        new_server_id.server());

    InTransitMap::const_iterator transIter = mInTransitOrLookup.find(obj_id);

    TransLookup tmpTransLookup;
    tmpTransLookup.sID = CraqEntry(new_server_id);

    Duration tmpDurer = Time::local() - Time::epoch();

    tmpTransLookup.timeAdmitted = (int)tmpDurer.toMilliseconds();

    mInTransitOrLookup[obj_id] = tmpTransLookup;

    //erases the local copy of obj_id
    UUID tmp_id = obj_id; //note: can probably delete this line

    //erase the local copy of the object.
    size_t num_erased = mObjects.erase(obj_id);
#ifdef CRAQ_DEBUG
    if (num_erased == 0)
    {
        std::cout<<"\n\nThe object clearly wasn't registered on this server.  This is obj id:  " <<  obj_id.toString() <<  ".  This is time:   " <<mContext->simTime().raw() << " Oh no.   ";

      if (clearToMigrate(obj_id))
        std::cout<<"  Likely a problem with clear to migrate\n\n";
      else
        std::cout<<"\n\n clear to migrate is fine migration is being called from somewhere besides server.cpp\n\n";
    }
#endif
  }


  //should be called from inside of o_strand and posts to postingStrand mainStrand->post.
  void CraqObjectSegmentation::callOsegLookupCompleted(const UUID& obj_id, const CraqEntry& sID, OSegLookupTraceToken* traceToken)
  {
    if (mReceivedStopRequest)
    {
      if (traceToken != NULL)
        delete traceToken;
      return;
    }
    
    mLookupListener->osegLookupCompleted(obj_id,sID);
    
    if (traceToken != NULL) {
        traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_LOOKUP_RETURN_END);
        CONTEXT_SPACETRACE(osegCumulativeResponse, traceToken);
        delete traceToken;
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
    if (mReceivedStopRequest)
    {
      delete msg;
      return;
    }

      if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_MOVE) {
          Sirikata::Protocol::OSeg::MigrateMessageMove oseg_move_msg;
          bool parsed = parsePBJMessage(&oseg_move_msg, msg->payload());
          if (parsed)
          {
            printf("\n\nMigrate message move in oseg is a deprecated message.  Killing\n\n");
            assert(false);
          }
      }
      else if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE) {
          Sirikata::Protocol::OSeg::MigrateMessageAcknowledge oseg_ack_msg;
          bool parsed = parsePBJMessage(&oseg_ack_msg, msg->payload());
          if (parsed)
            mStrand->post(std::tr1::bind(&CraqObjectSegmentation::processMigrateMessageAcknowledge, this,oseg_ack_msg));
      }
      else if (msg->dest_port() == SERVER_PORT_OSEG_UPDATE) {
          Sirikata::Protocol::OSeg::UpdateOSegMessage update_oseg_msg;
          bool parsed = parsePBJMessage(&update_oseg_msg, msg->payload());
          if (parsed)
            mStrand->post(std::tr1::bind(&CraqObjectSegmentation::processUpdateOSegMessage,this,update_oseg_msg));
      }
      else {
          printf("\n\nReceived the wrong type of message in receiveMessage of craqobjectsegmentation.cpp.\n\n ");
      }

      delete msg;
  }



  //called from within o_strand
  void CraqObjectSegmentation::processUpdateOSegMessage(const Sirikata::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg)
  {
    if (mReceivedStopRequest)
      return;

    mCraqCache->insert(update_oseg_msg.m_objid(), CraqEntry(update_oseg_msg.servid_obj_on(),update_oseg_msg.m_objradius()));
  }

  //called from within o_strand
  void CraqObjectSegmentation::processMigrateMessageAcknowledge(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg)
  {
    if (mReceivedStopRequest)
      return;

    CraqEntry serv_from(msg.m_servid_from(),msg.m_objradius()), serv_to(msg.m_servid_to(),msg.m_objradius());
    UUID obj_id;

    obj_id    = msg.m_objid();


    InTransitMap::iterator inTransIt;

    mCraqCache->insert(obj_id, serv_from);

    inTransOrLookup_m.lock();
    inTransIt = mInTransitOrLookup.find(obj_id);
    if (inTransIt != mInTransitOrLookup.end())
    {
      //means that we now remove the obj_id from mInTransit
      mInTransitOrLookup.erase(inTransIt);
      //add it to mFinishedMove.  serv_from

      //put the value in the cache!
      mCraqCache->insert(obj_id, serv_from);
      callOsegLookupCompleted(obj_id,serv_from, NULL);


      //log reception of acknowled message
      CONTEXT_SPACETRACE(objectAcknowledgeMigrate,
                    obj_id,serv_from.server(),
                    mContext->id());
    }
    inTransOrLookup_m.unlock();

    //send a message to the server that object should now disconnect
    mWriteListener->osegMigrationAcknowledged(obj_id);
  }

void CraqObjectSegmentation::handleNewMigAckMessages() {
    trySendMigAcks();
}

void CraqObjectSegmentation::trySendMigAcks() {
    if (mReceivedStopRequest)
      return;

    while(true) {
        if (mFrontMigAck == NULL) {
            if (mMigAckMessages.empty())
                return;
            mMigAckMessages.pop(mFrontMigAck);
        }

        bool sent = mOSegServerMessageService->route( mFrontMigAck );
        if (!sent)
            break;

        mFrontMigAck = NULL;
    }

    if (mFrontMigAck != NULL || !mMigAckMessages.empty()) {
        // We've still got work to do, setup a retry
        mContext->mainStrand->post(
            Duration::microseconds(100),
            std::tr1::bind(&CraqObjectSegmentation::trySendMigAcks, this)
                                   );
    }
}

  //this function tells us what to do with all the ids that just weren't found in craq.
  void CraqObjectSegmentation::notFoundFunction(CraqOperationResult* nf)
  {
    if (mReceivedStopRequest)
      return;

    //turn the id into a uuid and then push it onto the end of  mNfData queue.
    if (mapDataKeyToUUID.find(nf->idToString()) == mapDataKeyToUUID.end())
      return; //means that we really have no record of this objects' even having been requested.

    NotFoundData* nfd = new NotFoundData();
    nfd->obj_id =  mapDataKeyToUUID[nf->idToString()];
    nfd->dur = Time::local() - Time::epoch();
    nfd->traceToken = nf->traceToken;


    mNfData.push(nfd);
  }


  //this function tells us if there are any ids that have waited long enough for it to be safe to query for the again.
  void CraqObjectSegmentation::checkNotFoundData()
  {
    if (mReceivedStopRequest)
      return;

    //look at the first element of the queue.  if the time hasn't been sufficiently long, return.  if the time has been sufficiently long, then go ahead and directly request asyncCraq to perform another lookup.

    if (mNfData.size() == 0)
      return;

    std::cout<<"\n\nGOT NOT FOUND\n\n";

    Duration tmpDur = Time::local() - Time::epoch();


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

        CraqDataSetGet cdSetGet (indexer,CraqEntry(NullServerID,0),false,CraqDataSetGet::GET); //bftm modified
        craqDhtGet1.get(cdSetGet, nfd->traceToken); //calling the craqDht to do a get.

        mNfData.pop(); //remove the item from the queue.

        delete nfd; //memory managment
      }
      queueDataOldEnough = queueDataOldEnough && (mNfData.size() > 0);  //don't keep going if you're out of objects.
    }
  }

  //gets posted to from asyncCraqGet.  Should get inside of o_strand from the post.
  void CraqObjectSegmentation::craqGetResult(CraqOperationResult* cor)
  {
      --mOSegQueueLen;
      
    if (cor->traceToken != NULL)
        cor->traceToken->stamp(OSegLookupTraceToken::OSEG_TRACE_LOOKUP_RETURN_BEGIN);
        //cor->traceToken->lookupReturnBegin = tmpDur.toMicroseconds();

    if (mReceivedStopRequest)
    {
      if (cor->traceToken != NULL)
        delete cor->traceToken;
      delete cor;
      return;
    }

    if (cor->servID.isNull())
    {
      notFoundFunction(cor);

      if(cor->traceToken!= NULL)
        delete cor->traceToken;

      delete cor;

      std::cout<<"\n\nWe have an object that does not exist in craq system.  This shouldn't have really been called.\n\n";

      std::cout<<"Object:  "<<cor->objID<<"  server id:  "<<cor->servID.server()<<"  radius  "<<cor->servID.radius()<<"  tracking  "<<cor->tracking<<"  suceeded: "<<cor->succeeded<<"  GET or set:   "<<cor->whichOperation<<"\n\n";

      assert(false);
      return;
    }

    UUID tmper = mapDataKeyToUUID[cor->idToString()];

    //put the value in the cache!
    mCraqCache->insert(tmper, cor->servID);

    inTransOrLookup_m.lock();
    InTransitMap::iterator iter = mInTransitOrLookup.find(tmper);

    if (iter != mInTransitOrLookup.end()) //means that the object was already being looked up or in transit
    {
      if(iter->second.sID.isNull())
      {
        //means that after receiving a lookup request, we did not intermediarilly receive a migrate request.
        //if above predicate is not true, means that we did receive a migrate request, and therefore should not delete that migrate request from mInTransitOrLookup (ie do nothing).
        mInTransitOrLookup.erase(iter);
      }
    }
    inTransOrLookup_m.unlock();



    callOsegLookupCompleted(tmper,cor->servID, cor->traceToken);
    delete cor;
  }


  //locks intransor lookup and removes the
  //which we already know has an entry in trackingMessages
  void CraqObjectSegmentation::removeFromInTransOrLookup(const UUID& obj_id)
  {
    if (mReceivedStopRequest)
      return;

    inTransOrLookup_m.lock();

    //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
    InTransitMap::iterator inTransLookIter = mInTransitOrLookup.find(obj_id);

    if (inTransLookIter != mInTransitOrLookup.end())
    {
      //means that object can now be removed from mInTransitOrLookup
      mInTransitOrLookup.erase(inTransLookIter);
    }

    //finished deleting from mInTransitOrLookup
    inTransOrLookup_m.unlock();
  }


  //Assumes that the result as an entry in trackingMessages
  void CraqObjectSegmentation::removeFromReceivingObjects(const UUID& obj_id)
  {
    if (mReceivedStopRequest)
      return;

    //remove this object from mReceivingObjects,
    //this removal indicates that this object can now be safely migrated from this server to another if need be.
    receivingObjects_m.lock();
    mReceivingObjects.erase(obj_id);
    //done removing from receivingObjects.
    receivingObjects_m.unlock();
  }


  //gets posted to from asyncCraqSet.  Should get inside of o_strand from the post.
  void CraqObjectSegmentation::craqSetResult(CraqOperationResult* trackedSetResult)
  {
    if (mReceivedStopRequest)
    {
      delete trackedSetResult;
      return;
    }
    if(trackedSetResult->trackedMessage == 0)  //means that we weren't supposed to be tracking this message
    {
      delete trackedSetResult;
      return;
    }
    TrackedMessageMapAdded::iterator awhere;
    TrackedMessageMap::iterator where=trackingMessages.find(trackedSetResult->trackedMessage);
    if (where != trackingMessages.end())
    {
      //means that we have a record of locally tracking this message and that we'll have to send an ack.

      UUID obj_id = where->second.migAckMsg->m_objid();
      float radius=where->second.migAckMsg->m_objradius();

      //add this to the cache
      mCraqCache->insert(obj_id, CraqEntry(mContext->id(),radius));

      //add tomObjects the uuid associated with trackedMessage (ie, now we know that we own the object.)
      mObjects.insert(ObjectSet::value_type(obj_id,CraqEntry(mContext->id(),radius)));

      //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
      removeFromInTransOrLookup(obj_id);

      //remove this object from mReceivingObjects,
      removeFromReceivingObjects(obj_id);

      //send an acknowledge message to space server that formerly hosted object.
      Message* to_send = new Message(
                                     mContext->id(),
                                     SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
                                     trackingMessages[trackedSetResult->trackedMessage].migAckMsg->m_message_destination(),
                                     SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
                                     serializePBJMessage( *(trackingMessages[trackedSetResult->trackedMessage].migAckMsg) )
                                     );
      mMigAckMessages.push(to_send); // sending handled in main strand

      trackingMessages.erase(trackedSetResult->trackedMessage);//stop tracking this message.
    }
    else if ((awhere=trackedAddMessages.find(trackedSetResult->trackedMessage)) != trackedAddMessages.end())
    {

      //means that we just finished adding first object
        mObjects.insert( ObjectSet::value_type(awhere->second.msgAdded->m_objid(),
                                               CraqEntry(mContext->id(),
                                                         awhere->second.msgAdded->m_objradius())));//need to add obj_id

      UUID written_obj = awhere->second.msgAdded->m_objid();
      mWriteListener->osegWriteFinished(written_obj);
    }

    delete trackedSetResult;
    return;
  }


}//namespace Sirikata
