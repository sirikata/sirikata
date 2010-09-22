/*  Sirikata
 *  CacheRecords.cpp
 *
 *  Copyright (c) 2010, Behram Mistree
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cassert>
#include "CacheRecords.hpp"
#include "Utility.hpp"

namespace Sirikata
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
