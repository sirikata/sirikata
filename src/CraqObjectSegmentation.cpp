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
#include "OSegLookupTraceToken.hpp"
#include "Utility.hpp"

#include <sirikata/network/IOStrandImpl.hpp>
#include <sirikata/util/AtomicTypes.hpp>

namespace CBR
{
  //craq atomic int
  int CraqAtomicInt::getUniqueTrackID()
  {
    int returner = mAtom;
    ++mAtom;
    return returner;
  }
  
  
  //Basic constructor
  CraqObjectSegmentation::CraqObjectSegmentation (SpaceContext* con, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> getInitArgs, std::vector<CraqInitializeArgs> setInitArgs, char prefixID, IOStrand* o_strand, IOStrand* strand_to_post_to)
 : ObjectSegmentation(con, o_strand),
   postingStrand(strand_to_post_to),
   mStrand(o_strand),
   mStrandQLen(0),
   ctx(con),
   mReceivedStopRequest(false),
   myUniquePrefixKey(prefixID),
   mCSeg (cseg),
   craqDhtGet(con, o_strand, this),
   craqDhtSet(con, o_strand, this),
   postingStrand(strand_to_post_to),
   mStrand(o_strand),
   mCraqCache(con),
   mMigAckMessages( con->mainStrand->wrap(std::tr1::bind(&CraqObjectSegmentation::handleNewMigAckMessages, this)) ),
   mFrontMigAck(NULL)
  {
    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    //Register a ServerMessage service for oseg
    mOSegServerMessageService = mContext->serverRouter()->createServerMessageService("craq-oseg");

    craqDhtGet.initialize(getInitArgs);
    craqDhtSet.initialize(setInitArgs);
  }


  void CraqObjectSegmentation::stop()
  {
    craqDhtSet.stop();
    craqDhtGet.stop();

    mReceivedStopRequest = true;
  }


  //Destructor
  CraqObjectSegmentation::~CraqObjectSegmentation()
  {
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_MOVE,this);
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,this);
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    delete mOSegServerMessageService;


    for (TrackedMessageMapAdded::iterator tmessmapit  = trackedAddMessages.begin(); tmessmapit != trackedAddMessages.end(); ++tmessmapit)
      delete tmessmapit->second.msgAdded;

    trackedAddMessages.clear();


    //delete retries
    while( !mMigAckMessages.empty() )
    {
        Message* msg;
        mMigAckMessages.pop(msg);
        delete msg;
    }

  }

  /*
    This function checks to see whether the obj_id is hosted on this space server
  */
  bool CraqObjectSegmentation::checkOwn(const UUID& obj_id, float*radius)
  {
    Duration beginningDur = Time::local() - Time::epoch();
    ObjectSet::iterator where=mObjects.find(obj_id);
    if (where == mObjects.end())
    {
      //means that the object isn't hosted on this space server
      return false;
    }
    *radius=where->second.radius();
    //means that the object *is* hosted on this space server
    return true;
  }


  //If returns true, means that the object isn't already undergoing a migration,
  //and is therefore clear to migrate.
  bool CraqObjectSegmentation::clearToMigrate(const UUID& obj_id)
  {
    if (mReceivedStopRequest)
      return false;

    //set a mutex lock.
    inTransOrLookup_m.lock();
    std::map<UUID,TransLookup>::iterator mInTransIter = mInTransitOrLookup.find(obj_id);

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
    std::map<UUID,TransLookup>::const_iterator iterInTransOrLookup = mInTransitOrLookup.find(obj_id);

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

    return mCraqCache.get(obj_id);
  }


  int CraqObjectSegmentation::getUniqueTrackID()
  {
    atomic_track_id_m.lock();
      int returner = mAtomicTrackID;
      ++mAtomicTrackID;
    atomic_track_id_m.unlock();

    return returner;
  }


  void CraqObjectSegmentation::newObjectAdd(const UUID& obj_id, float radius)
  {
    if (mReceivedStopRequest)
      return ;

    CraqDataKey cdk;
    convert_obj_id_to_dht_key(obj_id,cdk);

    CraqDataSetGet cdSetGet(cdk, CraqEntry(mContext->id(),radius) ,true,CraqDataSetGet::SET);

    TrackedSetResultsDataAdded tsrda;
    tsrda.msgAdded  = generateAddedMessage(obj_id,radius);
    tsrda.dur = Time::local() - Time::epoch();

    int trackID = mUniqueID.getUniqueTrackID();
    craqDhtSet.set(cdSetGet, trackID);
    trackedAddMessages[trackID] = tsrda;
  }


