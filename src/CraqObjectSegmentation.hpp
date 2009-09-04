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
  class CraqObjectSegmentation : public ObjectSegmentation
  {
  private:
    CoordinateSegmentation* mCSeg; //will be used in lookup call


    std::map<std::string, UUID > mapDataKeyToUUID;
    std::map<UUID,ServerID> mInTransitOrLookup;//These are the objects that are in transit.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we
    std::map<UUID,ServerID> mFinishedMoveOrLookup;
    std::map<int,OSegMigrateMessageAcknowledge*> trackingMessages;


    AsyncCraq craqDht;
    void convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const;

    std::vector <UUID> mObjects; //a list of the objects that are currently being hosted on the space server associated with this oseg.
    bool checkOwn(const UUID& obj_id);

  public:
      CraqObjectSegmentation (SpaceContext* ctx, CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, std::vector<CraqInitializeArgs> initArgs);


    virtual ~CraqObjectSegmentation();
    virtual void lookup(const UUID& obj_id);
    virtual void service(std::map<UUID,ServerID>& updated);
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
    virtual void addObject(const UUID& obj_id, const ServerID idServerAckTo, bool);

    virtual void receiveMessage(Message* msg);

    OSegMigrateMessageAcknowledge* generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to);
    void processMigrateMessageAcknowledge(OSegMigrateMessageAcknowledge* msg);
    void processMigrateMessageMove(OSegMigrateMessageMove* msg);
    void processCraqTrackedSetResults(std::vector<CraqOperationResult> &trackedSetResults);
  };
}
#endif
