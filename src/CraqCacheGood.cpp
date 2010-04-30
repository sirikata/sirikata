
#include "CraqCacheGood.hpp"
#include <algorithm>
#include <map>
#include <list>
#include <functional>
#include <iostream>
#include <iomanip>
#include "Timer.hpp"
#include <boost/thread/mutex.hpp>
#include "Options.hpp"

namespace CBR
{

  CraqCacheGood::CraqCacheGood(Context* ctx)
   : mContext(ctx)
  {
      mTimer.start();
    insertMilliseconds = 0;
    numInserted = 0;
    maintainDur = 0;
    numMaintained = 0;

      mMaxCacheSize = GetOption(OSEG_CACHE_SIZE)->as<uint32>();
      mCleanGroupSize = GetOption(OSEG_CACHE_CLEAN_GROUP_SIZE)->as<uint32>();
      mEntryLifetime = GetOption(OSEG_CACHE_ENTRY_LIFETIME)->as<Duration>();
  }

  CraqCacheGood::~CraqCacheGood()
  {

    std::cout<<"\n\n  insertMilliseconds:  "<<insertMilliseconds<<"  numInserted:   "<<numInserted<<"    avg: "<<((double)insertMilliseconds)/((double) numInserted)<<"\n\n";

    std::cout<<"\n\n maintainDur:  "<<maintainDur<<"   numMaintained  "<<numMaintained<<"  avg: "<<((double)maintainDur)/((double)numMaintained)<<"\n\n";

  }


  //kill the last x objects that were in that map
  void CraqCacheGood::maintain()
  {
    if (idRecMap.size() > mMaxCacheSize)
    {
        static Time nulltime = Time::null();
        Duration beginningDur = mContext->recentSimTime()-nulltime;
      int numObjectsDeleted = 0;

      TimeRecordMap::iterator tMapIter = timeRecMap.begin();
      for (uint32 s=0; (s < mCleanGroupSize) && (tMapIter != timeRecMap.end()); ++s)
      {
        //remove the object from the map
        IDRecordMap::iterator idRecMapIterator =  idRecMap.find(tMapIter->second->obj_id);
        if (idRecMapIterator == idRecMap.end())
        {
          printf("\n\nInconsistent ids\n\n");
          fflush(stdout);
          assert(false);
        }
        delete idRecMapIterator->second;
        idRecMap.erase(idRecMapIterator);

        //delete this record from the timeRecMap
        delete tMapIter->second;

        //delete the record from the multimap
        timeRecMap.erase(tMapIter++);

      }
      Duration endingDur = mTimer.elapsed();
      maintainDur += endingDur.toMilliseconds() - beginningDur.toMilliseconds();
      ++numMaintained;
    }


  }


  void CraqCacheGood::insert(const UUID& uuid, const CraqEntry& sID)
  {
      boost::lock_guard<boost::mutex> lck(mMutex);

    Duration beginningDur = mTimer.elapsed();

    Duration currentDur = mTimer.elapsed();

    IDRecordMap::iterator idRecMapIter = idRecMap.find(uuid);
    if (idRecMapIter != idRecMap.end())
    {
      //object already exists.  We need to update the record associated with it.
      int previousTime = idRecMapIter->second->age;
      idRecMapIter->second->age = currentDur.toMilliseconds();
      idRecMapIter->second->sID  = sID;


      //update the value in the timerecordmap
      std::pair<TimeRecordMap::iterator, TimeRecordMap::iterator> eqRange = timeRecMap.equal_range(previousTime);
      TimeRecordMap::iterator timeRecMapIter;
      bool found = false;
      for(timeRecMapIter = eqRange.first; timeRecMapIter != eqRange.second; ++timeRecMapIter)
      {
        //run through looking for the corresponding
        if(timeRecMapIter->second->obj_id.toString() == uuid.toString())

        {
          //delete its entry
          delete timeRecMapIter->second;
          //remove the iterator
          timeRecMap.erase(timeRecMapIter);

          //re-insert the same record (with updated age) back into the multimap
          CraqCacheRecord* rcd = new CraqCacheRecord;
          rcd->obj_id = uuid;
          rcd->age    = currentDur.toMilliseconds();
          rcd->sID    = sID;
          timeRecMap.insert(std::pair<int,CraqCacheRecord* >(currentDur.toMilliseconds(), rcd));

          found = true;
          break;
        }
      }

      if (! found)
      {
        printf("\n\n Issues with maintaining craq cache correctly \n\n");
        fflush(stdout);
        assert(false);
      }
    }
    else
    {
      //means that the object did not already exist in the cache.  We're going to have to specifically add it

      //insert into time-record multimap
      CraqCacheRecord* rcdTimeRecMap = new CraqCacheRecord;
      rcdTimeRecMap->obj_id = uuid;
      rcdTimeRecMap->age    = currentDur.toMilliseconds();
      rcdTimeRecMap->sID    = sID;

      timeRecMap.insert(std::pair<int,CraqCacheRecord*>(currentDur.toMilliseconds(),rcdTimeRecMap));

      //insert into id-record map
      CraqCacheRecord* rcdIDRecMap   = new CraqCacheRecord;
      rcdIDRecMap->obj_id   = uuid;
      rcdIDRecMap->age      = currentDur.toMilliseconds();
      rcdIDRecMap->sID      = sID;

      idRecMap.insert(std::pair<UUID,CraqCacheRecord*>(uuid,rcdIDRecMap));
      maintain();
    }

    Duration endingDur = mTimer.elapsed();
    insertMilliseconds += endingDur.toMilliseconds() - beginningDur.toMilliseconds();
    ++numInserted;


  }


  const CraqEntry& CraqCacheGood::get(const UUID& uuid)
  {
      boost::lock_guard<boost::mutex> lck(mMutex);

    IDRecordMap::iterator idRecMapIter = idRecMap.find(uuid);

    if (idRecMapIter != idRecMap.end())
    {
      //means that we have a record of the object
      if (satisfiesCacheAgeCondition(idRecMapIter->second->age))
      {
        return idRecMapIter->second->sID;
      }
    }
    static CraqEntry justnothin(CraqEntry::null());
    return justnothin;
  }


//if inAge indicates that object is young enough, then return true.
//otherwise, return false
bool CraqCacheGood::satisfiesCacheAgeCondition(int inAge)
{
    static Time nulltime = Time::null();
    Duration currentDur = mContext->recentSimTime()-nulltime;
  if (currentDur.toMilliseconds() - inAge > mEntryLifetime.toMilliseconds())
  {
    return false;
  }
  return true;
}


}//end namespace
