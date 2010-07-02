/*  Sirikata
 *  CommunicationCache.cpp
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

#include "CommunicationCache.hpp"
#include <float.h>
#include <cmath>
#include "../craq_oseg/CraqEntry.hpp"
#include <sirikata/space/CoordinateSegmentation.hpp>

namespace Sirikata
{

  //lookupWeight
  double commCacheScoreFunction(const FCacheRecord* a)
  {
    return commCacheScoreFunctionPrint(a,false);
  }

  double commCacheScoreFunctionPrint(const FCacheRecord* a,bool toPrint)
  {
    return a->lookupWeight;
  }



  CommunicationCache::CommunicationCache(SpaceContext* spctx, float scalingUnits, CoordinateSegmentation* cseg,uint32 cacheSize)
    : mCompleteCache(.2,"CommunicationCache",&commCacheScoreFunction,&commCacheScoreFunctionPrint,spctx,cacheSize,FLT_MAX),
      mDistScaledUnits(scalingUnits),
      mCSeg(cseg),
      ctx(spctx),
      mCacheSize(cacheSize)
  {

    BoundingBoxList bboxes = mCSeg->serverRegion(ctx->id());
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);

    mCentralX = (bbox.max().x + bbox.min().x)/2;
    mCentralY = (bbox.max().y + bbox.min().y)/2;
    mCentralZ = (bbox.max().z + bbox.min().z)/2;


  }

  void CommunicationCache::insert(const UUID& uuid, const CraqEntry& sID)
  {
    BoundingBoxList bboxes = mCSeg->serverRegion(sID.server());
    BoundingBox3f bbox = bboxes[0];
    for(uint32 i = 1; i< bboxes.size(); i++)
        bbox.mergeIn(bboxes[i]);

    float xer = mCentralX - ((bbox.max().x + bbox.min().x)/2);
    float yer = mCentralY - ((bbox.max().y + bbox.min().y)/2);
    float zer = mCentralZ - ((bbox.max().z + bbox.min().z)/2);
    float distance = sqrt(xer*xer + yer*yer + zer*zer);

    float logger       = log(mDistScaledUnits*distance);
    float lookupWeight = sID.radius()/(mDistScaledUnits* distance*distance*logger*logger);

    boost::lock_guard<boost::mutex> lck(mMutex);
    mCompleteCache.insert(uuid,sID.server(),0,0,0,0,sID.radius(),lookupWeight,1);
  }

  const CraqEntry& CommunicationCache::get(const UUID& uuid)
  {
    boost::lock_guard<boost::mutex> lck(mMutex);
    return mCompleteCache.lookup(uuid);
  }

  void CommunicationCache::remove(const UUID& oid)
  {
    boost::lock_guard<boost::mutex> lck(mMutex);
    mCompleteCache.remove(oid);
  }
}
