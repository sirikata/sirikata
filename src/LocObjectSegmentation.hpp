#ifndef _CBR_LOC_OBJECT_SEGMENTATION_HPP_
#define _CBR_LOC_OBJECT_SEGMENTATION_HPP_

/*
  Uses a call to loc and cseg to determine position of object.
*/


#include "ObjectSegmentation.hpp"


namespace CBR
{

  class LocObjectSegmentation : ObjectSegmentation
  {
  private:
    CoordinateSegmentation* mCSeg; //will be used in lookup call
    LocationService* mLocationService; //will be used in lookup call
    std::vector<ServID> mServerList;   //initialized with this
    std::map<UUID,ServID> mObjectToServerMap;  //initialized with this

    
  public:
    LocObjectSegmentation(CoordinateSegmentation* cseg, LocationService* loc_service,Trace* tracer,std::vector<ServID> serverList,std::map<UUID,ServID> objectToServerMap);
    ~LocObjectSegmentation();

    virtual ServerID lookup(const UUID& obj_id);
    virtual void osegChangeMessage(OSegChangeMessage*);
    virtual void tick(const Time& t);

  };
}
#endif
