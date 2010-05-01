#include "Complete_Cache.hpp"
#include "../craq_oseg/CraqEntry.hpp"
#include "../SpaceContext.hpp"
#include <boost/thread/mutex.hpp>
#include "CraqCache.hpp"


#ifndef __COMMUNICATION_CACHE_HPP__
#define __COMMUNICATION_CACHE_HPP__

namespace CBR
{

  double commCacheScoreFunction(const FCacheRecord* a);
  double commCacheScoreFunctionPrint(const FCacheRecord* a,bool toPrint);


  class CommunicationCache : public CraqCache
  {
  private:
    Complete_Cache mCompleteCache;
    float mDistScaledUnits;
    float mCentralX;
    float mCentralY;
    float mCentralZ;
    CoordinateSegmentation* mCSeg;
    SpaceContext* ctx;
    boost::mutex mMutex;
    uint32 mCacheSize;
    
  public:
    CommunicationCache(SpaceContext* spctx, float scalingUnits, CoordinateSegmentation* cseg,uint32 cacheSize);
    
    virtual void insert(const UUID& uuid, const CraqEntry& sID);
    virtual const CraqEntry& get(const UUID& uuid);
    virtual void remove(const UUID& oid);
  
  };
}
#endif


