#ifndef _CBR_LOC_OBJECT_SEGMENTATION_HPP_
#define _CBR_LOC_OBJECT_SEGMENTATION_HPP_

/*
  Uses a call to loc and cseg to determine position of object.
*/


#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include "LocationService.hpp"
#include "CoordinateSegmentation.hpp"
#include "Statistics.hpp"
#include "Utility.hpp"
#include "Forwarder.hpp"
#include <map>
#include <vector>


namespace CBR
{

  class LocObjectSegmentation : public ObjectSegmentation
  {
  private:
    CoordinateSegmentation* mCSeg; //will be used in lookup call
    LocationService* mLocationService; //will be used in lookup call
    std::map<UUID,ServerID> mObjectToServerMap;  //initialized with this


  public:
    //    LocObjectSegmentation(CoordinateSegmentation* cseg, LocationService* loc_service,Trace* tracer,std::vector<ServerID> serverList,std::map<UUID,ServerID> objectToServerMap);

    LocObjectSegmentation(CoordinateSegmentation* cseg, LocationService* loc_service,std::map<UUID,ServerID> objectToServerMap);
    virtual ~LocObjectSegmentation();

    //virtual void lookup(const UUID& obj_id) const;
    virtual void lookup(const UUID& obj_id);

    //virtual  Message* generateAcknowledgeMessage(const UUID& obj_id, ServerID sID_to);
    //virtual void osegMigrateMessage(OSegMigrateMessage*);
    //virtual void processLookupMessage(OSegLookupMessage* msg);
    virtual void tick(const Time& t, std::map<UUID,ServerID>& updated);
    virtual ServerID getHostServerID();
    virtual void receiveMessage(Message* msg);
    
    virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
    virtual void addObject(const UUID& obj_id, const ServerID ourID);
    //virtual void getMessages(std::vector<Message*> &messToSendFromOSegToForwarder, std::vector<ServerID> &destServers );
    
  };
}
#endif
