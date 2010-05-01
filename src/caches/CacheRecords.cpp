#include <cassert>
#include "CacheRecords.hpp"
#include "../SpaceContext.hpp"
#include "Utility.hpp"

namespace CBR
{

  //fcache
  FCacheRecord::FCacheRecord()
    : bID(NullServerID),
      popAvg(0),
      reqsSinceEvict(0),
      lastEvictNum(0),
      vecIndex(CACHE_RECORDS_NULL_VEC_INDEX),
      weight(0),
      distance(0),
      radius(0),
      lookupWeight(0),
      mDistScalingUnits(1),
      mLastAccessTime(0),
      vMag(0)
  {
    //note: not initializing objid
  }


  FCacheRecord::FCacheRecord(const FCacheRecord& fcr)
    : bID(fcr.bID),
      objid(fcr.objid),
      popAvg(fcr.popAvg),
      reqsSinceEvict(fcr.reqsSinceEvict),
      lastEvictNum(fcr.lastEvictNum),
      vecIndex(fcr.vecIndex),
      weight(fcr.weight),
      distance(fcr.distance),
      radius(fcr.radius),
      lookupWeight(fcr.lookupWeight),
      mDistScalingUnits(fcr.mDistScalingUnits)
  {
    ctx = fcr.ctx;
    mLastAccessTime = fcr.mLastAccessTime;
    vMag = fcr.vMag;
  }

  FCacheRecord::FCacheRecord(const FCacheRecord* fcr)
    : bID(fcr->bID),
      objid(fcr->objid),
      popAvg(fcr->popAvg),
      reqsSinceEvict(fcr->reqsSinceEvict),
      lastEvictNum(fcr->lastEvictNum),
      vecIndex(fcr->vecIndex),
      weight(fcr->weight),
      distance(fcr->distance),
      radius(fcr->radius),
      lookupWeight(fcr->lookupWeight),
      mDistScalingUnits(fcr->mDistScalingUnits)
  {
    ctx = fcr->ctx;
    mLastAccessTime = fcr->mLastAccessTime;
    vMag = fcr->vMag;
  }



  FCacheRecord::FCacheRecord(UUID oid, ServerID bid, double pAvg, int reqsAftEvict,int gEvNum,double weighter,double distancer, double radiuser,double lookupWeighter,double distScaleUnits, SpaceContext* spctx, double vMag)
    : bID(bid),
      objid(oid),
      popAvg(pAvg),
      reqsSinceEvict(reqsAftEvict),
      lastEvictNum(gEvNum),
      vecIndex(CACHE_RECORDS_NULL_VEC_INDEX),
      weight(weighter),
      distance(distancer),
      radius(radiuser),
      lookupWeight(lookupWeighter),
      mDistScalingUnits(distScaleUnits)
  {
    ctx = spctx;
    mLastAccessTime = 0;
    this->vMag = vMag;
  }

  void FCacheRecord::copy(UUID oid, ServerID bid, double pAvg, int reqsAftEvict,int gEvNum,int index,double weighter,double distancer,double radiuser,double lookupWeighter,double distScaleUnits, SpaceContext* spctx, CacheTimeMS lastAccessTime, double vMag)
  {
    objid              =              oid;
    bID                =              bid;
    popAvg             =             pAvg;
    reqsSinceEvict     =     reqsAftEvict;
    lastEvictNum       =           gEvNum;
    vecIndex           =            index;
    weight             =         weighter;
    distance           =        distancer;
    radius             =         radiuser;
    lookupWeight       =   lookupWeighter;
    mDistScalingUnits  =   distScaleUnits;
    ctx                =            spctx;
    this->vMag 	       =             vMag;
  }

  void FCacheRecord::printRecord()
  {
    std::cout<<"\n BlockID: "<<bID<<"  ObjID: "<<objid.toString()<<"  popAvg: "<<popAvg<<"  reqsSinceEvict: "<<reqsSinceEvict<<"  lastEvictNum: "<<lastEvictNum<<"  weight: "<<weight<<"  distance: "<<distance<<"   radius: "<<radius<< " \n";
  }

  FCacheRecord::~FCacheRecord()
  {
  }
  
}

