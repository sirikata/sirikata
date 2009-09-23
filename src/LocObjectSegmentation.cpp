#include "LocObjectSegmentation.hpp"
#include "Message.hpp"
#include "Network.hpp"
#include <map>
#include <vector>
#include "LocationService.hpp"
#include "CoordinateSegmentation.hpp"
#include "Statistics.hpp"
#include "Utility.hpp"

namespace CBR
{
  /*
    Constructor
  */
  //LocObjectSegmentation::LocObjectSegmentation(SpaceContext* ctx, CoordinateSegmentation* cseg, LocationService* loc_service,std::map<UUID,ServerID> objectToServerMap)
  LocObjectSegmentation::LocObjectSegmentation(SpaceContext* ctx, CoordinateSegmentation* cseg, LocationService* loc_service,std::map<UUID,ServerID> objectToServerMap, Forwarder* fder)
 : ObjectSegmentation(ctx),
   mCSeg (cseg),
   mLocationService(loc_service),
   mObjectToServerMap(objectToServerMap),
   mForwarder(fder)
  {

  }

  /*
    Destructor
  */
  LocObjectSegmentation::~LocObjectSegmentation()
  {
    //nothing to really destruct.
  }

  bool LocObjectSegmentation::clearToMigrate(const UUID& obj_id)
  {
    return true;
  }

  int LocObjectSegmentation::getOSegType()
  {
    return LOC_OSEG;
  }
  

  /*
    Lookup server id based on check with location service and mcseg.
  */
  ServerID LocObjectSegmentation::lookup(const UUID& obj_id)
  {
    if (mForwarder->getObjectConnection(obj_id) != NULL)
    {
      return mContext->id();
    }

    
      if (mLocationService->contains(obj_id)) {
          Vector3f pos = mLocationService->currentPosition(obj_id);
          ServerID sid = mCSeg->lookup(pos);
          mObjectToServerMap[obj_id] = sid;

          //          return sid;
      }
      else {
          // XXX FIXME relying on the location service just isn't really an option any more
          // since it doesn't have global knowledge.  For now we just have to route randomly and
          // hope for convergence. EEK!
          ServerID sid = (ServerID)((rand() % mCSeg->numServers()) + 1);
          mObjectToServerMap[obj_id] = sid;
          //          return sid;
      }
               //    Vector3f pos = mLocationService->currentPosition(obj_id);
               //    ServerID sid = mCSeg->lookup(pos);
               //    Vector3f pos = mLocationService->currentPosition(obj_id);
               //    ServerID sid = mCSeg->lookup(pos);
               //    mObjectToServerMap[obj_id] = sid;

    //    return sid;
               //    return;

      return NullServerID;
  }


  void LocObjectSegmentation::receiveMessage(Message* msg)
  {
    delete msg; //delete message here.
  }

  
  /*
    Nothing to populate with currently
    Doesn't do anything with the messages currently.
   */
  void LocObjectSegmentation::service(std::map<UUID,ServerID> &updated)
  {
    updated = mObjectToServerMap;
    mObjectToServerMap.clear();
    return;
  }


  /*
    Dummy function.
  */
  ServerID LocObjectSegmentation::getHostServerID()
  {
    return -1;
  }

  
  /*
    Dummy function
  */
  void LocObjectSegmentation::migrateObject(const UUID& obj_id, const ServerID new_server_id)
  {
    return;
  }

  /*
    Dummy function
  */
  void LocObjectSegmentation::addObject(const UUID& obj_id, const ServerID ourID, bool nothingHere)
  {
    return;
  }

  void LocObjectSegmentation::newObjectAdd(const UUID& obj_id)
  {
    return;
  }

}
