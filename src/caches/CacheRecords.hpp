
#include "Utility.hpp"
#include "../SpaceContext.hpp"

#ifndef __CACHE_RECORDS_HPP__
#define __CACHE_RECORDS_HPP__

namespace CBR
{

  const static int CACHE_RECORDS_NULL_VEC_INDEX = -1;


  
  //fcache
  struct FCacheRecord
  {
    FCacheRecord();
    FCacheRecord(UUID oid, ServerID bid, double pAvg, int reqsAftEvict,int gEvNum,double weight,double distance,double radius,double lookupWeighter,double distanceScalingUnits, SpaceContext* spctx, double vMag);
  
    FCacheRecord(const FCacheRecord& bcr);
    FCacheRecord(const FCacheRecord* bcr);
    void copy(UUID oid, ServerID bid, double pAvg, int reqsAftEvict,int gEvNum,int index,double weight, double distance, double radius,double lookupWeighter,double distScaleUnits, SpaceContext* spctx, CacheTimeMS lastAccessed , double vMag);

    void printRecord();
    ~FCacheRecord();
    ServerID                     bID;
    UUID                      objid;
    double                    popAvg;
    int               reqsSinceEvict;
    int                 lastEvictNum;
    int                     vecIndex;
    double                    weight;
    double                  distance;
    double                    radius;
    double              lookupWeight;
    double         mDistScalingUnits;
    CacheTimeMS      mLastAccessTime;
    double                      vMag;
    SpaceContext*                ctx;
  };

}


#endif
