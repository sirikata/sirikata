#ifndef _CBR_DHT_OBJECT_SEGMENTATION_HPP_
#define _CBR_DHT_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Object.hpp"
#include "Statistics.hpp"
#include "Timer.hpp"
#include "ObjectSegmentation.hpp"
#include "craq_oseg/asyncCraq.hpp"
#include "craq_oseg/asyncUtil.hpp"
#include "craq_oseg/asyncConnection.hpp"
#include "CoordinateSegmentation.hpp"
#include <string.h>
#include <vector>

namespace CBR
{

  struct TransLookup
  {
    ServerID sID;
    uint64 timeAdmitted;
  };

  const ServerID CRAQ_OSEG_LOOKUP_SERVER_ID = NullServerID;
  
  
  class CraqObjectSegmentation : public ObjectSegmentation
  {
  private:
    CoordinateSegmentation* mCSeg; //will be used in lookup call

    //debugging:
    uint64 numTicks;

    Timer mTimer;

    char myUniquePrefixKey; //should just be one character long.

    std::map<std::string, UUID > mapDataKeyToUUID;
    //    std::map<UUID,ServerID> mInTransitOrLookup;//These are the objects that are in transit.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we

    std::map<UUID,TransLookup> mInTransitOrLookup;//These are the objects that are in transit from this server to another.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we
    
    std::map<UUID,ServerID> mFinishedMoveOrLookup;
    std::map<int,OSegMigrateMessageAcknowledge*> trackingMessages;
    std::vector<UUID> mReceivingObjects;

    
    void iteratedWait(int numWaits,std::vector<CraqOperationResult> &allGetResults,std::vector<CraqOperationResult>&allTrackedResults);
    void basicWait(std::vector<CraqOperationResult> &allGetResults,std::vector<CraqOperationResult>&allTrackedResults);

    
    AsyncCraq craqDht;
    void convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const;

    std::vector <UUID> mObjects; //a list of the objects that are currently being hosted on the space server associated with this oseg.
    bool checkOwn(const UUID& obj_id);

  public:
      CraqObjectSegmentation (SpaceContext* ctx, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> initArgs, char);


    virtual ~CraqObjectSegmentation();
    virtual void lookup(const UUID& obj_id);
    virtual void service(std::map<UUID,ServerID>& updated);
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
    virtual void addObject(const UUID& obj_id, const ServerID idServerAckTo, bool);

    virtual void receiveMessage(Message* msg);

    virtual bool clearToMigrate(const UUID& obj_id);

    
    OSegMigrateMessageAcknowledge* generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to);
    void processMigrateMessageAcknowledge(OSegMigrateMessageAcknowledge* msg);
    void processMigrateMessageMove(OSegMigrateMessageMove* msg);
    //    void processCraqTrackedSetResults(std::vector<CraqOperationResult> &trackedSetResults);
    void processCraqTrackedSetResults(std::vector<CraqOperationResult> &trackedSetResults, std::map<UUID,ServerID>& updated);
  };
}
#endif
