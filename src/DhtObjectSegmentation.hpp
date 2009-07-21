#ifndef _CBR_DHT_OBJECT_SEGMENTATION_HPP_
#define _CBR_DHT_OBJECT_SEGMENTATION_HPP_

#include "Utility.hpp"
#include "Statistics.hpp"
#include "Message.hpp"
#include "Object.hpp"
#include "Statistics.hpp"
#include "Time.hpp"
#include "ObjectSegmentation.hpp"

#include "oseg_dht/Bamboo.hpp"
#include "oseg_dht/gateway_prot.h"

#include "CoordinateSegmentation.hpp"

//object segmenter h file

namespace CBR
{

  class DhtObjectSegmentation : public ObjectSegmentation
  {                          

  private:

    CoordinateSegmentation* mCSeg; //will be used in lookup call
    
    std::map<UUID,ServerID> mInTransit;//These are the objects that are in transit.  When we receive an acknowledge message from the oseg that these objects are being sent to, then we remove that object's id from being in transit, then we 
    std::map<UUID,ServerID> mFinishedMove;

    Time mCurrentTime;

    Bamboo bambooDht;//(char* host, int port);


    void     convert_serv_id_to_dht_val   (const ServerID sID, Bamboo::Bamboo_val& returner) const;
    void     convert_obj_id_to_dht_key    (const UUID& obj_id,    Bamboo::Bamboo_key& returner) const;

      
    ServerID convert_dht_val_to_server_id (Bamboo::Bamboo_val bVal) const;

    
    
  public:
    //    DhtObjectSegmentation(CoordinateSegmentation* cseg, std::vector<UUID*> vectorOfObjectsInitializedOnThisServer,ServerID servID,  Trace* tracer, char* dht_host, int dht_port);

    DhtObjectSegmentation(CoordinateSegmentation* cseg, std::vector<UUID*> vectorOfObjectsInitializedOnThisServer,ServerID servID,  Trace* tracer, char* dht_host, int dht_port);

    virtual ~DhtObjectSegmentation();
    
    virtual ServerID lookup(const UUID& obj_id) const;
    virtual void osegMigrateMessage(OSegMigrateMessage*);
    virtual void tick(const Time& t, std::map<UUID,ServerID>& updated);
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
    virtual void addObject(const UUID& obj_id, const ServerID ourID);
    virtual Message* generateAcknowledgeMessage(Object* obj, ServerID sID_to);
    virtual ServerID getHostServerID();

  };
}
#endif
