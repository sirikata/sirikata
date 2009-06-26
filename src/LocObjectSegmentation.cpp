
#include "LocObjectSegmentation.hpp"


namespace CBR
{

  /*
    Constructor
  */
  //  LocObjectSegmentation::LocObjectSegmentation(CoordinateSegmentation* cseg, LocationService* loc_service)
  //  LocObjectSegmentation::LocObjectSegmentation(CoordinateSegmentation* cseg, LocationService* loc_service,std::vector<ServID> serverList,std::map<UUID,ServID> objectToServerMap)
  LocObjectSegmentation::LocObjectSegmentation(CoordinateSegmentation* cseg, LocationService* loc_service,Trace* tracer,std::vector<ServID> serverList,std::map<UUID,ServID> objectToServerMap)
    : mCSeg (cseg),
      mLocationService(loc_service),
      mTrace(tracer),
      mServerList(serverList),
      mObjectToServerMap(objectToServerMap)
  {
    
  }

  /*
    Destructor
  */
  LocObjectSegmentation::~LocObjectSegmentation()
  {
    //nothing to really destruct.
  }


  /*
    Lookup server id based on check with location service and mcseg.
  */
  ServerID LocObjectSegmentation::lookup(const UUID& obj_id)
  {
    Vector3f pos = mLocationService->currentPosition(obj_id);
    ServerID sid = mCSeg->lookup(pos);
    return sid;
  }


  /*
    Behavior:
    If receives a move message, moves object in mObjectToServerMap to another server.  (If object didn't exist before trying to move it, we create it, and put it on assigned server.)
    If receives a create message and object already exists, moves object to new server.
    If receives a kill message, deletes object server pair from map.
  */
  void LocObjectSegmentation::osegChangeMessage(OSegChangeMessage* msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;
    OsegAction oaction;

    serv_from = msg->getServFrom();
    serv_to   = msg->getServTo();
    obj_id    = msg->getObjID();
    oaction   = msg->getAction();
    
    switch (oaction)
    {
      case CREATE:      //msg says something has been added
        if (mObjectToServerMap.find(obj_id) != std::map::end)
        {
          //means that object exists.  we will move it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        else
        {
          //means that object doesn't exist.  we will create it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        break;
      case KILL:    //msg says object has been deleted

        mObjectToServerMap.erase(obj_id);
                  
        break;
      case MOVE:     //means that an object moved from one server to another.
        if (mObjectToServerMap.find(obj_id) != std::map::end)
        {
          //means that object exists.  Will move it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        else
        {
          //means that object doesn't exist.  We will create it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        break;
    } 
    

    
  }

  /*
    Nothing to populate with currently
   */
  void LocObjectSegmentation::tick(const Time& t)
  {
    
  }


}

