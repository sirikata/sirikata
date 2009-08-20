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

//#include "oseg_dht/Bamboo.hpp"
//#include "oseg_dht/gateway_prot.h"

#include "CoordinateSegmentation.hpp"
#include <string.h>


namespace CBR
{
  class CraqObjectSegmentation : public ObjectSegmentation
  {                          
  private:
    CoordinateSegmentation* mCSeg; //will be used in lookup call

    //    std::map<CraqDataKey,UUID> mapDataKeyToUUID;
    std::map<std::string, UUID > mapDataKeyToUUID;
    std::map<UUID,ServerID> mInTransitOrLookup;//These are the objects that are in transit.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we 
    std::map<UUID,ServerID> mFinishedMoveOrLookup;
    std::map<int,OSegMigrateMessageAcknowledge*> trackingMessages;
    
    
    Time mCurrentTime;
    //Bamboo bambooDht;//(char* host, int port);
    AsyncCraq craqDht;
    void convert_obj_id_to_dht_key(const UUID& obj_id, CraqDataKey& returner) const;

    
  public:
    CraqObjectSegmentation (CoordinateSegmentation* cseg, std::vector<UUID> vectorOfObjectsInitializedOnThisServer, ServerID servID,  Trace* tracer, std::vector<CraqInitializeArgs> initArgs,MessageRouter* router, MessageDispatcher* dispatcher );

    
    virtual ~CraqObjectSegmentation();
    virtual void lookup(const UUID& obj_id);
    //    virtual void osegMigrateMessage(OSegMigrateMessage*);
    virtual void tick(const Time& t, std::map<UUID,ServerID>& updated);
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
    //    virtual void addObject(const UUID& obj_id, Object* obj, const ServerID idServerAckTo);

    virtual void addObject(const UUID& obj_id, const ServerID idServerAckTo);
    //    virtual void addObject(const UUID& obj_id, const ServerID ourID) = 0;
    //    virtual  Message* generateAcknowledgeMessage(const UUID& obj_id,ServerID sID_to) = 0;


    
    virtual ServerID getHostServerID();

    virtual void receiveMessage(Message* msg);


    //    OSegMigrateMessageAcknowledge* generateAcknowledgeMessage(Object* obj, ServerID sID_to);
    OSegMigrateMessageAcknowledge* generateAcknowledgeMessage(const UUID &obj_id,ServerID sID_to);
    void processMigrateMessageAcknowledge(OSegMigrateMessageAcknowledge* msg);
    void processMigrateMessageMove(OSegMigrateMessageMove* msg);
    void processCraqTrackedSetResults(std::vector<CraqOperationResult> &trackedSetResults);
  };
}
#endif
