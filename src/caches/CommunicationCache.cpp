#include "CommunicationCache.hpp"
#include <float.h>
#include <cmath>
#include "../craq_oseg/CraqEntry.hpp"
#include "Proximity.hpp"
#include "../SpaceContext.hpp"

namespace CBR
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


