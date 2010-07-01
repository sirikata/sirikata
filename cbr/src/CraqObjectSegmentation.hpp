/*  Sirikata
 *  CraqObjectSegmentation.hpp
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

#ifndef _SIRIKATA_DHT_OBJECT_SEGMENTATION_HPP_
#define _SIRIKATA_DHT_OBJECT_SEGMENTATION_HPP_

#include <sirikata/core/trace/Trace.hpp>
#include "ServerMessage.hpp"
#include "ObjectSegmentation.hpp"
#include "craq_oseg/asyncCraq.hpp"
#include "craq_oseg/asyncUtil.hpp"
#include "craq_oseg/asyncConnection.hpp"
#include "CoordinateSegmentation.hpp"
#include <string.h>
#include <vector>

#include "caches/Cache.hpp"
#include "caches/CommunicationCache.hpp"
#include "caches/CacheLRUOriginal.hpp"

#include "OSegLookupTraceToken.hpp"
#include "craq_hybrid/asyncCraqHybrid.hpp"
#include "craq_hybrid/asyncCraqUtil.hpp"
#include <boost/thread/mutex.hpp>

#include "CBR_OSeg.pbj.hpp"

#include <sirikata/core/queue/ThreadSafeQueueWithNotification.hpp>

//#define CRAQ_DEBUG
#define CRAQ_CACHE


namespace Sirikata
{

  struct TransLookup
  {
    CraqEntry sID;

    int timeAdmitted;
    TransLookup():sID(CraqEntry::null()){}
  };


  static const int CRAQ_NOT_FOUND_SIT_OUT   =  500; //that's ms

  class CraqObjectSegmentation : public ObjectSegmentation
  {
  private:
      typedef std::tr1::unordered_map<UUID, CraqEntry, UUID::Hasher> ObjectSet;

    CoordinateSegmentation* mCSeg; //will be used in lookup call

      Router<Message*>* mOSegServerMessageService;

    double checkOwnTimeDur;
    int checkOwnTimeCount;


    //debugging:

    char myUniquePrefixKey; //should just be one character long.

    //for logging
    int numCacheHits;
    int numOnThisServer;
    int numLookups;
    int numCraqLookups;
    int numTimeElapsedCacheEviction;
    int numMigrationNotCompleteYet;
    int numAlreadyLookingUp;
    int numServices;
    int numLookingUpDebug;
    Duration lastTimerDur;
    //end for logging.

    std::map<std::string, UUID > mapDataKeyToUUID;
      typedef std::tr1::unordered_map<UUID,TransLookup,UUID::Hasher> InTransitMap;
      InTransitMap mInTransitOrLookup;//These are the objects that are in transit from this server to another.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we
    boost::mutex inTransOrLookup_m;


    struct TrackedSetResultsData
    {
      Sirikata::Protocol::OSeg::MigrateMessageAcknowledge* migAckMsg;
      Duration dur;
    };

      typedef std::tr1::unordered_map<int, TrackedSetResultsData> TrackedMessageMap;
    TrackedMessageMap trackingMessages;

      ObjectSet mReceivingObjects; //this is a vector of objects that have been pushed to this server, but whose migration isn't complete yet, becase we don't have an ack from CRAQ that they've been stored yet.
    boost::mutex receivingObjects_m;


    //what to do when craq can't find the object
    void notFoundFunction(CraqOperationResult* nf); //this function tells us what to do with all the ids that just weren't found in craq.

    struct NotFoundData
    {
      Duration dur;
      UUID obj_id;
      OSegLookupTraceToken* traceToken;
    };
    typedef std::queue<NotFoundData*> NfDataQ;
    NfDataQ mNfData;
    void checkNotFoundData();
    //end what to do when craq can't find the object


    //for lookups and sets respectively
    AsyncCraqHybrid craqDhtGet;
    AsyncCraqHybrid craqDhtSet;



    int mAtomicTrackID;
    boost::mutex atomic_track_id_m;
    int getUniqueTrackID();

    Network::IOStrand* postingStrand;
    Network::IOStrand* mStrand;

    void convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const;

      ObjectSet mObjects; //a list of the objects that are currently being hosted on the space server associated with this oseg.
    bool checkOwn(const UUID& obj_id, float*radius);
    bool checkMigratingFromNotCompleteYet(const UUID& obj_id,float*radius);

    void removeFromInTransOrLookup(const UUID& obj_id);
    void removeFromReceivingObjects(const UUID& obj_id);

    //for message addition. when add an object, send a message to the server that you can now finish adding it to forwarder, loc services, etc.
    struct TrackedSetResultsDataAdded
    {
      Sirikata::Protocol::OSeg::AddedObjectMessage* msgAdded;
      Duration dur;
    };
      typedef std::tr1::unordered_map<int, TrackedSetResultsDataAdded> TrackedMessageMapAdded;
    TrackedMessageMapAdded trackedAddMessages; // so that can't query for object until it's registered.
    Sirikata::Protocol::OSeg::AddedObjectMessage* generateAddedMessage(const UUID& obj_id, float radius);
    //end message addition.



    //building for the cache
    CraqEntry satisfiesCache(const UUID& obj_id);
    CraqCache* mCraqCache;
    //end building for the cache


      Sirikata::ThreadSafeQueueWithNotification<Message*> mMigAckMessages;
      Message* mFrontMigAck;
      void handleNewMigAckMessages();
      void trySendMigAcks();

    void beginCraqLookup(const UUID& obj_id, OSegLookupTraceToken* traceToken);
    void callOsegLookupCompleted(const UUID& obj_id, const CraqEntry& sID, OSegLookupTraceToken* traceToken);


    SpaceContext* ctx;
    bool mReceivedStopRequest;

  public:
    CraqObjectSegmentation (SpaceContext* con, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> getInitArgs, std::vector<CraqInitializeArgs> setInitArgs, char prefixID, Network::IOStrand* o_strand, Network::IOStrand* strand_to_post_to);


    virtual ~CraqObjectSegmentation();
    virtual CraqEntry lookup(const UUID& obj_id);
    virtual CraqEntry cacheLookup(const UUID& obj_id);
    virtual void migrateObject(const UUID& obj_id, const CraqEntry& new_server_id);
    virtual void addObject(const UUID& obj_id, float radius, ServerID idServerAckTo, bool);
    virtual void receiveMessage(Message* msg);
    virtual bool clearToMigrate(const UUID& obj_id);
    virtual void newObjectAdd(const UUID& obj_id, float radius);
    virtual void craqGetResult(CraqOperationResult* cor);
    virtual void craqSetResult(CraqOperationResult* cor);
    virtual void stop();




    Sirikata::Protocol::OSeg::MigrateMessageAcknowledge* generateAcknowledgeMessage(const UUID &obj_id, float radius, ServerID serverToAckTo);
    void processMigrateMessageAcknowledge(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg);
    void processMigrateMessageMove(const Sirikata::Protocol::OSeg::MigrateMessageMove& msg);
    void processUpdateOSegMessage(const Sirikata::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg);

  };
}
#endif
