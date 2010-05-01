#ifndef ___CBR_CRAQ_CACHE_GOOD_HPP___
#define ___CBR_CRAQ_CACHE_GOOD_HPP___




#include <map>
#include <functional>
#include <vector>
#include <list>
#include <boost/bind.hpp>
#include "../Utility.hpp"
#include "../Timer.hpp"
#include <boost/thread/mutex.hpp>
#include "../craq_oseg/CraqEntry.hpp"
#include "../Context.hpp"
#include "CraqCache.hpp"

namespace CBR
{
  struct CraqCacheRecordLRUOriginal
  {
    UUID obj_id;
    int age;
    CraqEntry sID;
    CraqCacheRecordLRUOriginal():sID(CraqEntry::null()){}
  };


  class CacheLRUOriginal : public CraqCache
  {
  private:
    Context* mContext;

    typedef std::tr1::unordered_map<UUID,CraqCacheRecordLRUOriginal*,UUIDHasher> IDRecordMap;
    IDRecordMap idRecMap;

    typedef std::multimap<int,CraqCacheRecordLRUOriginal*> TimeRecordMap;
    TimeRecordMap timeRecMap;
    IOStrand* mStrand;

    Timer mTimer;
    void maintain();
    bool satisfiesCacheAgeCondition(int inAge);

    boost::mutex mMutex;

    double insertMilliseconds;
    int numInserted;
    double maintainDur;
    int numMaintained;


      uint32 mMaxCacheSize; //what is the most number of objects that we can have in the craq cache before we start deleting them
      uint32 mCleanGroupSize; //how many should delete at a time when we get to our limit.
      Duration mEntryLifetime; //maximum age of a cache entry

  public:
    CacheLRUOriginal(Context* ctx, uint32 maxSize,uint32 cleanGroupSize,Duration entryLifetime);
    ~CacheLRUOriginal();

    virtual void insert(const UUID& uuid, const CraqEntry& sID);
    virtual const CraqEntry& get(const UUID& uuid);
    virtual void remove(const UUID& uuid);
  };
}

#endif