CBR::Protocol::OSeg::AddedObjectMessage* CraqObjectSegmentation::generateAddedMessage(const UUID& obj_id, float radius)
  {
    CBR::Protocol::OSeg::AddedObjectMessage* oadd = new CBR::Protocol::OSeg::AddedObjectMessage();
    oadd->set_m_objid(obj_id);
    oadd->set_m_objradius(radius);
    return oadd;
  }

  CraqEntry CraqObjectSegmentation::cacheLookup(const UUID& obj_id)
  {
      // NOTE: This must be thread safe, so don't access most state.  Don't
      // bother with local/migration checks.  Just check the cache and move on.
    CraqEntry cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn.notNull()) && (cacheReturn.server() != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {
      return cacheReturn;
    }
    return CraqEntry::null();
  }



  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
    Only called from postingStrand
  */
  CraqEntry CraqObjectSegmentation::lookup(const UUID& obj_id)
  {
    Duration beginDur = Time::local() - Time::epoch();

    if (mReceivedStopRequest)
      return CraqEntry::null();


    OSegLookupTraceToken* traceToken =   new OSegLookupTraceToken(obj_id);
    traceToken->initialLookupTime    =       (uint64)   beginDur.toMicroseconds();


    ++numLookups;
    float radius=0;
    if (checkOwn(obj_id,&radius))  //this call just checks through to see whether the object is on this space server.
    {
      delete traceToken;
      return CraqEntry(mContext->id(),radius);
    }


    //log the request.
    CONTEXT_TRACE(objectSegmentationLookupNotOnServerRequest,
        obj_id,
        mContext->id());

    if (checkMigratingFromNotCompleteYet(obj_id,&radius))//this call just checks to see whether the object is migrating from this server to another server.  If it is, but hasn't yet received an ack message to disconnect the object connection.
    {
      delete traceToken;
      return CraqEntry(mContext->id(),radius);
    }

    Duration cacheBeginDur = Time::local() - Time::epoch();
    traceToken->checkCacheLocalBegin = (uint64)cacheBeginDur.toMicroseconds();

    CraqEntry cacheReturn = satisfiesCache(obj_id);
    if ((cacheReturn.notNull()) && (cacheReturn.server() != mContext->id())) //have to perform second check to prevent accidentally infinitely re-routing to this server when the object doesn't reside here: if the object resided here, then one of the first two conditions would have triggered.
    {

        CONTEXT_TRACE(osegCacheResponse,
            cacheReturn.server(),
            obj_id);


      Duration cacheEndDur           =    Time::local() - Time::epoch();
      traceToken->checkCacheLocalEnd =    (uint64) cacheEndDur.toMicroseconds();
      delete traceToken;
      return cacheReturn;
    }

    Duration cacheEndDur             =    Time::local() - Time::epoch();
    traceToken->checkCacheLocalEnd   =     (uint64)cacheEndDur.toMicroseconds();

    traceToken->osegQLenPostQuery     =             (uint64)mStrandQLen;
    ++mStrandQLen;
    testSanityQLen();
    mStrand->post(boost::bind(&CraqObjectSegmentation::beginCraqLookup,this,obj_id, traceToken));

    return CraqEntry::null();
  }

  
  
  //this call actually wraps a call that performs the craq lookup
  void CraqObjectSegmentation::beginCraqLookup(const UUID& obj_id, OSegLookupTraceToken* traceToken)
  {
    --mStrandQLen;
    testSanityQLen();
    if (mReceivedStopRequest)
    {
      delete traceToken;
      return;
    }

    Duration beginCraqDur  = Time::local() - Time::epoch();
    traceToken->craqLookupBegin = (uint64)beginCraqDur.toMicroseconds();  //this one is the one that takes a long time.

    std::map<UUID,TransLookup>::const_iterator iter = mInTransitOrLookup.find(obj_id);

    //checks if the the object isn't already being looked up and the object isn't already in transit
    if (iter == mInTransitOrLookup.end()) 
    {
      CraqDataSetGet* cdSetGet = generateLookup(obj_id,traceToken);
      placeInTransOrLookup(obj_id,traceToken);
      craqDhtGet.get(cdSetGet,traceToken);
    }
    else
    {
      //object is already being looked up.
      Duration endCraqDur            =             Time::local() - Time::epoch();
      traceToken->craqLookupEnd      =            (uint64)   endCraqDur.toMicroseconds();
      mContext->trace()->osegCumulativeResponse(mContext->simTime(), traceToken);
    }
  }

  //this function generates a craq lookup.  Note: it also timestamps the passed in trace token.  it is called by beginCraqLookup
  CraqDataSetGet* CraqObjectSegmentation::generateLookup(const UUID& obj_id,OSegLookupTraceToken* traceToken)
  {
    Duration beginCraqLookupNotAlreadyLookingUpDur = Time::local() - Time::epoch();
    traceToken->craqLookupNotAlreadyLookingUpBegin  = (uint64)beginCraqLookupNotAlreadyLookingUpDur.toMicroseconds();


    CraqDataSetGet cdSetGet (indexer,CraqEntry::null(),false,CraqDataSetGet::GET); //bftm modified

    CraqDataSetGet* cdSetGet  = new CraqDataSetGet(indexer,0,false,CraqDataSetGet::GET); //bftm modified

    mapDataKeyToUUID[indexer] = obj_id;

    mContext->trace()->objectSegmentationCraqLookupRequest(mContext->simTime(),
                                                           obj_id,
                                                           mContext->id());

    return cdSetGet;
  }



  TransLookup tmpTransLookup;
  tmpTransLookup.sID = CraqEntry::null();  //means that we're performing a lookup, rather than a migrate.
  tmpTransLookup.timeAdmitted = (int)timerDur.toMilliseconds();

  TransLookup tmpTransLookup;
  tmpTransLookup.sID = CRAQ_OSEG_LOOKUP_SERVER_ID;  //means that we're performing a lookup, rather than a migrate.
  tmpTransLookup.timeAdmitted = (int)timerDur.toMilliseconds();
  
  mInTransitOrLookup[objid] = tmpTransLookup; //just says that we are performing a lookup on the object
  
  Duration endCraqDur  = Time::local() - Time::epoch();
  traceToken->craqLookupEnd = (uint64)endCraqDur.toMicroseconds();
  traceToken->craqLookupNotAlreadyLookingUpEnd = (uint64)endCraqDur.toMicroseconds();
}


  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  CBR::Protocol::OSeg::MigrateMessageAcknowledge* CraqObjectSegmentation::generateAcknowledgeMessage(const UUID &obj_id,float radius, ServerID sID_to)
  {
    CBR::Protocol::OSeg::MigrateMessageAcknowledge* oseg_ack_msg = new CBR::Protocol::OSeg::MigrateMessageAcknowledge();
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
void CraqObjectSegmentation::addObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool generateAck)
  {
    CraqDataKey cdk;
    convert_obj_id_to_dht_key(obj_id,cdk);

    CraqDataSetGet* cdSetGet = new CraqDataSetGet(cdk, mContext->id() ,true,CraqDataSetGet::SET);

    receivingObjects_m.lock();

    CraqDataSetGet cdSetGet(cdk, CraqEntry(mContext->id(),radius) ,true,CraqDataSetGet::SET);

    receivingObjects_m.lock();
    mReceivingObjects.insert(ObjectSet::value_type(obj_id,CraqEntry(mContext->id(),radius)));
    receivingObjects_m.unlock();

    TrackedSetResultsData tsrd;

    tsrd.migAckMsg = generateAcknowledgeMessage(obj_id, radius, idServerAckTo);
    //      tsrd.dur       = mTimer.elapsed();

    tsrd.dur =  Time::local() - Time::epoch();

    int trackID = mUniqueID.getUniqueTrackID();
    craqDhtSet.set(cdSetGet,trackID);
    trackingMessages[trackID] = tsrd;
  }


  //potentially clean up: we should really only be adding objects without
  //generating acks using the newObjectAdd interface
  void CraqObjectSegmentation::addObjectRequiresNoAck(const UUID& obj_id)
  {
    CraqDataKey cdk;
    convert_obj_id_to_dht_key(obj_id,cdk);

    CraqDataSetGet* cdSetGet = new CraqDataSetGet(cdk, mContext->id() ,false,CraqDataSetGet::SET);
    int trackID = mUniqueID.getUniqueTrackID();
    craqDhtSet.set(cdSetGet, trackID);

      std::pair<ObjectSet::iterator, bool> inserted = mObjects.insert(ObjectSet::value_type(obj_id,CraqEntry(mContext->id(),radius)));
      if (inserted.second)
      {
        std::cout<<"\n\nAdding object:  "<<obj_id.toString()<<"\n";
      }

    }
  }
  

  //This is the function that we use to add an object.  generateAck is true if you want to generate
  //an ack message to server idServerAckTo  If you're initially adding an object to the world, you should
  //use the newObjectAdd function instead.
  void CraqObjectSegmentation::addObject(const UUID& obj_id, const ServerID idServerAckTo, bool generateAck)
  {
    if (mReceivedStopRequest)
      return;

    if (generateAck)
      addObjectRequiresAck(obj_id,idServerAckTo);
    else
      addObjectRequiresNoAck(obj_id);

  }



  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
    Whatever calls this must verify that the object is on this server.
    I can do the check in the function by querrying bambooDht as well
  */
  void CraqObjectSegmentation::migrateObject(const UUID& obj_id, const CraqEntry& new_server_id)
  {
    if (mReceivedStopRequest)
      return;

    //log the message.
    CONTEXT_TRACE(objectBeginMigrate,
        obj_id,mContext->id(),
        new_server_id.server());

    std::map<UUID,TransLookup>::const_iterator transIter = mInTransitOrLookup.find(obj_id);

    TransLookup tmpTransLookup;
    tmpTransLookup.sID = new_server_id;
    Duration tmpDurer = Time::local() - Time::epoch();
    tmpTransLookup.timeAdmitted = (int)tmpDurer.toMilliseconds();
    mInTransitOrLookup[obj_id] = tmpTransLookup;


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


    if (traceToken != NULL) {
        CONTEXT_TRACE(osegCumulativeResponse, traceToken);
         I do not know that I have to delete the trace token here;
        delete traceToken;
    }

    postingStrand->post(std::tr1::bind (&OSegLookupListener::osegLookupCompleted,mLookupListener,obj_id,sID));
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

    if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_MOVE)
    {
      CBR::Protocol::OSeg::MigrateMessageMove oseg_move_msg;
      bool parsed = parsePBJMessage(&oseg_move_msg, msg->payload());
      if (parsed)
      {
        printf("\n\nMigrate message move in oseg is a deprecated message.  Killing\n\n");
        assert(false);
      }
    }
    else if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE)
    {
      CBR::Protocol::OSeg::MigrateMessageAcknowledge oseg_ack_msg;
      bool parsed = parsePBJMessage(&oseg_ack_msg, msg->payload());
      if (parsed)
      {
        ++mStrandQLen;
        testSanityQLen();
        mStrand->post(std::tr1::bind(&CraqObjectSegmentation::processMigrateMessageAcknowledge, this,oseg_ack_msg));
      }
    }
    else if (msg->dest_port() == SERVER_PORT_OSEG_UPDATE)
    {
      CBR::Protocol::OSeg::UpdateOSegMessage update_oseg_msg;
      bool parsed = parsePBJMessage(&update_oseg_msg, msg->payload());
      if (parsed)
      {
        ++mStrandQLen;
        testSanityQLen();
        mStrand->post(std::tr1::bind(&CraqObjectSegmentation::processUpdateOSegMessage,this,update_oseg_msg));
      }
    }
    else
    {
      printf("\n\nReceived the wrong type of message in receiveMessage of craqobjectsegmentation.cpp.\n\n ");
    }

    delete msg;
  }


  //called from within o_strand
  void CraqObjectSegmentation::processUpdateOSegMessage(const CBR::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg)
  {
    --mStrandQLen;
    testSanityQLen();
    if (mReceivedStopRequest)
      return;

    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, update_oseg_msg.m_objid(), CraqEntry(update_oseg_msg.servid_obj_on(),update_oseg_msg.m_objradius())));
  }

  //called from within o_strand.  Just received a migrate message ack.
  void CraqObjectSegmentation::processMigrateMessageAcknowledge(const CBR::Protocol::OSeg::MigrateMessageAcknowledge& msg)
  {
    --mStrandQLen;
    testSanityQLen();
    if (mReceivedStopRequest)
      return;

    CraqEntry serv_from(msg.m_servid_from(),msg.m_objradius()), serv_to(msg.m_servid_to(),msg.m_objradius());
    UUID obj_id;

    obj_id    = msg.m_objid();


    std::map<UUID,TransLookup>::iterator inTransIt;

    //put the value in the cache!
    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, obj_id, serv_from));

    inTransOrLookup_m.lock();
    inTransIt = mInTransitOrLookup.find(obj_id);
    if (inTransIt != mInTransitOrLookup.end())
    {
      //means that we now remove the obj_id from mInTransit
      mInTransitOrLookup.erase(inTransIt);

      callOsegLookupCompleted(obj_id,serv_from, NULL);


      //log reception of acknowled message
      CONTEXT_TRACE(objectAcknowledgeMigrate,
                    obj_id,serv_from.server(),
                    mContext->id());
    }
    inTransOrLookup_m.unlock();

    //send a message to the server that object should now disconnect
    mWriteListener->osegMigrationAcknowledged(obj_id);
  }

  
  void CraqObjectSegmentation::handleNewMigAckMessages()
  {
    trySendMigAcks();
  }

  void CraqObjectSegmentation::trySendMigAcks()
  {
    if (mReceivedStopRequest)
      return;

    while(true)
    {
      if (mFrontMigAck == NULL)
      {
        if (mMigAckMessages.empty())
          return;
        mMigAckMessages.pop(mFrontMigAck);
      }

      bool sent = mOSegServerMessageService->route( mFrontMigAck );
      if (!sent)
        break;

      mFrontMigAck = NULL;
    }

    if (mFrontMigAck != NULL || !mMigAckMessages.empty())
    {
      // We've still got work to do, setup a retry
      mContext->mainStrand->post(
                                 Duration::microseconds(100),
                                 std::tr1::bind(&CraqObjectSegmentation::trySendMigAcks, this)
                                 );
    }
  }


  //gets called by other functions to then post to oStrand
  void CraqObjectSegmentation::craqGetResult(CraqOperationResult* cor)
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
        craqDhtGet.get(cdSetGet, nfd->traceToken); //calling the craqDht to do a get.

        mNfData.pop(); //remove the item from the queue.

        delete nfd; //memory managment
      }
      queueDataOldEnough = queueDataOldEnough && (mNfData.size() > 0);  //don't keep going if you're out of objects.
    }
  }
  

  //gets posted to from asyncCraqGet.  Should get inside of o_strand from the post.
  void CraqObjectSegmentation::processCraqGetResult(CraqOperationResult* cor)
  {
    --mStrandQLen;
    testSanityQLen();
    Duration tmpDur = Time::local() - Time::epoch();

    if (cor->traceToken != NULL)
      cor->traceToken->lookupReturnBegin = (uint64)tmpDur.toMicroseconds();

    if (mReceivedStopRequest)
    {
      if (cor->traceToken != NULL)
        delete cor->traceToken;
      delete cor;
      return;
    }

    if (cor->servID.isNull())
    {
      if(cor->traceToken!= NULL)
        delete cor->traceToken;

      delete cor;

      std::cout<<"\n\nWe have an object that does not exist in craq system.  This shouldn't have really been called.\n\n";

      std::cout<<"Object:  "<<cor->objID<<"  server id:  "<<cor->servID.server()<<"  radius  "<<cor->servID.radius()<<"  tracking  "<<cor->tracking<<"  suceeded: "<<cor->succeeded<<"  GET or set:   "<<cor->whichOperation<<"\n\n";


      assert(false);
      return;
    }

    finishProcessGetResult(cor);
  }


  
  void CraqObjectSegmentation::finishProcessGetResult(CraqOperationResult* cor)
  {
    UUID tmper = mapDataKeyToUUID[cor->idToString()];
    
    //put the value in the cache!
    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, tmper, cor->servID));

    inTransOrLookup_m.lock();
    std::map<UUID,TransLookup>::iterator iter = mInTransitOrLookup.find(tmper);

    //if true, means that the object was already being looked up or in transit
    if (iter != mInTransitOrLookup.end()) 
    {
      //log message stating that object was processed.
      Duration timerDur = Time::local() - Time::epoch();

      CONTEXT_TRACE(objectSegmentationProcessedRequest,
          tmper,
          cor->servID.server(),
          mContext->id(),
          (uint32) (((int) timerDur.toMilliseconds()) - (int)(iter->second.timeAdmitted)),
          (uint32) craqDhtGet.queueSize()  );


      if(iter->second.sID.isNull())
      {
        //means that after receiving a lookup request, we did not intermediarilly receive a migrate request.
        //if above predicate is not true, means that we did receive a migrate request, and therefore should
        //not delete that migrate request from mInTransitOrLookup (ie do nothing).
        mInTransitOrLookup.erase(iter);
      }
    }
    inTransOrLookup_m.unlock();

    Duration tmpDurEnd = Time::local() - Time::epoch();
    if (cor->traceToken != NULL)
      cor->traceToken->lookupReturnEnd = (uint64)tmpDurEnd.toMicroseconds();


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
    std::map<UUID,TransLookup>::iterator inTransLookIter = mInTransitOrLookup.find(obj_id);

    //checks if the object can now be removed from mInTransitOrLookup
    if (inTransLookIter != mInTransitOrLookup.end())
      mInTransitOrLookup.erase(inTransLookIter);


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

  void CraqObjectSegmentation::processSetSendAck(CraqOperationResult* trackedSetResult)
  {
    UUID obj_id = trackingMessages[trackedSetResult->trackedMessage].migAckMsg->m_objid();

    //add this to the cache
    postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, obj_id, mContext->id()));

    //add to mObjects the uuid associated with trackedMessage (ie, now we know that we own the object.)
    mObjects[obj_id] = true;
    
    //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
    removeFromInTransOrLookup(obj_id);

    //remove this object from mReceivingObjects,
    removeFromReceivingObjects(obj_id);

    //log event
    Duration procTrackedSetRes = Time::local() - Time::epoch();
    int durMs = procTrackedSetRes.toMilliseconds() - trackingMessages[trackedSetResult->trackedMessage].dur.toMilliseconds();


    CONTEXT_TRACE(processOSegTrackedSetResults,
                  obj_id,
                  mContext->id(),
                  durMs);

    
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
  

  void CraqObjectSegmentation::craqSetResult(CraqOperationResult* trackedSetResult)
  {
    if (mReceivedStopRequest)
    {
      delete trackedSetResult;
      return;
    }
    if(trackedSetResult->trackedMessage == 0)  
    {
      //means that we weren't supposed to be tracking this message
      delete trackedSetResult;
      return;
    }
    
    ++mStrandQLen;
    testSanityQLen();
    mStrand->post(boost::bind(&CraqObjectSegmentation::processCraqSetResult, this, trackedSetResult));
    //    lkjs;
  }
    
  //gets posted to from craqSetResult.  Should get inside of o_strand from the post.
  void CraqObjectSegmentation::processCraqSetResult(CraqOperationResult* trackedSetResult)
  {
    --mStrandQLen;
    testSanityQLen();
    if (mReceivedStopRequest)
    {
      delete trackedSetResult;
      return;
    }
    if(trackedSetResult->trackedMessage == 0)  
    {
      //means that we weren't supposed to be tracking this message
      delete trackedSetResult;
      return;
    }
    TrackedMessageMapAdded::iterator awhere;
    TrackedMessageMap::iterator where=trackingMessages.find(trackedSetResult->trackedMessage);
    if (where != trackingMessages.end())
    {
      //means that we have a record of locally tracking this message and that we'll have to send an ack.
      processSetSendAck(trackedSetResult);

      UUID obj_id = where->second.migAckMsg->m_objid();
      float radius=where->second.migAckMsg->m_objradius();

      //add this to the cache
      postingStrand->post(boost::bind( &CraqCacheGood::insert, &mCraqCache, obj_id, CraqEntry(mContext->id(),radius)));

      //add tomObjects the uuid associated with trackedMessage (ie, now we know that we own the object.)
      mObjects.insert(ObjectSet::value_type(obj_id,CraqEntry(mContext->id(),radius)));

      //delete the mInTransitOrLookup entry for this object sequence because now we know where it is.
      removeFromInTransOrLookup(obj_id);

      //remove this object from mReceivingObjects,
      removeFromReceivingObjects(obj_id);

      //log event
      Duration procTrackedSetRes = Time::local() - Time::epoch();

      int durMs = procTrackedSetRes.toMilliseconds() - trackingMessages[trackedSetResult->trackedMessage].dur.toMilliseconds();

      CONTEXT_TRACE(processOSegTrackedSetResults,
          obj_id,
          mContext->id(),
          durMs);

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

  //used when received a set result that we were not tracking, and therefore did not need to send an ack for.
  void CraqObjectSegmentation::processSetNoAck(CraqOperationResult* trackedSetResult)
  {
    //need to add obj_id
    mObjects[trackedAddMessages[trackedSetResult->trackedMessage].msgAdded->m_objid()] = true; 

    UUID written_obj = trackedAddMessages[trackedSetResult->trackedMessage].msgAdded->m_objid();
    mWriteListener->osegWriteFinished(written_obj);
  }
  

  std::vector<PollingService*> CraqObjectSegmentation::getNestedPollers()
  {
    std::vector <PollingService*> returner;

    std::vector <PollingService*> getPollingServices = craqDhtGet.getPollingServices();
    std::vector <PollingService*> setPollingServices = craqDhtSet.getPollingServices();

    returner.insert(returner.end(),getPollingServices.begin(), getPollingServices.end());
    returner.insert(returner.end(),setPollingServices.begin(), setPollingServices.end());

    return returner;
  }

  void CraqObjectSegmentation::testSanityQLen()
  {
    std::cout<<"\n ql: "<<mStrandQLen<<"\n";
  }

}//namespace CBR
