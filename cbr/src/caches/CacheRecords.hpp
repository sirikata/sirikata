/*  Sirikata
 *  CacheRecords.hpp
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


#include "Utility.hpp"
#include <sirikata/cbrcore/SpaceContext.hpp>

#ifndef __CACHE_RECORDS_HPP__
#define __CACHE_RECORDS_HPP__

namespace Sirikata
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
