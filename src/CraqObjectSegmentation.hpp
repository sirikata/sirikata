#ifndef _CBR_DHT_OBJECT_SEGMENTATION_HPP_
#define _CBR_DHT_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Statistics.hpp"
#include "Timer.hpp"
#include "ObjectSegmentation.hpp"
#include "craq_oseg/asyncCraq.hpp"
#include "craq_oseg/asyncUtil.hpp"
#include "craq_oseg/asyncConnection.hpp"
#include "CoordinateSegmentation.hpp"
#include <string.h>
#include <vector>
#include "CraqCacheGood.hpp"

#include "OSegLookupTraceToken.hpp"
#include "craq_hybrid/asyncCraqHybrid.hpp"
#include "craq_hybrid/asyncCraqUtil.hpp"
#include <boost/thread/mutex.hpp>
#include "OSegLookupTraceToken.hpp"

#include "CBR_OSeg.pbj.hpp"

#include <sirikata/util/ThreadSafeQueueWithNotification.hpp>

//#define CRAQ_DEBUG
#define CRAQ_CACHE


namespace CBR
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
    Timer mServiceTimer;
    Duration lastTimerDur;
    //end for logging.

    std::map<std::string, UUID > mapDataKeyToUUID;
    std::map<UUID,TransLookup> mInTransitOrLookup;//These are the objects that are in transit from this server to another.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we
    boost::mutex inTransOrLookup_m;


    struct TrackedSetResultsData
    {
      CBR::Protocol::OSeg::MigrateMessageAcknowledge* migAckMsg;
      Duration dur;
    };

    typedef std::map<int, TrackedSetResultsData> TrackedMessageMap;
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

    IOStrand* postingStrand;
    IOStrand* mStrand;

    void convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const;

      ObjectSet mObjects; //a list of the objects that are currently being hosted on the space server associated with this oseg.
    bool checkOwn(const UUID& obj_id, float*radius);
    bool checkMigratingFromNotCompleteYet(const UUID& obj_id,float*radius);

    void removeFromInTransOrLookup(const UUID& obj_id);
    void removeFromReceivingObjects(const UUID& obj_id);

    //for message addition. when add an object, send a message to the server that you can now finish adding it to forwarder, loc services, etc.
    struct TrackedSetResultsDataAdded
    {
      CBR::Protocol::OSeg::AddedObjectMessage* msgAdded;
      Duration dur;
    };
    typedef std::map<int, TrackedSetResultsDataAdded> TrackedMessageMapAdded;
    TrackedMessageMapAdded trackedAddMessages; // so that can't query for object until it's registered.
    CBR::Protocol::OSeg::AddedObjectMessage* generateAddedMessage(const UUID& obj_id, float radius);
    //end message addition.



    //building for the cache
    CraqEntry satisfiesCache(const UUID& obj_id);
    CraqCacheGood mCraqCache;
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
    CraqObjectSegmentation (SpaceContext* con, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> getInitArgs, std::vector<CraqInitializeArgs> setInitArgs, char prefixID, IOStrand* o_strand, IOStrand* strand_to_post_to);


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
    virtual std::vector<PollingService*> getNestedPollers();
    virtual void stop();




    CBR::Protocol::OSeg::MigrateMessageAcknowledge* generateAcknowledgeMessage(const UUID &obj_id, float radius, ServerID serverToAckTo);
    void processMigrateMessageAcknowledge(const CBR::Protocol::OSeg::MigrateMessageAcknowledge& msg);
    void processMigrateMessageMove(const CBR::Protocol::OSeg::MigrateMessageMove& msg);
    void processUpdateOSegMessage(const CBR::Protocol::OSeg::UpdateOSegMessage& update_oseg_msg);

  };
}
#endif
