/*  Sirikata
 *  CacheLRUOriginal.cpp
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


#include "CacheLRUOriginal.hpp"
#include <algorithm>
#include <map>
#include <list>
#include <functional>
#include <iostream>
#include <iomanip>
#include <boost/thread/mutex.hpp>
#include <sirikata/cbrcore/Options.hpp>



namespace Sirikata
{

  CacheLRUOriginal::CacheLRUOriginal(Context* ctx, uint32 maxSize,uint32 cleanGroupSize,Duration entryLifetime)
    : mContext(ctx),
      mMaxCacheSize(maxSize),
      mCleanGroupSize(cleanGroupSize),
      mEntryLifetime(entryLifetime)
  {
    mTimer.start();
    insertMilliseconds = 0;
    numInserted = 0;
    maintainDur = 0;
    numMaintained = 0;
  }

  CacheLRUOriginal::~CacheLRUOriginal()
  {

    std::cout<<"\n\n  insertMilliseconds:  "<<insertMilliseconds<<"  numInserted:   "<<numInserted<<"    avg: "<<((double)insertMilliseconds)/((double) numInserted)<<"\n\n";

    std::cout<<"\n\n maintainDur:  "<<maintainDur<<"   numMaintained  "<<numMaintained<<"  avg: "<<((double)maintainDur)/((double)numMaintained)<<"\n\n";

  }


  //kill the last x objects that were in that map
  void CacheLRUOriginal::maintain()
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


  void CacheLRUOriginal::insert(const UUID& uuid, const CraqEntry& sID)
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
          CraqCacheRecordLRUOriginal* rcd = new CraqCacheRecordLRUOriginal;
          rcd->obj_id = uuid;
          rcd->age    = currentDur.toMilliseconds();
          rcd->sID    = sID;
          timeRecMap.insert(std::pair<int,CraqCacheRecordLRUOriginal* >(currentDur.toMilliseconds(), rcd));

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
      CraqCacheRecordLRUOriginal* rcdTimeRecMap = new CraqCacheRecordLRUOriginal;
      rcdTimeRecMap->obj_id = uuid;
      rcdTimeRecMap->age    = currentDur.toMilliseconds();
      rcdTimeRecMap->sID    = sID;

      timeRecMap.insert(std::pair<int,CraqCacheRecordLRUOriginal*>(currentDur.toMilliseconds(),rcdTimeRecMap));

      //insert into id-record map
      CraqCacheRecordLRUOriginal* rcdIDRecMap   = new CraqCacheRecordLRUOriginal;
      rcdIDRecMap->obj_id   = uuid;
      rcdIDRecMap->age      = currentDur.toMilliseconds();
      rcdIDRecMap->sID      = sID;

      idRecMap.insert(std::pair<UUID,CraqCacheRecordLRUOriginal*>(uuid,rcdIDRecMap));
      maintain();
    }

    Duration endingDur = mTimer.elapsed();
    insertMilliseconds += endingDur.toMilliseconds() - beginningDur.toMilliseconds();
    ++numInserted;


  }


  const CraqEntry& CacheLRUOriginal::get(const UUID& uuid)
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

  //delete the data;
  void CacheLRUOriginal::remove(const UUID& uuid)
  {
    boost::lock_guard<boost::mutex> lck(mMutex);
    IDRecordMap::iterator iter = idRecMap.find(uuid);
    if (iter != idRecMap.end())
    {
      //record exists, must delete it from both time map and id map


      TimeRecordMap::iterator timeIter = timeRecMap.find(iter->second->age);
      if (timeIter == timeRecMap.end())
      {
        std::cout<<"\n\nProblem.  Time record and id record are out of sync\n\n";
        assert(false);
      }

      delete timeIter->second;
      delete iter->second;

      //erase from timeRecMap
      timeRecMap.erase(timeIter);

      //erase from idRecmap
      idRecMap.erase(iter);
    }
  }


//if inAge indicates that object is young enough, then return true.
//otherwise, return false
bool CacheLRUOriginal::satisfiesCacheAgeCondition(int inAge)
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
