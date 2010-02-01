#ifndef ___CBR_CRAQ_CACHE_GOOD_HPP___
#define ___CBR_CRAQ_CACHE_GOOD_HPP___




#include <map>
#include <functional>
#include <vector>
#include <list>
#include <boost/bind.hpp>
#include "Utility.hpp"
#include "Timer.hpp"
#include <boost/thread/mutex.hpp>


namespace CBR
{
  struct CraqCacheRecord
  {
    UUID obj_id;
    int age;
    ServerID sID;
  };


  class CraqCacheGood
  {
  private:
    typedef std::map<UUID,CraqCacheRecord*> IDRecordMap;
    IDRecordMap idRecMap;

    typedef std::multimap<int,CraqCacheRecord*> TimeRecordMap;
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
    CraqCacheGood();
    ~CraqCacheGood();

    void insert(const UUID& uuid, const ServerID& sID);
    ServerID get(const UUID& uuid);
  };
}

#endif
